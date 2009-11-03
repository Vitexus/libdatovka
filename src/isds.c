#define _XOPEN_SOURCE 500   /* strdup from string.h, strptime from time.h */
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdarg.h>
#include <time.h>
#include "isds_priv.h"
#include "utils.h"
#include "soap.h"
#include "validator.h"


/* Initialize ISDS library.
 * Global function, must be called before other functions.
 * If it failes you can not use ISDS library and must call isds_cleanup() to
 * free partially inititialized global variables. */
isds_error isds_init(void) {
    /* NULL global variables */
    xml_node = NULL;
    soap_ns = NULL;
    log_facilities = ILF_NONE;
    log_level = ILL_NONE;

    /* Initialize CURL */
    if (curl_global_init(CURL_GLOBAL_ALL)) {
        return IE_ERROR;
    }

    /* This can _exit() current program. Find not so assertive check. */
    LIBXML_TEST_VERSION;

    /* Allocate global variables */
    if (!(xml_node = xmlNewNode(NULL, BAD_CAST "global-element")))
        return IE_ERROR;
    if (!(soap_ns = xmlNewNs(NULL, BAD_CAST SOAP_NS, BAD_CAST "soap")))
        return IE_ERROR;
    if (!(isds_ns = xmlNewNs(NULL, BAD_CAST ISDS_NS, BAD_CAST "isds")))
        return IE_ERROR;

    return IE_SUCCESS;
}


/* Deinicialize ISDS library.
 * Global function, must be called as last library function. */
isds_error isds_cleanup(void) {
    /* XML */
    xmlFreeNs(isds_ns);
    xmlFreeNs(soap_ns);
    xmlFreeNode(xml_node);
    xmlCleanupParser();
    
    /* Curl */
    curl_global_cleanup();

    return IE_SUCCESS;
}


/* Return text description of ISDS error */
char *isds_strerror(const isds_error error) {
    switch (error) {
        case IE_SUCCESS:
            return(_("Success")); break;
        case IE_ERROR:
            return(_("Unspecified error")); break;
        case IE_NOTSUP:
            return(_("Not supported")); break;
        case IE_INVAL:
            return(_("Invalid value")); break;
        case IE_INVALID_CONTEXT:
            return(_("Invalid context")); break;
        case IE_NOT_LOGGED_IN:
            return(_("Not logged in")); break;
        case IE_CONNECTION_CLOSED:
            return(_("Connection closed")); break;
        case IE_TIMED_OUT:
            return(_("Timed out")); break;
        case IE_NOEXIST:
            return(_("Not exist")); break;
        case IE_NOMEM:
            return(_("Out of memory")); break;
        case IE_NETWORK:
            return(_("Network problem")); break;
        case IE_SOAP:
            return(_("SOAP problem")); break;
        case IE_XML:
            return(_("XML problem")); break;
        case IE_ISDS:
            return(_("ISDS server problem")); break;
        case IE_ENUM:
            return(_("Invalid enum value")); break;
        case IE_DATE:
            return(_("Invalid date value")); break;
        default:
            return(_("Unknown error"));
    }
}


/* Create ISDS context.
 * Each context can be used for different sessions to (possibly) differnet
 * ISDS server with different credentials. */
struct isds_ctx *isds_ctx_create(void) {
    struct isds_ctx *context;
    context = malloc(sizeof(*context));
    if (context) memset(context, 0, sizeof(*context));
    return context;
};


/* Destroy ISDS context and free memmory.
 * @context will be NULLed on success. */
isds_error isds_ctx_free(struct isds_ctx **context) {
    if (!context || !*context) {
        return IE_INVALID_CONTEXT;
    }
  
    /* Discard credentials */
    isds_logout(*context);

    /* Free other structures */
    free((*context)->long_message);

    free(*context);
    *context = NULL;
    return IE_SUCCESS;
}


