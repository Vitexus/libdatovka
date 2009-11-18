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


/* Free isds_list with all member data.
 * @list list to free, on return will be NULL */
void isds_list_free(struct isds_list **list) {
    struct isds_list *item, *next_item;

    if (!list || !*list) return;
    
    for(item = *list; item; item = next_item) {
        (item->destructor)(&(item->data));
        next_item = item->next;
        free(item);
    }

    *list = NULL;
}


/* Deallocate structure isds_PersonName recursively and NULL it */
static void isds_PersonName_free(struct isds_PersonName **person_name) {
    if (!person_name || !*person_name) return;

    free((*person_name)->pnFirstName);
    free((*person_name)->pnMiddleName);
    free((*person_name)->pnLastName);
    free((*person_name)->pnLastNameAtBirth);
   
    free(*person_name);
    *person_name = NULL;
}


/* Deallocate structure isds_BirthInfo recursively and NULL it */
static void isds_BirthInfo_free(struct isds_BirthInfo **birth_info) {
    if (!birth_info || !*birth_info) return;

    free((*birth_info)->biDate);
    free((*birth_info)->biCity);
    free((*birth_info)->biCounty);
    free((*birth_info)->biState);
    
    free(*birth_info);
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
    
    free(*address);
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


/* Deallocate struct isds_envelope recurisvely and NULL it */
void isds_envelope_free(struct isds_envelope **envelope) {
    if (!envelope || !*envelope) return;

    free((*envelope)->dmID);
    free((*envelope)->dbIDSender);
    free((*envelope)->dmSender);
    free((*envelope)->dmSenderAddress);
    free((*envelope)->dmSenderType);
    free((*envelope)->dmRecipient);
    free((*envelope)->dmRecipientAddress);
    free((*envelope)->dmAmbiguousRecipient);

    free((*envelope)->dmSenderOrgUnit);
    free((*envelope)->dmSenderOrgUnitNum);
    free((*envelope)->dbIDRecipient);
    free((*envelope)->dmRecipientOrgUnit);
    free((*envelope)->dmRecipientOrgUnitNum);
    free((*envelope)->dmToHands);
    free((*envelope)->dmAnnotation);
    free((*envelope)->dmRecipientRefNumber);
    free((*envelope)->dmSenderRefNumber);
    free((*envelope)->dmRecipientIdent);
    free((*envelope)->dmSenderIdent);

    free((*envelope)->dmLegalTitleLaw);
    free((*envelope)->dmLegalTitleYear);
    free((*envelope)->dmLegalTitleSect);
    free((*envelope)->dmLegalTitlePar);
    free((*envelope)->dmLegalTitlePoint);

    free((*envelope)->dmPersonalDelivery);
    free((*envelope)->dmAllowSubstDelivery);
    free((*envelope)->dmOVM);

    free(*envelope);
    *envelope = NULL;
}


/* Deallocate struct isds_message recurisvely and NULL it */
void isds_message_free(struct isds_message **message) {
    if (!message || !*message) return;

    free((*message)->raw);
    isds_envelope_free(&((*message)->envelope));
    isds_list_free(&((*message)->documents));

    free(*message);
    *message = NULL;
}


/* Deallocate struct isds_document recurisvely and NULL it */
void isds_document_free(struct isds_document **document) {
    if (!document || !*document) return;

    free((*document)->data);
    free((*document)->dmMimeType);
    free((*document)->dmFileGuid);
    free((*document)->dmUpFileGuid);
    free((*document)->dmFileDescr);
    free((*document)->dmFormat);
    
    free(*document);
    *document = NULL;
}


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
        case IE_2BIG:
            return(_("Too big")); break;
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
 * supplied. Return string is locale encoded. */
char *isds_long_message(const struct isds_ctx *context) {
    if (!context) return NULL;
    return context->long_message;
}


/* Stores message into context' long_message buffer.
 * Application can pick the message up using isds_long_message().
 * NULL @message truncates the buffer but does not deallocate it.
 * @message is coded in locale encoding */
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


/* Stores formated message into context' long_message buffer.
 * Application can pick the message up using isds_long_message(). */
_hidden isds_error isds_printf_message(struct isds_ctx *context,
        const char *format, ...) {
    va_list ap;
    int length;
    
    if (!context) return IE_INVALID_CONTEXT;
    va_start(ap, format);
    length = isds_vasprintf(&(context->long_message), format, ap);
    va_end(ap);
        
    return (length < 0) ? IE_ERROR: IE_SUCCESS;
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
    if (!context->curl) {
        /* Testing printf message */
        isds_printf_message(context, "%s", _("I said connection closed"));
        return IE_CONNECTION_CLOSED;
    }


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
        char *code_locale = utf82locale((char*)code);
        char *message_locale = utf82locale((char*)message);
        isds_log(ILF_ISDS, ILL_DEBUG,
                _("Server refused bogus request (code=%s, message=%s)\n"),
                code_locale, message_locale);
        /* XXX: Literal error messages from ISDS are Czech mesages
         * (English sometimes) in UTF-8. It's hard to catch them for
         * translation. Successfully gettextized would return in locale
         * encoding, unsuccessfully translated would pass in UTF-8. */
        isds_log_message(context, message_locale);
        free(code_locale);
        free(message_locale);
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


/* Convert ISDS dbType enum @type to UTF-8 string.
 * @Return pointer to static string, or NULL if unkwnow enum value */
static const xmlChar *isds_DbType2string(const isds_DbType type) {
     switch(type) {
            case DBTYPE_FO: return(BAD_CAST "FO"); break;
            case DBTYPE_PFO: return(BAD_CAST "PFO"); break;
            case DBTYPE_PFO_ADVOK: return(BAD_CAST "PFO_ADVOK"); break;
            case DBTYPE_PFO_DANPOR: return(BAD_CAST "PFO_DAPOR"); break;
            case DBTYPE_PFO_INSSPR: return(BAD_CAST "PFO_INSSPR"); break;
            case DBTYPE_PO: return(BAD_CAST "PO"); break;
            case DBTYPE_PO_ZAK: return(BAD_CAST "PO_ZAK"); break;
            case DBTYPE_PO_REQ: return(BAD_CAST "PO_REQ"); break;
            case DBTYPE_OVM: return(BAD_CAST "OVM"); break;
            case DBTYPE_OVM_NOTAR: return(BAD_CAST "OVM_NOTAR"); break;
            case DBTYPE_OVM_EXEKUT: return(BAD_CAST "OVM_EXEKUT"); break;
            case DBTYPE_OVM_REQ: return(BAD_CAST "OVM_REQ"); break;
            default: return NULL; break;
        }
}


/* Convert ISDS dmFileMetaType enum @type to UTF-8 string.
 * @Return pointer to static string, or NULL if unkwnow enum value */
static const xmlChar *isds_FileMetaType2string(const isds_FileMetaType type) {
     switch(type) {
            case FILEMETATYPE_MAIN: return(BAD_CAST "main"); break;
            case FILEMETATYPE_ENCLOSURE: return(BAD_CAST "enclosure"); break;
            case FILEMETATYPE_SIGNATURE: return(BAD_CAST "signature"); break;
            case FILEMETATYPE_META: return(BAD_CAST "meta"); break;
            default: return NULL; break;
        }
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

/* Convert struct tm *@time to UTF-8 ISO 8601 date @string. */
static isds_error tm2datestring(struct tm *time, xmlChar **string) {
    if (!time || !string) return IE_INVAL;

    if (-1 == isds_asprintf((char **) string, "%d-%02d-%02d",
                time->tm_year + 1900, time->tm_mon + 1, time->tm_mday))
        return IE_ERROR;

    return IE_SUCCESS;
}


/* Following EXTRACT_* macros expects @result, @xpath_ctx, @err, @context
 * and leave lable */
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

#define EXTRACT_BOOLEAN(element, booleanPtr) \
    { \
        char *string = NULL; \
        EXTRACT_STRING(element, string); \
         \
        if (string) { \
            (booleanPtr) = calloc(1, sizeof(*(booleanPtr))); \
            if (!(booleanPtr)) { \
                free(string); \
                err = IE_NOMEM; \
                goto leave; \
            } \
             \
            if (!xmlStrcmp((xmlChar *)string, BAD_CAST "true") || \
                    !xmlStrcmp((xmlChar *)string, BAD_CAST "1")) \
                *(booleanPtr) = 1; \
            else if (!xmlStrcmp((xmlChar *)string, BAD_CAST "false") || \
                    !xmlStrcmp((xmlChar *)string, BAD_CAST "0")) \
                *(booleanPtr) = 0; \
            else { \
                char *string_locale = utf82locale((char*)string); \
                isds_printf_message(context, \
                        _(element " value is not valid boolean: "), \
                        string_locale); \
                free(string_locale); \
                free(string); \
                err = IE_ERROR; \
                goto leave; \
            } \
             \
            free(string); \
        } \
    } 

#define EXTRACT_LONGINT(element, longintPtr, preallocated) \
    { \
        char *string = NULL; \
        EXTRACT_STRING(element, string); \
        if (string) { \
            long int number; \
            char *endptr; \
             \
            number = strtol((char*)string, &endptr, 10); \
             \
            if (*endptr != '\0') { \
                char *string_locale = utf82locale((char *)string); \
                isds_printf_message(context, \
                        _(element" is not valid integer: %s"), \
                        string_locale); \
                free(string_locale); \
                err = IE_ISDS; \
                goto leave; \
            } \
             \
            if (number == LONG_MIN || number == LONG_MAX) { \
                char *string_locale = utf82locale((char *)string); \
                isds_printf_message(context, \
                        _(element " value out of range of long int: %s"), \
                        string_locale); \
                free(string_locale); \
                err = IE_ERROR; \
                goto leave; \
            } \
             \
            free(string); string = NULL; \
             \
            if (!(preallocated)) { \
                (longintPtr) = calloc(1, sizeof(*(longintPtr))); \
                if (!(longintPtr)) { \
                    err = IE_NOMEM; \
                    goto leave; \
                } \
            } \
            *(longintPtr) = number; \
        } \
    }

#define INSERT_STRING(parent, element, string) \
    node = xmlNewTextChild(parent, NULL, BAD_CAST (element), \
            (xmlChar *) (string)); \
    if (!node) { \
        isds_printf_message(context, _("Could not add " element " child to " \
                    "%s element"), (parent)->name); \
        err = IE_ERROR; \
        goto leave; \
    }

#define INSERT_BOOLEAN(parent, element, booleanPtr) \
    if ((booleanPtr)) { \
        if (*(booleanPtr)) { INSERT_STRING(parent, element, "true"); } \
        else { INSERT_STRING(parent, element, "false") } \
    } else { INSERT_STRING(parent, element, NULL) }

#define INSERT_LONGINT(parent, element, longintPtr, buffer) \
    if ((longintPtr)) { \
        /* FIXME: locale sensitive */ \
        if (-1 == isds_asprintf((char **)&(buffer), "%ld", *(longintPtr))) { \
            err = IE_NOMEM; \
            goto leave; \
        } \
        INSERT_STRING(parent, element, buffer) \
        free(buffer); (buffer) = NULL; \
    } else { INSERT_STRING(parent, element, NULL) }

#define INSERT_STRING_ATTRIBUTE(parent, attribute, string) \
    attribute_node = xmlNewProp((parent), BAD_CAST (attribute), \
            (xmlChar *) (string)); \
    if (!attribute_node) { \
        isds_printf_message(context, _("Could not add " attribute \
                    " attribute to %s element"), (parent)->name); \
        err = IE_ERROR; \
        goto leave; \
    }


/* Convert isds:dBOwnerInfo XML tree into structure
 * @context is ISDS context
 * @db_owner_info is automically reallocated box owner info structure
 * @xpath_ctx is XPath context with current node as isds:dBOwnerInfo element
 * In case of error @db_owner_info will be freed. */
static isds_error extract_DbOwnerInfo(struct isds_ctx *context,
        struct isds_DbOwnerInfo **db_owner_info,
        xmlXPathContextPtr xpath_ctx) {
    isds_error err = IE_SUCCESS;
    xmlXPathObjectPtr result = NULL;
    char *string = NULL;

    if (!context) return IE_INVALID_CONTEXT;
    if (!db_owner_info) return IE_INVAL;
    isds_DbOwnerInfo_free(db_owner_info);
    if (!xpath_ctx) return IE_INVAL;


    *db_owner_info = calloc(1, sizeof(**db_owner_info));
    if (!*db_owner_info) {
        err = IE_NOMEM;
        goto leave;
    }

    EXTRACT_STRING("isds:dbID", (*db_owner_info)->dbID);
    
    EXTRACT_STRING("isds:dbType", string);
    if (string) {
        (*db_owner_info)->dbType =
            calloc(1, sizeof(*((*db_owner_info)->dbType)));
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
                isds_printf_message(context, _("Unknown isds:dbType: %s"), 
                    (char *)string);
            }
            goto leave;
        }
        free(string); string = NULL;
    }

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
    if (!(*db_owner_info)->personName->pnFirstName &&
            !(*db_owner_info)->personName->pnMiddleName &&
            !(*db_owner_info)->personName->pnLastName &&
            !(*db_owner_info)->personName->pnLastNameAtBirth)
        isds_PersonName_free(&(*db_owner_info)->personName);

    EXTRACT_STRING("isds:firmName", (*db_owner_info)->firmName);

    (*db_owner_info)->birthInfo =
        calloc(1, sizeof(*((*db_owner_info)->birthInfo)));
    if (!(*db_owner_info)->birthInfo) {
        err = IE_NOMEM;
        goto leave;
    }
    EXTRACT_STRING("isds:biDate", string);
    if (string) {
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
                isds_printf_message(context,
                        _("Invalid isds:biDate value: %s"), (char *)string);
            }
            goto leave;
        }
        free(string); string = NULL;
    }
    EXTRACT_STRING("isds:biCity", (*db_owner_info)->birthInfo->biCity);
    EXTRACT_STRING("isds:biCounty", (*db_owner_info)->birthInfo->biCounty);
    EXTRACT_STRING("isds:biState", (*db_owner_info)->birthInfo->biState);
    if (!(*db_owner_info)->birthInfo->biDate &&
            !(*db_owner_info)->birthInfo->biCity &&
            !(*db_owner_info)->birthInfo->biCounty &&
            !(*db_owner_info)->birthInfo->biState)
        isds_BirthInfo_free(&(*db_owner_info)->birthInfo);

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
    if (!(*db_owner_info)->address->adCity &&
            !(*db_owner_info)->address->adStreet &&
            !(*db_owner_info)->address->adNumberInStreet &&
            !(*db_owner_info)->address->adNumberInMunicipality &&
            !(*db_owner_info)->address->adZipCode &&
            !(*db_owner_info)->address->adState)
        isds_Address_free(&(*db_owner_info)->address);

    EXTRACT_STRING("isds:nationality", (*db_owner_info)->nationality);
    EXTRACT_STRING("isds:email", (*db_owner_info)->email);
    EXTRACT_STRING("isds:telNumber", (*db_owner_info)->telNumber);
    EXTRACT_STRING("isds:identifier", (*db_owner_info)->identifier);
    EXTRACT_STRING("isds:registryCode", (*db_owner_info)->registryCode);
    
    EXTRACT_LONGINT("isds:dbState", (*db_owner_info)->dbState, 0);

    EXTRACT_BOOLEAN("isds:dbEffectiveOVM", (*db_owner_info)->dbEffectiveOVM);
    EXTRACT_BOOLEAN("isds:dbOpenAddressing",
            (*db_owner_info)->dbOpenAddressing);