/* Return long message text produced by library fucntion, e.g. detailed error
 * mesage. Returned pointer is only valid until new library function is
 * called for the same context. Could be NULL, especially if NULL context is
 * supplied. */
char *isds_long_message(const struct isds_ctx *context) {
    if (!context) return NULL;
    return context->long_message;
}


/* Stores message into context' long_message buffer.
 * Application can pick the message up using isds_long_message().
 * NULL @message truncates the buffer but does not deallocate it. */
_hidden isds_error isds_log_message(struct isds_ctx *context,
        const char *message) {
    char *buffer;
    size_t length;
    
    if (!context) return IE_INVALID_CONTEXT;
    
    /* FIXME: Check for integer overflow */
    length = 1 + ((message) ? strlen(message) : 0);
    buffer = realloc(context->long_message, length);
    if (!buffer) return IE_NOMEM;

    if (message)
        strcpy(buffer, message);
    else
        *buffer = '\0';

    context->long_message = buffer;
    return IE_SUCCESS;
}


/* Appends message into context' long_message buffer.
 * Application can pick the message up using isds_long_message().
 * NULL message has void effect. */
_hidden isds_error isds_append_message(struct isds_ctx *context,
        const char *message) {
    char *buffer;
    size_t old_length, length;
    
    if (!context) return IE_INVALID_CONTEXT;
    if (!message) return IE_SUCCESS;
    if (!context->long_message)
        return isds_log_message(context, message);
    
    old_length = strlen(context->long_message);
    /* FIXME: Check for integer overflow */
    length = 1 + old_length + strlen(message);
    buffer = realloc(context->long_message, length);
    if (!buffer) return IE_NOMEM;

    strcpy(buffer + old_length, message);

    context->long_message = buffer;
    return IE_SUCCESS;
}


/* Set logging up.
 * @facilities is bitmask of isds_log_facility values,
 * @level is verbosity level. */
void isds_set_logging(const unsigned int facilities,
        const isds_log_level level) {
    log_facilities = facilities;
    log_level = level;
}


/* Log @message in class @facility with log @level into global log. @message
 * is printf(3) formating string, variadic arguments may be neccessary.
 * For debugging purposes. */
_hidden isds_error isds_log(const isds_log_facility facility,
        const isds_log_level level, const char *message, ...) {
    va_list ap;

    if (level > log_level) return IE_SUCCESS;
    if (!(log_facilities & facility)) return IE_SUCCESS;
    if (!message) return IE_INVAL;

    /* TODO: Allow to register output function privided by application
     * (e.g. fprintf to stderr or copy to text area GUI widget). */

    va_start(ap, message);
    vfprintf(stderr, message, ap);
    va_end(ap);
    /* Line buffered printf is default.
     * fflush(stderr);*/

    return IE_SUCCESS;
}


/* Connect to given url.
 * It just makes TCP connection to ISDS server found in @url hostname part. */
/*int isds_connect(struct isds_ctx *context, const char *url);*/

/* Set timeout in miliseconds for each network job like connecting to server
 * or sending message. Use 0 to disable timeout limits. */
isds_error isds_set_timeout(struct isds_ctx *context,
        const unsigned int timeout) {
    if (!context) return IE_INVALID_CONTEXT;

    context->timeout = timeout;

    if (context->curl) {
        CURLcode curl_err;

        curl_err = curl_easy_setopt(context->curl, CURLOPT_NOSIGNAL, 1);
        if (!curl_err) 
            curl_err = curl_easy_setopt(context->curl, CURLOPT_TIMEOUT_MS,
                    context->timeout);
        if (curl_err) return IE_ERROR;
    }

    return IE_SUCCESS;
}


/* Discard credentials.
 * Only that. It does not cause log out, connection close or similar. */
static isds_error discard_credentials(struct isds_ctx *context) {
    if(!context) return IE_INVALID_CONTEXT;

    if (context->username) {
        memset(context->username, 0, strlen(context->username));
        free(context->username);
        context->username = NULL;
    }
    if (context->password) {
        memset(context->password, 0, strlen(context->password));
        free(context->password);
        context->password = NULL;
    }

    return IE_SUCCESS;
}