leave:
    if (err) isds_DbOwnerInfo_free(db_owner_info);
    free(string);
    xmlXPathFreeObject(result);
    return err;
}


/* Convert isds_document structure into XML tree and append to dmFiles node.
 * @context is session context
 * @document is ISDS document
 * @dm_files is XML element the resulting tree will be appended to as a child.
 * @return error code, in case of error context' message is filled. */
static isds_error insert_document(struct isds_ctx *context,
        struct isds_document *document, xmlNodePtr dm_files) {
    isds_error err = IE_SUCCESS;
    xmlNodePtr file = NULL, node;
    xmlAttrPtr attribute_node;
    xmlChar *base64data = NULL;

    if (!context) return IE_INVALID_CONTEXT;
    if (!document || !dm_files) return IE_INVAL;

    
    file = xmlNewChild(dm_files, NULL, BAD_CAST "dmFile", NULL);
    if (!file) {
        isds_printf_message(context, _("Could not add dmFile child to "
                    "%s element"), dm_files->name);
        err = IE_ERROR;
        goto leave;
    }

    /* @dmMimeType is required */
    if (!document->dmMimeType) {
        isds_log_message(context,
                _("Document is missing mandatory MIME type definition"));
        err = IE_INVAL;
        goto leave;
    }
    INSERT_STRING_ATTRIBUTE(file, "dmMimeType", document->dmMimeType);

    const xmlChar *string = isds_FileMetaType2string(document->dmFileMetaType);
    if (!string) {
        isds_printf_message(context,
                _("Document has unkown dmFileMetaType: %ld"),
                document->dmFileMetaType);
        err = IE_ENUM;
        goto leave;
    }
    INSERT_STRING_ATTRIBUTE(file, "dmFileMetaType", string);

    if (document->dmFileGuid) {
        INSERT_STRING_ATTRIBUTE(file, "dmFileGuid", document->dmFileGuid);
    }
    if (document->dmUpFileGuid) {
        INSERT_STRING_ATTRIBUTE(file, "dmUpFileGuid", document->dmUpFileGuid);
    }

    /* @dmFileDescr is required */
    if (!document->dmFileDescr) {
        isds_log_message(context,
                _("Document is missing mandatory description (title)"));
        err = IE_INVAL;
        goto leave;
    }
    INSERT_STRING_ATTRIBUTE(file, "dmFileDescr", document->dmFileDescr);

    if (document->dmFormat) {
        INSERT_STRING_ATTRIBUTE(file, "dmFormat", document->dmFormat);
    }


    /* Insert content (data) of the document. */
    /* XXX; Only base64 is implemented currently. */
    base64data = (xmlChar *) b64encode(document->data, document->data_length);
    if (!base64data) {
        isds_printf_message(context,
                _("Not enought memory to encode %zd bytes into Base64"),
                document->data_length);
        err = IE_NOMEM;
        goto leave;
    }
    INSERT_STRING(file, "dmEncodedContent", base64data)

leave:
    return err;
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
        char *code_locale = utf82locale((char*)code);
        char *message_locale = utf82locale((char*)message);
        isds_log(ILF_ISDS, ILL_DEBUG,
                _("Server refused GetOwnerInfoFromLogin request "
                    "(code=%s, message=%s)\n"), code_locale, message_locale);
        isds_log_message(context, message_locale);
        free(code_locale);
        free(message_locale);
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
    xmlXPathFreeObject(result); result = NULL;

    /* Extract it */
    err = extract_DbOwnerInfo(context, db_owner_info, xpath_ctx);

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

    if (!err)
        isds_log(ILF_ISDS, ILL_DEBUG,
                _("GetOwnerInfoFromLogin request processed by server "
                    "successfully.\n"));

    return err;
}