/* Connect and log in into ISDS server.
 * @url is address of ISDS web service
 * @username is user name of ISDS user
 * @password is user's secret password
 * @certificate is NULL terminated string with PEM formated client's
 * certificate. Use NULL if only password autentication should be performed.
 * @key is private key for client's certificate as (base64 encoded?) NULL
 * terminated string. Use NULL if only password autentication is desired.
 * */
isds_error isds_login(struct isds_ctx *context, const char *url,
        const char *username, const char *password,
        const char *certificate, const char* key) {
    isds_error err = IE_NOT_LOGGED_IN;
    isds_error soap_err;
    xmlNsPtr isds_ns = NULL;
    xmlNodePtr request = NULL;
    xmlNodePtr response = NULL;

    if (!context) return IE_INVALID_CONTEXT;
    if (!url || !username || !password) return IE_INVAL;
    if (certificate || key) return IE_NOTSUP;

    /* Store configuration */
    free(context->url);
    context->url = strdup(url);
    if (!(context->url))
        return IE_NOMEM;

    /* Close connection if already logged in */
    if (context->curl) {
        close_connection(context);
    }

    /* Prepare CURL handle */
    context->curl = curl_easy_init();
    if (!(context->curl))
        return IE_ERROR;

    /* Build login request */
    request = xmlNewNode(NULL, BAD_CAST "DummyOperation");
    if (!request) {
        isds_log_message(context, _("Could build ISDS login request"));
        return IE_ERROR;
    }
    isds_ns = xmlNewNs(request, BAD_CAST ISDS_NS, NULL);
    if(!isds_ns) {
        isds_log_message(context, _("Could not create ISDS name space"));
        xmlFreeNode(request);
        return IE_ERROR;
    }
    xmlSetNs(request, isds_ns);

    /* Store credentials */
    /* FIXME: mlock password
     * (I have a library) */
    discard_credentials(context);
    context->username = strdup(username);
    context->password = strdup(password);
    if (!(context->username && context->password)) {
        discard_credentials(context);
        xmlFreeNode(request);
        return IE_NOMEM;
    }

    isds_log(ILF_ISDS, ILL_DEBUG, _("Logging user %s into server %s\n"),
            username, url);

    /* Send login request */
    soap_err = soap(context, "dz", request, &response);
   
    /* Remove credentials */
    discard_credentials(context);
   
    /* Destroy login request */
    xmlFreeNode(request);

    if (soap_err) {
        xmlFreeNodeList(response);
        close_connection(context);
        return soap_err;
    }

    /* XXX: Until we don't propagate HTTP code 500 or 4xx, we can be sure
     * authentication succeeded if soap_err == IE_SUCCESS */
    err = IE_SUCCESS;

    xmlFreeNodeList(response);

    if (!err) 
        isds_log(ILF_ISDS, ILL_DEBUG,
                _("User %s has been logged into server %s successfully\n"),
            username, url);
    return err;
}


/* Log out from ISDS server discards credentials and connection configuration. */
isds_error isds_logout(struct isds_ctx *context) {
    if (!context) return IE_INVALID_CONTEXT;

    /* Close connection */
    if (context->curl) {
        close_connection(context);
    }

    /* Discard credentials for sure. They should not survive isds_login(),
     * even successful .*/
    discard_credentials(context);
    free(context->url);
    context->url = NULL;

    isds_log(ILF_ISDS, ILL_DEBUG, _("Logged out from ISDS server\n"));
    return IE_SUCCESS;
}


/* Verify connection to ISDS is alive and server is responding.
 * Sent dumy request to ISDS and expect dummy response. */
isds_error isds_ping(struct isds_ctx *context) {
    isds_error soap_err;
    xmlNsPtr isds_ns = NULL;
    xmlNodePtr request = NULL;
    xmlNodePtr response = NULL;

    if (!context) return IE_INVALID_CONTEXT;

    /* Check if connection is established */
    if (!context->curl) return IE_CONNECTION_CLOSED;


    /* Build dummy request */
    request = xmlNewNode(NULL, BAD_CAST "DummyOperation");
    if (!request) {
        isds_log_message(context, _("Could build ISDS dummy request"));
        return IE_ERROR;
    }
    isds_ns = xmlNewNs(request, BAD_CAST ISDS_NS, NULL);
    if(!isds_ns) {
        isds_log_message(context, _("Could not create ISDS name space"));
        xmlFreeNode(request);
        return IE_ERROR;
    }
    xmlSetNs(request, isds_ns);

    isds_log(ILF_ISDS, ILL_DEBUG, _("Pinging ISDS server\n"));

    /* Sent dummy request */
    soap_err = soap(context, "dz", request, &response);
   
    /* Destroy login request */
    xmlFreeNode(request);

    if (soap_err) {
        isds_log(ILF_ISDS, ILL_DEBUG,
                _("ISDS server could not be contacted\n"));
        xmlFreeNodeList(response);
        close_connection(context);
        return soap_err;
    }

    /* XXX: Untill we don't propagate HTTP code 500 or 4xx, we can be sure
     * authentication succeeded if soap_err == IE_SUCCESS */
    /* TODO: ISDS documentation does not specify response body.
     * However real server sends back DummyOperationResponse */
    

    xmlFreeNodeList(response);

    isds_log(ILF_ISDS, ILL_DEBUG, _("ISDS server alive\n"));

    return IE_SUCCESS;
}


/* Send bogus request to ISDS.
 * Just for test purposes */
isds_error isds_bogus_request(struct isds_ctx *context) {
    isds_error err;
    xmlNsPtr isds_ns = NULL;
    xmlNodePtr request = NULL;
    xmlDocPtr response = NULL;
    xmlChar *code = NULL, *message = NULL;

    if (!context) return IE_INVALID_CONTEXT;

    /* Check if connection is established */
    if (!context->curl) return IE_CONNECTION_CLOSED;


    /* Build dummy request */
    request = xmlNewNode(NULL, BAD_CAST "X-BogusOperation");
    if (!request) {
        isds_log_message(context, _("Could build ISDS bogus request"));
        return IE_ERROR;
    }
    isds_ns = xmlNewNs(request, BAD_CAST ISDS_NS, NULL);
    if(!isds_ns) {
        isds_log_message(context, _("Could not create ISDS name space"));
        xmlFreeNode(request);
        return IE_ERROR;
    }
    xmlSetNs(request, isds_ns);

    isds_log(ILF_ISDS, ILL_DEBUG, _("Sending bogus request to ISDS\n"));

    /* Sent bogus request */
    err = isds(context, SERVICE_DM_OPERATIONS, request, &response);
   
    /* Destroy request */
    xmlFreeNode(request);

    if (err) {
        isds_log(ILF_ISDS, ILL_DEBUG,
                _("Processing ISDS response on bogus request failed\n"));
        xmlFreeDoc(response);
        return err;
    }

    /* Check for response status */
    err = isds_response_status(context, SERVICE_DM_OPERATIONS, response,
            &code, &message, NULL);
    if (err) {
        isds_log(ILF_ISDS, ILL_DEBUG,
                _("ISDS response on bogus request is missing status\n"));
        free(code);
        free(message);
        xmlFreeDoc(response);
        return err;
    }
    if (xmlStrcmp(code, BAD_CAST "0000")) {
        /* FIXME: Convert UTF-8 into locale */
        isds_log(ILF_ISDS, ILL_DEBUG,
                _("Server refused bogus request (code=%s, message=%s)\n"),
                code, message);
        isds_log_message(context, _((char *)message));
        free(code);
        free(message);
        xmlFreeDoc(response);
        return IE_ISDS;
    }
   

    free(code);
    free(message);
    xmlFreeDoc(response);

    isds_log(ILF_ISDS, ILL_DEBUG,
            _("Bogus message accepted by server. This should not happen.\n"));

    return IE_SUCCESS;
}