/* Find boxes suiting given criteria.
 * @criteria is filter. You should fill in at least some memebers.
 * @boxes is automatically reallocated list of isds_DbOwnerInfo structures,
 * possibly empty. Input NULL or valid old structure.
 * @return:
 *  IE_SUCCESS if search sucseeded, @boxes contains usefull data
 *  IE_NOEXIST if no such box exists, @boxes will be NULL
 *  IE_2BIG if too much boxes exist and server truncated the resuluts, @boxes
 *      contains still valid data
 *  other code if something bad happens. @boxes will be NULL. */
isds_error isds_FindDataBox(struct isds_ctx *context,
        const struct isds_DbOwnerInfo *criteria,
        struct isds_list **boxes) {
    isds_error err = IE_SUCCESS;
    _Bool truncated = 0;
    xmlNsPtr isds_ns = NULL;
    xmlNodePtr request = NULL;
    xmlDocPtr response = NULL;
    xmlChar *code = NULL, *message = NULL;
    xmlNodePtr db_owner_info, node;
    xmlXPathContextPtr xpath_ctx = NULL;
    xmlXPathObjectPtr result = NULL;
    xmlChar *string = NULL;


    if (!context) return IE_INVALID_CONTEXT;
    if (!boxes) return IE_INVAL;
    isds_list_free(boxes);

    if (!criteria) {
        return IE_INVAL;
    }

    /* Check if connection is established
     * TODO: This check should be done donwstairs. */
    if (!context->curl) return IE_CONNECTION_CLOSED;


    /* Build FindDataBox request */
    request = xmlNewNode(NULL, BAD_CAST "FindDataBox");
    if (!request) {
        isds_log_message(context,
                _("Could build FindDataBox request"));
        return IE_ERROR;
    }
    isds_ns = xmlNewNs(request, BAD_CAST ISDS_NS, NULL);
    if(!isds_ns) {
        isds_log_message(context, _("Could not create ISDS name space"));
        xmlFreeNode(request);
        return IE_ERROR;
    }
    xmlSetNs(request, isds_ns);
    db_owner_info = xmlNewChild(request, NULL, BAD_CAST "dbOwnerInfo", NULL);
    if (!db_owner_info) {
        isds_log_message(context, _("Could not add dbOwnerInfo Child to "
                    "FindDataBox element"));
        xmlFreeNode(request);
        return IE_ERROR;
    }


    INSERT_STRING(db_owner_info, "dbID", criteria->dbID);

    /* dbType */
    if (criteria->dbType) {
        const xmlChar *type_string = isds_DbType2string(*(criteria->dbType));
        if (!type_string) {
            isds_printf_message(context, _("Invalid dbType value: %d"),
                    *(criteria->dbType));
            err = IE_ENUM;
            goto leave;
        }
        INSERT_STRING(db_owner_info, "dbType", type_string);
    }

    INSERT_STRING(db_owner_info, "firmName", criteria->firmName);
    INSERT_STRING(db_owner_info, "ic", criteria->ic);
    if (criteria->personName) {
        INSERT_STRING(db_owner_info, "pnFirstName",
                criteria->personName->pnFirstName);
        INSERT_STRING(db_owner_info, "pnMiddleName",
                criteria->personName->pnMiddleName);
        INSERT_STRING(db_owner_info, "pnLastName",
                criteria->personName->pnLastName);
        INSERT_STRING(db_owner_info, "pnLastNameAtBirth",
                criteria->personName->pnLastNameAtBirth);
    }
    if (criteria->birthInfo) {
        if (criteria->birthInfo->biDate) {
            if (!tm2datestring(criteria->birthInfo->biDate, &string))
                INSERT_STRING(db_owner_info, "biDate", string);
            free(string); string = NULL;
        }
        INSERT_STRING(db_owner_info, "biCity", criteria->birthInfo->biCity);
        INSERT_STRING(db_owner_info, "biCounty", criteria->birthInfo->biCounty);
        INSERT_STRING(db_owner_info, "biState", criteria->birthInfo->biState);
    }
    if (criteria->address) {
        INSERT_STRING(db_owner_info, "adCity", criteria->address->adCity);
        INSERT_STRING(db_owner_info, "adStreet", criteria->address->adStreet);
        INSERT_STRING(db_owner_info, "adNumberInStreet",
                criteria->address->adNumberInStreet);
        INSERT_STRING(db_owner_info, "adNumberInMunicipality",
                criteria->address->adNumberInMunicipality);
        INSERT_STRING(db_owner_info, "adZipCode", criteria->address->adZipCode);
        INSERT_STRING(db_owner_info, "adState", criteria->address->adState);
    }
    INSERT_STRING(db_owner_info, "nationality", criteria->nationality);
    INSERT_STRING(db_owner_info, "email", criteria->email);
    INSERT_STRING(db_owner_info, "telNumber", criteria->telNumber);
    INSERT_STRING(db_owner_info, "identifier", criteria->identifier);
    INSERT_STRING(db_owner_info, "registryCode", criteria->registryCode);

    INSERT_LONGINT(db_owner_info, "dbState", criteria->dbState, string);

    INSERT_BOOLEAN(db_owner_info, "dbEffectiveOVM", criteria->dbEffectiveOVM);
    INSERT_BOOLEAN(db_owner_info, "dbOpenAddressing",
            criteria->dbOpenAddressing);


    isds_log(ILF_ISDS, ILL_DEBUG, _("Sending FindDataBox request to ISDS\n"));

    /* Sent request */
    err = isds(context, SERVICE_DB_SEARCH, request, &response);
   
    /* Destroy request */
    xmlFreeNode(request); request = NULL;

    if (err) {
        isds_log(ILF_ISDS, ILL_DEBUG,
                _("Processing ISDS response on FindDataBox "
                    "request failed\n"));
        goto leave;
    }

    /* Check for response status */
    err = isds_response_status(context, SERVICE_DB_SEARCH, response,
            &code, &message, NULL);
    if (err) {
        isds_log(ILF_ISDS, ILL_DEBUG,
                _("ISDS response on FindDataBox request is missing status\n"));
        goto leave;
    }

    /* Request processed, but nothing found */
    if (!xmlStrcmp(code, BAD_CAST "0002") ||
            !xmlStrcmp(code, BAD_CAST "5001")) {
        char *code_locale = utf82locale((char*)code);
        char *message_locale = utf82locale((char*)message);
        isds_log(ILF_ISDS, ILL_DEBUG,
                _("Server did not found any box on FindDataBox request "
                    "(code=%s, message=%s)\n"), code_locale, message_locale);
        isds_log_message(context, message_locale);
        free(code_locale);
        free(message_locale);
        err = IE_NOEXIST;
        goto leave;
    }

    /* Warning, not a error */
    if (!xmlStrcmp(code, BAD_CAST "0003")) {
        char *code_locale = utf82locale((char*)code);
        char *message_locale = utf82locale((char*)message);
        isds_log(ILF_ISDS, ILL_DEBUG,
                _("Server truncated response on FindDataBox request "
                    "(code=%s, message=%s)\n"), code_locale, message_locale);
        isds_log_message(context, message_locale);
        free(code_locale);
        free(message_locale);
        truncated = 1;
    } 
    
    /* Other error */
    else if (xmlStrcmp(code, BAD_CAST "0000")) {
        char *code_locale = utf82locale((char*)code);
        char *message_locale = utf82locale((char*)message);
        isds_log(ILF_ISDS, ILL_DEBUG,
                _("Server refused FindDataBox request "
                    "(code=%s, message=%s)\n"), code_locale, message_locale);
        isds_log_message(context, message_locale);
        free(code_locale);
        free(message_locale);
        err = IE_ISDS;
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

    /* Extract boxes if they present */
    result = xmlXPathEvalExpression(BAD_CAST
            "/isds:FindDataBoxResponse/isds:dbResults/isds:dbOwnerInfo",
            xpath_ctx);
    if (!result) {
        err = IE_ERROR;
        goto leave;
    }
    if (!xmlXPathNodeSetIsEmpty(result->nodesetval)) {
        struct isds_list *item, *prev_item = NULL;
        for (int i = 0; i < result->nodesetval->nodeNr; i++) {
            item = calloc(1, sizeof(*item));
            if (!item) {
                err = IE_NOMEM;
                goto leave;
            }
           
            item->destructor = (void (*)(void **))isds_DbOwnerInfo_free;
            if (i == 0) *boxes = item;
            else prev_item->next = item;
            prev_item = item;

            xpath_ctx->node = result->nodesetval->nodeTab[i];
            err = extract_DbOwnerInfo(context,
                    (struct isds_DbOwnerInfo **) &(item->data), xpath_ctx);
            if (err) goto leave;
        }
    }

leave:
    if (err) {
        isds_list_free(boxes);
    } else {
        if (truncated) err = IE_2BIG;
    }

    free(string);
    xmlFreeNode(request);
    xmlXPathFreeObject(result);
    xmlXPathFreeContext(xpath_ctx);

    free(code);
    free(message);
    xmlFreeDoc(response);

    if (!err)
        isds_log(ILF_ISDS, ILL_DEBUG,
                _("FindDataBox request processed by server successfully.\n"));

    return err;
}


/* Get status of a box.
 * @context is ISDS session context.
 * @box_id is UTF-8 encoded box identifier as zero terminated string
 * @box_status is return value of box status.
 * @return:
 *  IE_SUCCESS if box has been found and its status retrieved
 *  IE_NOEXIST if box is not known to ISDS server
 *  or other appropriate error.
 *  You can use isds_DbState to enumerate box status. However out of enum
 *  range value can be returned too. This is feature because ISDS
 *  specification leaves the set of values open.
 *  Be ware that status DBSTATE_REMOVED is signaled as IE_SUCCESS. That means
 *  the box has been deleted, but ISDS still lists its former existence. */
isds_error isds_CheckDataBox(struct isds_ctx *context, const char *box_id,
        long int *box_status) {
    isds_error err = IE_SUCCESS;
    xmlNsPtr isds_ns = NULL;
    xmlNodePtr request = NULL, db_id;
    xmlDocPtr response = NULL;
    xmlChar *code = NULL, *message = NULL;
    xmlXPathContextPtr xpath_ctx = NULL;
    xmlXPathObjectPtr result = NULL;
    xmlChar *string = NULL;

    if (!context) return IE_INVALID_CONTEXT;
    if (!box_status || !box_id || *box_id == '\0') return IE_INVAL;

    /* Check if connection is established
     * TODO: This check should be done donwstairs. */
    if (!context->curl) return IE_CONNECTION_CLOSED;


    /* Build CheckDataBox request */
    request = xmlNewNode(NULL, BAD_CAST "CheckDataBox");
    if (!request) {
        isds_log_message(context,
                _("Could build CheckDataBox request"));
        return IE_ERROR;
    }
    isds_ns = xmlNewNs(request, BAD_CAST ISDS_NS, NULL);
    if(!isds_ns) {
        isds_log_message(context, _("Could not create ISDS name space"));
        xmlFreeNode(request);
        return IE_ERROR;
    }
    xmlSetNs(request, isds_ns);
    db_id = xmlNewTextChild(request, NULL, BAD_CAST "dbID", (xmlChar *) box_id);
    if (!db_id) {
        isds_log_message(context, _("Could not add dbId Child to "
                    "CheckDataBox element"));
        xmlFreeNode(request);
        return IE_ERROR;
    }


    isds_log(ILF_ISDS, ILL_DEBUG, _("Sending CheckDataBox request to ISDS\n"));

    /* Sent request */
    err = isds(context, SERVICE_DB_SEARCH, request, &response);
   
    /* Destroy request */
    xmlFreeNode(request);

    if (err) {
        isds_log(ILF_ISDS, ILL_DEBUG,
                _("Processing ISDS response on CheckDataBox "
                    "request failed\n"));
        goto leave;
    }

    /* Check for response status */
    err = isds_response_status(context, SERVICE_DB_SEARCH, response,
            &code, &message, NULL);
    if (err) {
        isds_log(ILF_ISDS, ILL_DEBUG,
                _("ISDS response on CheckDataBox request is missing status\n"));
        goto leave;
    }

    /* Request processed, but nothing found */
    if (!xmlStrcmp(code, BAD_CAST "5001")) {
        char *box_id_locale = utf82locale((char*)box_id);
        char *code_locale = utf82locale((char*)code);
        char *message_locale = utf82locale((char*)message);
        isds_log(ILF_ISDS, ILL_DEBUG,
                _("Server did not found box %s on CheckDataBox request "
                    "(code=%s, message=%s)\n"),
                box_id_locale, code_locale, message_locale);
        isds_log_message(context, message_locale);
        free(box_id_locale);
        free(code_locale);
        free(message_locale);
        err = IE_NOEXIST;
        goto leave;
    }

    /* Other error */
    else if (xmlStrcmp(code, BAD_CAST "0000")) {
        char *code_locale = utf82locale((char*)code);
        char *message_locale = utf82locale((char*)message);
        isds_log(ILF_ISDS, ILL_DEBUG,
                _("Server refused CheckDataBox request "
                    "(code=%s, message=%s)\n"), code_locale, message_locale);
        isds_log_message(context, message_locale);
        free(code_locale);
        free(message_locale);
        err = IE_ISDS;
        goto leave;
    }

    /* Extract data */
    xpath_ctx = xmlXPathNewContext(response);
    if (!xpath_ctx) {
        err = IE_ERROR;
        goto leave;
    }
    if (register_namespaces(xpath_ctx)) {
        err = IE_ERROR;
        goto leave;
    }
    result = xmlXPathEvalExpression(BAD_CAST "/isds:CheckDataBoxResponse",
            xpath_ctx);
    if (!result) {
        err = IE_ERROR;
        goto leave;
    }
    if (xmlXPathNodeSetIsEmpty(result->nodesetval)) {
        isds_log_message(context, _("Missing CheckDataBoxResponse element"));
        err = IE_ISDS;
        goto leave;
    }
    if (result->nodesetval->nodeNr > 1) {
        isds_log_message(context, _("Multiple CheckDataBoxResponse element"));
        err = IE_ISDS;
        goto leave;
    }
    xpath_ctx->node = result->nodesetval->nodeTab[0];
    xmlXPathFreeObject(result); result = NULL;

    EXTRACT_LONGINT("isds:dbState", box_status, 1); 


leave:
    free(string);
    xmlXPathFreeObject(result);
    xmlXPathFreeContext(xpath_ctx);

    free(code);
    free(message);
    xmlFreeDoc(response);

    if (!err)
        isds_log(ILF_ISDS, ILL_DEBUG,
                _("CheckDataBox request processed by server successfully.\n"));

    return err;
}


/* Send a message via ISDS to a recipent
 * @context is session context
 * @outgoing_message is message to send; Some memebers are mandatory (like
 * dbIDRecipient), some are optional and some are irrelevant (especialy data
 * about sender). Included pointer to isds_list documents must contain at
 * least one document of FILEMETATYPE_MAIN. This is read-write structure, some
 * members will be filled with valid data from ISDS. Exact list of write
 * members is subject to change. Currently dmId is changed.
 * @return ISDS_SUCCESS, or other error code if something goes wrong. */
isds_error isds_send_message(struct isds_ctx *context,
        struct isds_message *outgoing_message) {

    isds_error err = IE_SUCCESS;
    xmlNsPtr isds_ns = NULL;
    xmlNodePtr request = NULL, envelope, dm_files, node;
    xmlDocPtr response = NULL;
    xmlChar *code = NULL, *message = NULL;
    xmlXPathContextPtr xpath_ctx = NULL;
    xmlXPathObjectPtr result = NULL;
    xmlChar *string = NULL;

    if (!context) return IE_INVALID_CONTEXT;
    if (!outgoing_message) return IE_INVAL;

    /* Check if connection is established
     * TODO: This check should be done donwstairs. */
    if (!context->curl) return IE_CONNECTION_CLOSED;


    /* Build CreateMessage request */
    request = xmlNewNode(NULL, BAD_CAST "CreateMessage");
    if (!request) {
        isds_log_message(context,
                _("Could build CreateMessage request"));
        return IE_ERROR;
    }
    isds_ns = xmlNewNs(request, BAD_CAST ISDS_NS, NULL);
    if(!isds_ns) {
        isds_log_message(context, _("Could not create ISDS name space"));
        xmlFreeNode(request);
        return IE_ERROR;
    }
    xmlSetNs(request, isds_ns);


    /* Build envelope */
    envelope = xmlNewChild(request, NULL, BAD_CAST "dmEnvelope", NULL);
    if (!envelope) {
        isds_log_message(context, _("Could not add dmEnvelope child to "
                    "CreateMessage element"));
        xmlFreeNode(request);
        return IE_ERROR;
    }

    if (!outgoing_message->envelope) {
        isds_log_message(context, _("outgoing message is missing envelope"));
        err = IE_INVAL;
        goto leave;
    }

    INSERT_STRING(envelope, "dmSenderOrgUnit",
            outgoing_message->envelope->dmSenderOrgUnit);
    INSERT_LONGINT(envelope, "dmSenderOrgUnitNum",
            outgoing_message->envelope->dmSenderOrgUnitNum, string);

    if (!outgoing_message->envelope->dbIDRecipient) {
        isds_log_message(context,
                _("outgoing message is missing recipient box identifier"));
        err = IE_INVAL;
        goto leave;
    }
    INSERT_STRING(envelope, "dbIDRecipient",
            outgoing_message->envelope->dbIDRecipient);

    INSERT_STRING(envelope, "dmRecipientOrgUnit",
            outgoing_message->envelope->dmRecipientOrgUnit);
    INSERT_LONGINT(envelope, "dmRecipientOrgUnitNum",
            outgoing_message->envelope->dmRecipientOrgUnitNum, string);
    INSERT_STRING(envelope, "dmToHands", outgoing_message->envelope->dmToHands);

#define CHECK_FOR_STRING_LENGTH(string, limit, name) \
    if ((string) && xmlUTF8Strlen((xmlChar *) (string)) > (limit)) { \
        isds_printf_message(context, \
                _("%s has more than %d characters"), (name), (limit)); \
        err = IE_2BIG; \
        goto leave; \
    }

    CHECK_FOR_STRING_LENGTH(outgoing_message->envelope->dmAnnotation, 255,
            "dmAnnotation");
    INSERT_STRING(envelope, "dmAnnotation",
            outgoing_message->envelope->dmAnnotation);

    CHECK_FOR_STRING_LENGTH(outgoing_message->envelope->dmRecipientRefNumber,
            50, "dmRecipientRefNumber");
    INSERT_STRING(envelope, "dmRecipientRefNumber",
            outgoing_message->envelope->dmRecipientRefNumber);

    CHECK_FOR_STRING_LENGTH(outgoing_message->envelope->dmSenderRefNumber,
            50, "dmSenderRefNumber");
    INSERT_STRING(envelope, "dmSenderRefNumber",
            outgoing_message->envelope->dmSenderRefNumber);

    CHECK_FOR_STRING_LENGTH(outgoing_message->envelope->dmRecipientIdent,
            50, "dmRecipientIdent");
    INSERT_STRING(envelope, "dmRecipientIdent",
            outgoing_message->envelope->dmRecipientIdent);

    CHECK_FOR_STRING_LENGTH(outgoing_message->envelope->dmSenderIdent,
            50, "dmSenderIdent");
    INSERT_STRING(envelope, "dmSenderIdent",
            outgoing_message->envelope->dmSenderIdent);

    INSERT_LONGINT(envelope, "dmLegalTitleLaw",
            outgoing_message->envelope->dmLegalTitleLaw, string);
    INSERT_LONGINT(envelope, "dmLegalTitleYear",
            outgoing_message->envelope->dmLegalTitleYear, string);
    INSERT_STRING(envelope, "dmLegalTitleSect",
            outgoing_message->envelope->dmLegalTitleSect);
    INSERT_STRING(envelope, "dmLegalTitlePar",
            outgoing_message->envelope->dmLegalTitlePar);
    INSERT_STRING(envelope, "dmLegalTitlePoint",
            outgoing_message->envelope->dmLegalTitlePoint);

    INSERT_BOOLEAN(envelope, "dmPersonalDelivery",
            outgoing_message->envelope->dmPersonalDelivery);
    INSERT_BOOLEAN(envelope, "dmAllowSubstDelivery",
            outgoing_message->envelope->dmAllowSubstDelivery);

#undef CHECK_FOR_STRING_LENGTH

    /* ???: Should we require value for dbEffectiveOVM sender?
     * ISDS has default as true */
    INSERT_BOOLEAN(envelope, "dmOVM", outgoing_message->envelope->dmOVM);


    /* Append dmFiles */
    if (!outgoing_message->documents) {
        isds_log_message(context,
                _("outgoing message is missing list of documents"));
        err = IE_INVAL;
        goto leave;
    }
    dm_files = xmlNewChild(request, NULL, BAD_CAST "dmFiles", NULL);
    if (!dm_files) {
        isds_log_message(context, _("Could not add dmFiles child to "
                    "CreateMessage element"));
        err = IE_ERROR;
        goto leave;
    }

    /* Process each document */
    for (struct isds_list *item =
            (struct isds_list *) outgoing_message->documents;
            item; item = item->next) {
        if (!item->data) {
            isds_log_message(context,
                    _("list of documents contains empty item"));
            err = IE_INVAL;
            goto leave;
        }
        /* FIXME: Check for dmFileMetaType and for document references.
         * Only first document can be of MAIN type */
        err = insert_document(context, (struct isds_document*) item->data,
                dm_files);

        if (err) goto leave;
    }

    

    isds_log(ILF_ISDS, ILL_DEBUG, _("Sending CreateMessage request to ISDS\n"));

    /* Sent request */
    err = isds(context, SERVICE_DM_OPERATIONS, request, &response);
   
    /* Destroy request */
    xmlFreeNode(request); request = NULL;

    if (err) {
        isds_log(ILF_ISDS, ILL_DEBUG,
                _("Processing ISDS response on CreateMessage "
                    "request failed\n"));
        goto leave;
    }

    /* Check for response status */
    err = isds_response_status(context, SERVICE_DM_OPERATIONS, response,
            &code, &message, NULL);
    if (err) {
        isds_log(ILF_ISDS, ILL_DEBUG,
                _("ISDS response on CreateMessage request "
                    "is missing status\n"));
        goto leave;
    }

    /* Request processed, but nothing found */
    if (xmlStrcmp(code, BAD_CAST "0000")) {
        char *box_id_locale =
            utf82locale((char*)outgoing_message->envelope->dbIDRecipient);
        char *code_locale = utf82locale((char*)code);
        char *message_locale = utf82locale((char*)message);
        isds_log(ILF_ISDS, ILL_DEBUG,
                _("Server did not accept message for %s on CreateMessage "
                    "request (code=%s, message=%s)\n"),
                box_id_locale, code_locale, message_locale);
        isds_log_message(context, message_locale);
        free(box_id_locale);
        free(code_locale);
        free(message_locale);
        err = IE_ISDS;
        goto leave;
    }


    /* Extract data */
    xpath_ctx = xmlXPathNewContext(response);
    if (!xpath_ctx) {
        err = IE_ERROR;
        goto leave;
    }
    if (register_namespaces(xpath_ctx)) {
        err = IE_ERROR;
        goto leave;
    }
    result = xmlXPathEvalExpression(BAD_CAST "/isds:CreateMessageResponse",
            xpath_ctx);
    if (!result) {
        err = IE_ERROR;
        goto leave;
    }
    if (xmlXPathNodeSetIsEmpty(result->nodesetval)) {
        isds_log_message(context, _("Missing CreateMessageResponse element"));
        err = IE_ISDS;
        goto leave;
    }
    if (result->nodesetval->nodeNr > 1) {
        isds_log_message(context, _("Multiple CreateMessageResponse element"));
        err = IE_ISDS;
        goto leave;
    }
    xpath_ctx->node = result->nodesetval->nodeTab[0];
    xmlXPathFreeObject(result); result = NULL;

    if (outgoing_message->envelope->dmID) {
        free(outgoing_message->envelope->dmID);
        outgoing_message->envelope->dmID = NULL;
    }
    EXTRACT_STRING("isds:dmID", outgoing_message->envelope->dmID);
    if (!outgoing_message->envelope->dmID) {
        isds_log(ILF_ISDS, ILL_ERR, _("Server accepted sent message, "
                    "but did not returen assigned message ID\n"));
    }

leave:
    free(string);
    xmlXPathFreeObject(result);
    xmlXPathFreeContext(xpath_ctx);

    free(code);
    free(message);
    xmlFreeDoc(response);
    xmlFreeNode(request);

    if (!err)
        isds_log(ILF_ISDS, ILL_DEBUG,
                _("CreateMessage request processed by server "
                    "successfully.\n"));

    return err;
}

#undef INSERT_STRING_ATTRIBUTE
#undef INSERT_LONGINT
#undef INSERT_BOOLEAN
#undef INSERT_STRING
#undef EXTRACT_LONGINT
#undef EXTRACT_BOOLEAN
#undef EXTRACT_STRING


/*int isds_get_message(struct isds_ctx *context, const unsigned int id,
        struct isds_message **message);
int isds_send_message(struct isds_ctx *context, struct isds_message *message);
int isds_list_messages(struct isds_ctx *context, struct isds_message **message);
int isds_find_recipient(struct isds_ctx *context, const struct address *pattern,
        struct isds_address **address);

int isds_message_free(struct isds_message **message);
int isds_address_free(struct isds_address **address);
*/


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