/* Convert UTF-8 @string represantion of ISDS dbType to enum @type */
static isds_error string2isds_DbType(xmlChar *string, isds_DbType *type) {
    if (!string || !type) return IE_INVAL;

    if (!xmlStrcmp(string, BAD_CAST "FO"))
        *type = DBTYPE_FO;
    else if (!xmlStrcmp(string, BAD_CAST "PFO"))
        *type = DBTYPE_PFO;
    else if (!xmlStrcmp(string, BAD_CAST "PFO_ADVOK"))
        *type = DBTYPE_PFO_ADVOK;
    else if (!xmlStrcmp(string, BAD_CAST "PFO_DANPOR"))
        *type = DBTYPE_PFO_DANPOR;
    else if (!xmlStrcmp(string, BAD_CAST "PFO_INSSPR"))
        *type = DBTYPE_PFO_INSSPR;
    else if (!xmlStrcmp(string, BAD_CAST "PO"))
        *type = DBTYPE_PO;
    else if (!xmlStrcmp(string, BAD_CAST "PO_ZAK"))
        *type = DBTYPE_PO_ZAK;
    else if (!xmlStrcmp(string, BAD_CAST "PO_REQ"))
        *type = DBTYPE_PO_REQ;
    else if (!xmlStrcmp(string, BAD_CAST "OVM"))
        *type = DBTYPE_OVM;
    else if (!xmlStrcmp(string, BAD_CAST "OVM_NOTAR"))
        *type = DBTYPE_OVM_NOTAR;
    else if (!xmlStrcmp(string, BAD_CAST "OVM_EXEKUT"))
        *type = DBTYPE_OVM_EXEKUT;
    else if (!xmlStrcmp(string, BAD_CAST "OVM_REQ"))
        *type = DBTYPE_OVM_REQ;
    else
        return IE_ENUM;
    return IE_SUCCESS;
}


/* Convert UTF-8 @string represantion of ISO 8601 date to @time.
 * XXX: Not all ISO formats are supported */
static isds_error datestring2tm(xmlChar *string, struct tm *time) {
    char *offset;
    if (!string || !time) return IE_INVAL;
    
    /* xsd:date is ISO 8601 string, thus ASCII */
    offset = strptime((char*)string, "%Y-%m-%d", time);
    if (offset && *offset == '\0')
        return IE_SUCCESS;

    offset = strptime((char*)string, "%Y%m%d", time);
    if (offset && *offset == '\0')
        return IE_SUCCESS;

    offset = strptime((char*)string, "%Y-%j", time);
    if (offset && *offset == '\0')
        return IE_SUCCESS;

    return IE_NOTSUP;
}


/* Get data about logged in user and his box. */
isds_error isds_GetOwnerInfoFromLogin(struct isds_ctx *context,
        struct isds_DbOwnerInfo **db_owner_info) {
    isds_error err = IE_SUCCESS;
    xmlNsPtr isds_ns = NULL;
    xmlNodePtr request = NULL;
    xmlDocPtr response = NULL;
    xmlChar *code = NULL, *message = NULL;
    xmlNodePtr node;
    xmlXPathContextPtr xpath_ctx = NULL;
    xmlXPathObjectPtr result = NULL;
    char *string = NULL;

    if (!context) return IE_INVALID_CONTEXT;
    if (!db_owner_info) return IE_INVAL;

    /* Check if connection is established */
    if (!context->curl) return IE_CONNECTION_CLOSED;


    /* Build GetOwnerInfoFromLogin request */
    request = xmlNewNode(NULL, BAD_CAST "GetOwnerInfoFromLogin");
    if (!request) {
        isds_log_message(context,
                _("Could build GetOwnerInfoFromLogin request"));
        return IE_ERROR;
    }
    isds_ns = xmlNewNs(request, BAD_CAST ISDS_NS, NULL);
    if(!isds_ns) {
        isds_log_message(context, _("Could not create ISDS name space"));
        xmlFreeNode(request);
        return IE_ERROR;
    }
    xmlSetNs(request, isds_ns);
    node = xmlNewChild(request, NULL, BAD_CAST "dbDummy", NULL);
    if (!node) {
        isds_log_message(context, _("Could nod add dbDummy Child to "
                    "GetOwnerInfoFromLogin element"));
        xmlFreeNode(request);
        return IE_ERROR;
    }


    isds_log(ILF_ISDS, ILL_DEBUG,
            _("Sending GetOwnerInfoFromLogin request to ISDS\n"));

    /* Sent request */
    err = isds(context, SERVICE_DB_SUPPLEMENTARY, request, &response);
   
    /* Destroy request */
    xmlFreeNode(request);

    if (err) {
        isds_log(ILF_ISDS, ILL_DEBUG,
                _("Processing ISDS response on GetOwnerInfoFromLogin "
                    "request failed\n"));
        xmlFreeDoc(response);
        return err;
    }

    /* Check for response status */
    err = isds_response_status(context, SERVICE_DB_SUPPLEMENTARY, response,
            &code, &message, NULL);
    if (err) {
        isds_log(ILF_ISDS, ILL_DEBUG,
                _("ISDS response on GetOwnerInfoFromLogin request is "
                    "missing status\n"));
        free(code);
        free(message);
        xmlFreeDoc(response);
        return err;
    }
    if (xmlStrcmp(code, BAD_CAST "0000")) {
        /* FIXME: Convert UTF-8 into locale */
        isds_log(ILF_ISDS, ILL_DEBUG,
                _("Server refused GetOwnerInfoFromLogin request "
                    "(code=%s, message=%s)\n"), code, message);
        isds_log_message(context, _((char *)message));
        free(code);
        free(message);
        xmlFreeDoc(response);
        return IE_ISDS;
    }

    /* Extract data */
    /* Prepare stucture */
    isds_DbOwnerInfo_free(db_owner_info);
    *db_owner_info = calloc(1, sizeof(**db_owner_info));
    if (!*db_owner_info) {
        err = IE_NOMEM;
        goto leave;
    }
    xpath_ctx = xmlXPathNewContext(response);
    if (!xpath_ctx) {
        err = IE_ERROR;
        goto leave;
    }
    if (register_namespaces(xpath_ctx)) {
        err = IE_ERROR;
        goto leave;
    }

    /* Set context node */
    result = xmlXPathEvalExpression(BAD_CAST
            "/isds:GetOwnerInfoFromLoginResponse/isds:dbOwnerInfo", xpath_ctx);
    if (!result) {
        err = IE_ERROR;
        goto leave;
    }
    if (xmlXPathNodeSetIsEmpty(result->nodesetval)) {
        isds_log_message(context, _("Missing dbOwnerInfo element"));
        err = IE_ISDS;
        goto leave;
    }
    if (result->nodesetval->nodeNr > 1) {
        isds_log_message(context, _("Multiple dbOwnerInfo element"));
        err = IE_ISDS;
        goto leave;
    }
    xpath_ctx->node = result->nodesetval->nodeTab[0];
    xmlXPathFreeObject(result);

#define EXTRACT_STRING(element, string) \
    result = xmlXPathEvalExpression(BAD_CAST element "/text()", xpath_ctx); \
    if (!result) { \
        err = IE_ERROR; \
        goto leave; \
    } \
    if (!xmlXPathNodeSetIsEmpty(result->nodesetval)) { \
        if (result->nodesetval->nodeNr > 1) { \
            isds_log_message(context, _("Multiple " element " element")); \
            err = IE_ERROR; \
            goto leave; \
        } \
        (string) = (char *) \
            xmlXPathCastNodeSetToString(result->nodesetval); \
        if (!(string)) { \
            err = IE_ERROR; \
            goto leave; \
        } \
    }

    EXTRACT_STRING("isds:dbID", (*db_owner_info)->dbID);
    
    EXTRACT_STRING("isds:dbType", string);
    (*db_owner_info)->dbType = calloc(1, sizeof(*((*db_owner_info)->dbType)));
    if (!(*db_owner_info)->dbType) {
        err = IE_NOMEM;
        goto leave;
    }
    err = string2isds_DbType((xmlChar *)string, (*db_owner_info)->dbType);
    if (err) {
        free((*db_owner_info)->dbType);
        (*db_owner_info)->dbType = NULL;
        if (err == IE_ENUM) {
            err = IE_ISDS;
            isds_log_message(context, _("Unknown isds:dbType: "));
            isds_append_message(context, (char *)string);
        }
        goto leave;
    }
    free(string); string = NULL;

    EXTRACT_STRING("isds:ic", (*db_owner_info)->ic);

    (*db_owner_info)->personName =
        calloc(1, sizeof(*((*db_owner_info)->personName)));
    if (!(*db_owner_info)->personName) {
        err = IE_NOMEM;
        goto leave;
    }
    EXTRACT_STRING("isds:pnFirstName",
            (*db_owner_info)->personName->pnFirstName);
    EXTRACT_STRING("isds:pnMiddleName",
            (*db_owner_info)->personName->pnMiddleName);
    EXTRACT_STRING("isds:pnLastName",
            (*db_owner_info)->personName->pnLastName);
    EXTRACT_STRING("isds:pnLastNameAtBirth",
            (*db_owner_info)->personName->pnLastNameAtBirth);

    EXTRACT_STRING("isds:firmName", (*db_owner_info)->firmName);

    (*db_owner_info)->birthInfo =
        calloc(1, sizeof(*((*db_owner_info)->birthInfo)));
    if (!(*db_owner_info)->birthInfo) {
        err = IE_NOMEM;
        goto leave;
    }

    EXTRACT_STRING("isds:biDate", string);
    (*db_owner_info)->birthInfo->biDate =
        calloc(1, sizeof(*((*db_owner_info)->birthInfo->biDate)));
    if (!(*db_owner_info)->birthInfo->biDate) {
        err = IE_NOMEM;
        goto leave;
    }
    err = datestring2tm((xmlChar *)string,
            (*db_owner_info)->birthInfo->biDate);
    if (err) {
        free((*db_owner_info)->birthInfo->biDate);
        (*db_owner_info)->birthInfo->biDate = NULL;
        if (err == IE_NOTSUP) {
            err = IE_ISDS;
            isds_log_message(context, _("Invalid isds:biDate value: "));
            isds_append_message(context, (char *)string);
        }
        goto leave;
    }
    free(string); string = NULL;
    EXTRACT_STRING("isds:biCity", (*db_owner_info)->birthInfo->biCity);
    EXTRACT_STRING("isds:biCounty", (*db_owner_info)->birthInfo->biCounty);
    EXTRACT_STRING("isds:biState", (*db_owner_info)->birthInfo->biState);

    (*db_owner_info)->address =
        calloc(1, sizeof(*((*db_owner_info)->address)));
    if (!(*db_owner_info)->address) {
        err = IE_NOMEM;
        goto leave;
    }
    EXTRACT_STRING("isds:adCity",
            (*db_owner_info)->address->adCity);
    EXTRACT_STRING("isds:adStreet",
            (*db_owner_info)->address->adStreet);
    EXTRACT_STRING("isds:adNumberInStreet",
            (*db_owner_info)->address->adNumberInStreet);
    EXTRACT_STRING("isds:adNumberInMunicipality",
            (*db_owner_info)->address->adNumberInMunicipality);
    EXTRACT_STRING("isds:adZipCode",
            (*db_owner_info)->address->adZipCode);
    EXTRACT_STRING("isds:adState",
            (*db_owner_info)->address->adState);
#undef EXTRACT_STRING

leave:
    if (err) {
        isds_DbOwnerInfo_free(db_owner_info);
    }

    free(string);
    xmlXPathFreeObject(result);
    xmlXPathFreeContext(xpath_ctx);

    free(code);
    free(message);
    xmlFreeDoc(response);

    isds_log(ILF_ISDS, ILL_DEBUG,
            _("GetOwnerInfoFromLogin request processed by server "
                "successfully.\n"));

    return err;
}


/*int isds_get_message(struct isds_ctx *context, const unsigned int id,
        struct isds_message **message);
int isds_send_message(struct isds_ctx *context, struct isds_message *message);
int isds_list_messages(struct isds_ctx *context, struct isds_message **message);
int isds_find_recipient(struct isds_ctx *context, const struct address *pattern,
        struct isds_address **address);

int isds_message_free(struct isds_message **message);
int isds_address_free(struct isds_address **address);
*/


/* Deallocate structure isds_PersonName recursively and NULL it */
static void isds_PersonName_free(struct isds_PersonName **person_name) {
    if (!person_name || !*person_name) return;

    free((*person_name)->pnFirstName);
    free((*person_name)->pnMiddleName);
    free((*person_name)->pnLastName);
    free((*person_name)->pnLastNameAtBirth);
    
    *person_name = NULL;
}


/* Deallocate structure isds_BirthInfo recursively and NULL it */
static void isds_BirthInfo_free(struct isds_BirthInfo **birth_info) {
    if (!birth_info || !*birth_info) return;

    free((*birth_info)->biDate);
    free((*birth_info)->biCity);
    free((*birth_info)->biCounty);
    free((*birth_info)->biState);
    
    *birth_info = NULL;
}


/* Deallocate structure isds_Address recursively and NULL it */
static void isds_Address_free(struct isds_Address **address) {
    if (!address || !*address) return;

    free((*address)->adCity);
    free((*address)->adStreet);
    free((*address)->adNumberInStreet);
    free((*address)->adNumberInMunicipality);
    free((*address)->adZipCode);
    free((*address)->adState);
    
    *address = NULL;
}


/* Deallocate structure isds_DbOwnerInfo recursively and NULL it */
void isds_DbOwnerInfo_free(struct isds_DbOwnerInfo **db_owner_info) {
    if (!db_owner_info || !*db_owner_info) return;

    free((*db_owner_info)->dbID);
    free((*db_owner_info)->dbType);
    free((*db_owner_info)->ic);
    isds_PersonName_free(&((*db_owner_info)->personName));
    free((*db_owner_info)->firmName);
    isds_BirthInfo_free(&((*db_owner_info)->birthInfo));
    isds_Address_free(&((*db_owner_info)->address));
    free((*db_owner_info)->nationality);
    free((*db_owner_info)->email);
    free((*db_owner_info)->telNumber);
    free((*db_owner_info)->identifier);
    free((*db_owner_info)->registryCode);
    free((*db_owner_info)->dbState);
    free((*db_owner_info)->dbEffectiveOVM);
    
    free(*db_owner_info);
    *db_owner_info = NULL;
}


/* Makes known all relevant namespaces to give @xpat_ctx */
_hidden isds_error register_namespaces(xmlXPathContextPtr xpath_ctx) {
    if (!xpath_ctx) return IE_ERROR;

    if (xmlXPathRegisterNs(xpath_ctx, BAD_CAST "soap", BAD_CAST SOAP_NS))
        return IE_ERROR;
    if (xmlXPathRegisterNs(xpath_ctx, BAD_CAST "isds", BAD_CAST ISDS_NS))
        return IE_ERROR;
    if (xmlXPathRegisterNs(xpath_ctx, BAD_CAST "xs", BAD_CAST SCHEMA_NS))
        return IE_ERROR;
    return IE_SUCCESS;
}

