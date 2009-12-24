#define _XOPEN_SOURCE 500   /* strdup from string.h, strptime from time.h */
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdarg.h>
#include <ctype.h>
#include "isds_priv.h"
#include "utils.h"
#include "soap.h"
#include "validator.h"
#include "crypto.h"
#include <gpg-error.h> /* Because of ksba or gpgme */


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


/* Deallocate structure isds_hash and NULL it.
 * @hash  hash to to free */
void isds_hash_free(struct isds_hash **hash) {
    if(!hash || !*hash) return;
    free((*hash)->value);
    zfree((*hash));
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

    free((*envelope)->dmOrdinal);
    free((*envelope)->dmMessageStatus);
    free((*envelope)->dmDeliveryTime);
    free((*envelope)->dmAcceptanceTime);
    isds_hash_free(&(*envelope)->hash);
    free((*envelope)->timestamp);

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
    log_facilities = ILF_ALL;
    log_level = ILL_WARNING;

    /* Initialize CURL */
    if (curl_global_init(CURL_GLOBAL_ALL)) {
        isds_log(ILF_ISDS, ILL_CRIT, _("CURL library initialization failed\n"));
        return IE_ERROR;
    }

    /* Inicialize gpg-error because of gpgme and ksba */
    if (gpg_err_init()) {
        isds_log(ILF_ISDS, ILL_CRIT,
                _("gpg-error library initialization failed\n"));
        return IE_ERROR;
    }

    /* Initialize GPGME */
    if (init_gpgme()) {
        isds_log(ILF_ISDS, ILL_CRIT,
                _("GPGME library initialization failed\n"));
        return IE_ERROR;
    }

    /* Initialize gcrypt */
    if (init_gcrypt()) {
        isds_log(ILF_ISDS, ILL_CRIT,
                _("gcrypt library initialization failed\n"));
        return IE_ERROR;
    }


    /* This can _exit() current program. Find not so assertive check. */
    LIBXML_TEST_VERSION;

    /* Allocate global variables */


    return IE_SUCCESS;
}


/* Deinicialize ISDS library.
 * Global function, must be called as last library function. */
isds_error isds_cleanup(void) {
    /* XML */
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
        case IE_HTTP:
            return(_("HTTP problem")); break;
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
        case IE_NOTUNIQ:
            return(_("Value not unique")); break;
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
    free((*context)->tls_verify_server);
    free((*context)->tls_ca_file);
    free((*context)->tls_ca_dir);
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


/* Change SSL/TLS settings.
 * @context is context which setting will be applied to
 * @option is name of option. It determines the type of last argument. See
 * isds_tls_option definition for more info.
 * @... is value of new setting. Type is determined by @option
 * */
isds_error isds_set_tls(struct isds_ctx *context, const isds_tls_option option,
        ...) {
    isds_error err = IE_SUCCESS;
    va_list ap;
    char *pointer, *string;

    if (!context) return IE_INVALID_CONTEXT;

    va_start(ap, option);

#define REPLACE_VA_STRING(destination) \
    string = va_arg(ap, char *); \
    if (string) { \
        pointer = realloc((destination), 1 + strlen(string)); \
        if (!pointer) { err = IE_NOMEM; goto leave; } \
        strcpy(pointer, string); \
        (destination) = pointer; \
    } else { \
        free(destination); \
        (destination) = NULL; \
    } 

    switch (option) {
        case ITLS_VERIFY_SERVER:
            if (!context->tls_verify_server) {
                context->tls_verify_server =
                    malloc(sizeof(*context->tls_verify_server));
                if (!context->tls_verify_server) {
                    err = IE_NOMEM; goto leave;
                }
            }
            *context->tls_verify_server = (_Bool) (0 != va_arg(ap, int));
            break;

        case ITLS_CA_FILE:
            REPLACE_VA_STRING(context->tls_ca_file);
            break;
        case ITLS_CA_DIRECTORY:
            REPLACE_VA_STRING(context->tls_ca_dir);
            break;

        default:
            err = IE_ENUM; goto leave;
    }

#undef REPLACE_VA_STRING

leave:
    va_end(ap);
    return err;
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

        /* Discard credentials for sure. They should not survive isds_login(),
         * even successful .*/
        discard_credentials(context);
        free(context->url);
        context->url = NULL;

        isds_log(ILF_ISDS, ILL_DEBUG, _("Logged out from ISDS server\n"));
    } else {
        discard_credentials(context);
    }
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


/* Serialize XML subtree to buffer preserving XML indentatition.
 * @context is session context
 * @subtree is XML element to be serialized (with childern)
 * @buffer is automarically reallocated buffer where serialize to
 * @length is size of serialized stream in bytes
 * @return standard error code, free @buffer in case of error */
static isds_error serialize_subtree(struct isds_ctx *context,
        xmlNodePtr subtree, void **buffer, size_t *length) {
    isds_error err = IE_SUCCESS;
    xmlBufferPtr xml_buffer = NULL;
    xmlSaveCtxtPtr save_ctx = NULL;
    xmlDocPtr subtree_doc = NULL;
    xmlNodePtr subtree_copy;
    xmlNsPtr isds_ns;
    void *new_buffer;

    if (!context) return IE_INVALID_CONTEXT;
    if (!buffer) return IE_INVAL;
    zfree(*buffer);
    if (!subtree || !length) return IE_INVAL;

    /* Make temporary XML document with @subtree root element */
    /* XXX: We can not use xmlNodeDump() because it dumps the subtree as is.
     * It can result in not well-formed on invalid XML tree (e.g. name space
     * prefix definition can miss. */
    /*FIXME */
    
    subtree_doc = xmlNewDoc(BAD_CAST "1.0");
    if (!subtree_doc) {
        isds_log_message(context, _("Could not build temporary document"));
        err = IE_ERROR;
        goto leave;
    }
    
    /* XXX: Copy subtree and attach the copy to document.
     * One node can not bee attached into more document at the same time.
     * XXX: Check xmlDOMWrapRemoveNode(). It could solve NS references
     * automatically.
     * XXX: Check xmlSaveTree() too. */
    subtree_copy = xmlCopyNodeList(subtree);
    if (!subtree_copy) {
        isds_log_message(context, _("Could not copy subtree"));
        err = IE_ERROR;
        goto leave;
    }
    xmlDocSetRootElement(subtree_doc, subtree_copy);

    /* Only this way we get namespace definition as @xmlns:isds,
     * otherwise we get namespace prefix without definition */
    /* FIXME: Don't overwrite original default namespace */
    isds_ns = xmlNewNs(subtree_copy, BAD_CAST ISDS_NS, NULL);
    if(!isds_ns) {
        isds_log_message(context, _("Could not create ISDS name space"));
        err = IE_ERROR;
        goto leave;
    }
    xmlSetNs(subtree_copy, isds_ns);


    /* Serialize the document into buffer */
    xml_buffer = xmlBufferCreate();
    if (!xml_buffer) {
        isds_log_message(context, _("Could not create xmlBuffer"));
        err = IE_ERROR;
        goto leave;
    }
    /* Last argument 0 means to not format the XML tree */
    save_ctx = xmlSaveToBuffer(xml_buffer, "UTF-8", 0);
    if (!save_ctx) {
        isds_log_message(context, _("Could not create XML serializer"));
        err = IE_ERROR;
        goto leave;
    }
    /* XXX: According LibXML documentation, this function does not return
     * meaningfull value yet */
    xmlSaveDoc(save_ctx, subtree_doc);
    if (-1 == xmlSaveFlush(save_ctx)) {
        isds_log_message(context,
                _("Could not serialize XML subtree"));
        err = IE_ERROR;
        goto leave;
    }
    /* XXX: libxml-2.7.4 complains when xmlSaveClose() on immutable buffer
     * even after xmlSaveFlush(). Thus close it here */
    xmlSaveClose(save_ctx); save_ctx = NULL;
   

    /* Store and detach buffer from xml_buffer */
    *buffer = xml_buffer->content;
    *length = xml_buffer->use;
    xmlBufferSetAllocationScheme(xml_buffer, XML_BUFFER_ALLOC_IMMUTABLE);

    /* Shrink buffer */
    new_buffer = realloc(*buffer, *length);
    if (new_buffer) *buffer = new_buffer;

leave:
    if (err) {
        zfree(*buffer);
        *length = 0;
    }

    xmlSaveClose(save_ctx);
    xmlBufferFree(xml_buffer);
    xmlFreeDoc(subtree_doc); /* Frees subtree_copy, isds_ns etc. */
    return err;
}


/* Dump XML subtree to buffer as literral string, not valid XML possibly.
 * @context is session context
 * @document is original document where @nodeset points to
 * @nodeset is XPath node set to dump (recursively)
 * @buffer is automarically reallocated buffer where serialize to
 * @length is size of serialized stream in bytes
 * @return standard error code, free @buffer in case of error */
static isds_error dump_nodeset(struct isds_ctx *context,
        const xmlDocPtr document, const xmlNodeSetPtr nodeset,
        void **buffer, size_t *length) {
    isds_error err = IE_SUCCESS;
    xmlBufferPtr xml_buffer = NULL;
    void *new_buffer;

    if (!context) return IE_INVALID_CONTEXT;
    if (!buffer) return IE_INVAL;
    zfree(*buffer); 
    if (!document || !nodeset || !length) return IE_INVAL;
    *length = 0;

    /* Empty node set results into NULL buffer */
    if (xmlXPathNodeSetIsEmpty(nodeset)) {
        goto leave;
    }

    /* Resuling the document into buffer */
    xml_buffer = xmlBufferCreate();
    if (!xml_buffer) {
        isds_log_message(context, _("Could not create xmlBuffer"));
        err = IE_ERROR;
        goto leave;
    }
   
    /* Itearate over all nodes */
    for (int i = 0; i < nodeset->nodeNr; i++) {
        /* Serialize node.
         * XXX: xmlNodeDump() appends to xml_buffer. */
        if (-1 ==
                xmlNodeDump(xml_buffer, document, nodeset->nodeTab[i], 0, 0)) {
            isds_log_message(context, _("Could not dump XML node"));
            err = IE_ERROR;
            goto leave;
        }
    }

    /* Store and detach buffer from xml_buffer */
    *buffer = xml_buffer->content;
    *length = xml_buffer->use;
    xmlBufferSetAllocationScheme(xml_buffer, XML_BUFFER_ALLOC_IMMUTABLE);

    /* Shrink buffer */
    new_buffer = realloc(*buffer, *length);
    if (new_buffer) *buffer = new_buffer;


leave:
    if (err) {
        zfree(*buffer);
        *length = 0;
    }

    xmlBufferFree(xml_buffer);
    return err;
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
            case DBTYPE_PFO_DANPOR: return(BAD_CAST "PFO_DANPOR"); break;
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


/* Convert UTF-8 @string to ISDS dmFileMetaType enum @type.
 * @Return IE_ENUM if @string is not valid enum member */
static isds_error string2isds_FileMetaType(const xmlChar *string,
        isds_FileMetaType *type) {
    if (!string || !type) return IE_INVAL;

    if (!xmlStrcmp(string, BAD_CAST "main"))
        *type = FILEMETATYPE_MAIN;
    else if (!xmlStrcmp(string, BAD_CAST "enclosure"))
        *type = FILEMETATYPE_ENCLOSURE;
    else if (!xmlStrcmp(string, BAD_CAST "signature"))
        *type = FILEMETATYPE_SIGNATURE;
    else if (!xmlStrcmp(string, BAD_CAST "meta"))
        *type = FILEMETATYPE_META;
    else
        return IE_ENUM;
    return IE_SUCCESS;
}


/* Convert UTF-8 @string to ISDS hash @algorithm.
 * @Return IE_ENUM if @string is not valid enum member */
static isds_error string2isds_hash_algorithm(const xmlChar *string,
        isds_hash_algorithm *algorithm) {
    if (!string || !algorithm) return IE_INVAL;

    if (!xmlStrcmp(string, BAD_CAST "MD5"))
        *algorithm = HASH_ALGORITHM_MD5;
    else if (!xmlStrcmp(string, BAD_CAST "SHA-1"))
        *algorithm = HASH_ALGORITHM_SHA_1;
    else if (!xmlStrcmp(string, BAD_CAST "SHA-256"))
        *algorithm = HASH_ALGORITHM_SHA_256;
    else if (!xmlStrcmp(string, BAD_CAST "SHA-512"))
        *algorithm = HASH_ALGORITHM_SHA_512;
    else
        return IE_ENUM;
    return IE_SUCCESS;
}


/* Convert UTF-8 @string represantion of ISO 8601 date to @time.
 * XXX: Not all ISO formats are supported */
static isds_error datestring2tm(const xmlChar *string, struct tm *time) {
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
static isds_error tm2datestring(const struct tm *time, xmlChar **string) {
    if (!time || !string) return IE_INVAL;

    if (-1 == isds_asprintf((char **) string, "%d-%02d-%02d",
                time->tm_year + 1900, time->tm_mon + 1, time->tm_mday))
        return IE_ERROR;

    return IE_SUCCESS;
}


/* Convert struct timeval * @time to UTF-8 ISO 8601 date-time @string. It
 * respects the @time microseconds too. */
static isds_error timeval2timestring(const struct timeval *time,
        xmlChar **string) {
    struct tm broken;

    if (!time || !string) return IE_INVAL;

    if (!gmtime_r(&time->tv_sec, &broken)) return IE_DATE;
    if (time->tv_usec < 0 || time->tv_usec > 999999) return IE_DATE;

    /* TODO: small negative year should be formated as "-0012". This is not
     * true for glibc "%04d". We should implement it.
     * TODO: What's type of time->tv_usec exactly? Unsigned? Absolute?
     * See <http://www.w3.org/TR/2001/REC-xmlschema-2-20010502/#dateTime> */ 
    if (-1 == isds_asprintf((char **) string,
                "%04d-%02d-%02dT%02d:%02d:%02d.%06ld",
                broken.tm_year + 1900, broken.tm_mon + 1, broken.tm_mday,
                broken.tm_hour, broken.tm_min, broken.tm_sec,
                time->tv_usec))
        return IE_ERROR;

    return IE_SUCCESS;
}


/* Convert UTF-8 ISO 8601 date-time @string to struct timeval.
 * It respects microseconds too.
 * In case of error, @time will be freed. */
static isds_error timestring2timeval(const xmlChar *string,
        struct timeval **time) {
    struct tm broken;
    char *offset, *delim, *endptr;
    char subseconds[7];
    int offset_hours, offset_minutes;
    int i;
    
    if (!time) return IE_INVAL;

    memset(&broken, 0, sizeof(broken));

    if (!*time) {
        *time = calloc(1, sizeof(**time));
        if (!*time) return IE_NOMEM;
    } else {
        memset(*time, 0, sizeof(**time));
    }


    /* xsd:date is ISO 8601 string, thus ASCII */
    /*TODO: negative year */

    /* Parse date and time without subseconds and offset */
    offset = strptime((char*)string, "%Y-%m-%dT%T", &broken);
    if (!offset) {
        free(*time); *time = NULL;
        return IE_DATE;
    }
    
    /* Get subseconds */
    if (*offset == '.' ) {
        offset++;

        /* Copy first 6 digits, padd it with zeros.
         * XXX: It truncates longer number, no round.
         * Current server implementation uses only milisecond resolution. */
        /* TODO: isdigit() is locale sensitive */
        for (i = 0;
                i < sizeof(subseconds)/sizeof(char) - 1  && isdigit(*offset);
                i++, offset++) {
            subseconds[i] = *offset;
        }
        for (; i < sizeof(subseconds)/sizeof(char) - 1; i++) {
            subseconds[i] = '0';
        }
        subseconds[6] = '\0';

        /* Convert it into integer */
        (*time)->tv_usec = strtol(subseconds, &endptr, 10); 
        if (*endptr != '\0' || (*time)->tv_usec == LONG_MIN ||
            (*time)->tv_usec == LONG_MAX) {
            free(*time); *time = NULL;
            return IE_DATE;
        }

        /* move to the zone offset delimiter */
        delim = strchr(offset, '-');
        if (!delim)
            delim = strchr(offset, '+');
        offset = delim;
    }

    /* Get zone offset */
    /* ISO allows zone offset string only: "" | "Z" | ("+"|"-" "<HH>:<MM>")
     * "" equals to "Z" and it means UTC zone. */
    /* One can not use strptime(, "%z",) becase it's RFC E-MAIL format without
     * colon separator */
    if (*offset == '-' || *offset == '+') {
        offset++;
        if (2 != sscanf(offset, "%2d:%2d", &offset_hours, &offset_minutes)) {
            free(*time); *time = NULL;
            return IE_DATE;
        }
        broken.tm_hour -= offset_hours;
        broken.tm_min -= offset_minutes * ((offset_hours<0) ? -1 : 1);
    }

    /* Convert to time_t */
    switch_tz_to_utc();
    (*time)->tv_sec = mktime(&broken);
    switch_tz_to_native();
    if ((*time)->tv_sec == (time_t) -1) {
        free(*time); *time = NULL;
        return IE_DATE;
    }

    return IE_SUCCESS;
}


/* Convert unsigned int into isds_message_status.
 * @context is session context
 * @number is pointer to number value. NULL will be treated as invalid value.
 * @status is automatically reallocated status
 * @return IE_SUCCESS, or error code and free status */
static isds_error uint2isds_message_status(struct isds_ctx *context,
    const unsigned long int *number, isds_message_status **status) {
    if (!context) return IE_INVALID_CONTEXT;
    if (!status) return IE_INVAL;

    free(*status); *status = NULL;
    if (!number) return IE_INVAL;

    if (*number < 1 || *number > 9) {
        isds_printf_message(context, _("Invalid messsage status value: %lu"),
                *number);
        return IE_ENUM;
    }

    *status = malloc(sizeof(**status));
    if (!*status) return IE_NOMEM;

    **status = 1 << *number;
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
                free(string); \
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
                free(string); \
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

#define EXTRACT_ULONGINT(element, ulongintPtr, preallocated) \
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
                free(string); \
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
                free(string); \
                err = IE_ERROR; \
                goto leave; \
            } \
             \
            free(string); string = NULL; \
            if (number < 0) { \
                isds_printf_message(context, \
                        _(element " value is negative: %ld"), number); \
                err = IE_ERROR; \
                goto leave; \
            } \
             \
            if (!(preallocated)) { \
                (ulongintPtr) = calloc(1, sizeof(*(ulongintPtr))); \
                if (!(ulongintPtr)) { \
                    err = IE_NOMEM; \
                    goto leave; \
                } \
            } \
            *(ulongintPtr) = number; \
        } \
    }

#define EXTRACT_STRING_ATTRIBUTE(attribute, string, required) \
    (string) = (char *) xmlGetNsProp(xpath_ctx->node, ( BAD_CAST attribute), \
            NULL); \
    if ((required) && (!string)) { \
        char *attribute_locale = utf82locale(attribute); \
        char *element_locale = utf82locale((char *)xpath_ctx->node->name); \
        isds_printf_message(context, \
                _("Could not extract required %s attribute value from " \
                    "%s element"), attribute_locale, element_locale); \
        free(element_locale); \
        free(attribute_locale); \
        err = IE_ERROR; \
        goto leave; \
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

#define INSERT_ULONGINT(parent, element, ulongintPtr, buffer) \
    if ((ulongintPtr)) { \
        /* FIXME: locale sensitive */ \
        if (-1 == isds_asprintf((char **)&(buffer), "%lu", *(ulongintPtr))) { \
            err = IE_NOMEM; \
            goto leave; \
        } \
        INSERT_STRING(parent, element, buffer) \
        free(buffer); (buffer) = NULL; \
    } else { INSERT_STRING(parent, element, NULL) }

#define INSERT_ULONGINTNOPTR(parent, element, ulongint, buffer) \
    /* FIXME: locale sensitive */ \
    if (-1 == isds_asprintf((char **)&(buffer), "%lu", ulongint)) { \
        err = IE_NOMEM; \
        goto leave; \
    } \
    INSERT_STRING(parent, element, buffer) \
    free(buffer); (buffer) = NULL; \

#define INSERT_STRING_ATTRIBUTE(parent, attribute, string) \
    attribute_node = xmlNewProp((parent), BAD_CAST (attribute), \
            (xmlChar *) (string)); \
    if (!attribute_node) { \
        isds_printf_message(context, _("Could not add " attribute \
                    " attribute to %s element"), (parent)->name); \
        err = IE_ERROR; \
        goto leave; \
    }


/* Find child element by name in given XPath context and switch context onto
 * it. The child must be uniq and must exist. Otherwise failes.
 * @context is ISDS context
 * @child is child element name
 * @xpath_ctx is XPath context. In success, the @xpath_ctx will be changed
 * into it child. In error case, the @xpath_ctx keeps original value. */
static isds_error move_xpathctx_to_child(struct isds_ctx *context,
        const xmlChar *child, xmlXPathContextPtr xpath_ctx) {
    isds_error err = IE_SUCCESS;
    xmlXPathObjectPtr result = NULL;

    if (!context) return IE_INVALID_CONTEXT;
    if (!child || !xpath_ctx) return IE_INVAL;

    /* Find child */
    result = xmlXPathEvalExpression(child, xpath_ctx);
    if (!result) {
        err = IE_XML;
        goto leave;
    }

    /* No match */
    if (xmlXPathNodeSetIsEmpty(result->nodesetval)) {
        char *parent_locale = utf82locale((char*) xpath_ctx->node->name);
        char *child_locale = utf82locale((char*) child);
        isds_printf_message(context,
                _("%s element does not contain %s child"),
                parent_locale, child_locale);
        free(child_locale);
        free(parent_locale);
        err = IE_NOEXIST;
        goto leave;
    }

    /* More matches */
    if (result->nodesetval->nodeNr > 1) {
        char *parent_locale = utf82locale((char*) xpath_ctx->node->name);
        char *child_locale = utf82locale((char*) child);
        isds_printf_message(context,
                _("%s element contains multiple %s childs"),
                parent_locale, child_locale);
        free(child_locale);
        free(parent_locale);
        err = IE_NOTUNIQ;
        goto leave;
    }

    /* Switch context */
    xpath_ctx->node = result->nodesetval->nodeTab[0];

leave:
    xmlXPathFreeObject(result);
    return err;
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


/* Convert XSD gMessageEnvelopeSub group of elements from XML tree into
 * isds_envelope structure. The envelope is automatically allocated but not
 * reallocated. The date are just appended into envelope structure.
 * @context is ISDS context
 * @envelope is automically allocated message envelope structure
 * @xpath_ctx is XPath context with current node as gMessageEnvelope parent
 * In case of error @envelope will be freed. */
static isds_error append_GMessageEnvelopeSub(struct isds_ctx *context,
        struct isds_envelope **envelope, xmlXPathContextPtr xpath_ctx) {
    isds_error err = IE_SUCCESS;
    xmlXPathObjectPtr result = NULL;

    if (!context) return IE_INVALID_CONTEXT;
    if (!envelope) return IE_INVAL;
    if (!xpath_ctx) return IE_INVAL;


    if (!*envelope) {
        /* Allocate envelope */
        *envelope = calloc(1, sizeof(**envelope));
        if (!*envelope) {
            err = IE_NOMEM;
            goto leave;
        }
    } else {
        /* Else free former data */
        zfree((*envelope)->dmSenderOrgUnit);
        zfree((*envelope)->dmSenderOrgUnitNum);
        zfree((*envelope)->dbIDRecipient);
        zfree((*envelope)->dmRecipientOrgUnit);
        zfree((*envelope)->dmSenderOrgUnitNum);
        zfree((*envelope)->dmToHands);
        zfree((*envelope)->dmAnnotation);
        zfree((*envelope)->dmRecipientRefNumber);
        zfree((*envelope)->dmSenderRefNumber);
        zfree((*envelope)->dmRecipientIdent);
        zfree((*envelope)->dmSenderIdent);
        zfree((*envelope)->dmLegalTitleLaw);
        zfree((*envelope)->dmLegalTitleYear);
        zfree((*envelope)->dmLegalTitleSect);
        zfree((*envelope)->dmLegalTitlePar);
        zfree((*envelope)->dmLegalTitlePoint);
        zfree((*envelope)->dmPersonalDelivery);
        zfree((*envelope)->dmAllowSubstDelivery);
    }

    /* Extract envelope elements added by sender or ISDS
     * (XSD: gMessageEnvelopeSub type) */
    EXTRACT_STRING("isds:dmSenderOrgUnit", (*envelope)->dmSenderOrgUnit);
    EXTRACT_LONGINT("isds:dmSenderOrgUnitNum",
            (*envelope)->dmSenderOrgUnitNum, 0);
    EXTRACT_STRING("isds:dbIDRecipient", (*envelope)->dbIDRecipient);
    EXTRACT_STRING("isds:dmRecipientOrgUnit", (*envelope)->dmRecipientOrgUnit);
    EXTRACT_LONGINT("isds:dmRecipientOrgUnitNum",
            (*envelope)->dmSenderOrgUnitNum, 0);
    EXTRACT_STRING("isds:dmToHands", (*envelope)->dmToHands);
    EXTRACT_STRING("isds:dmAnnotation", (*envelope)->dmAnnotation);
    EXTRACT_STRING("isds:dmRecipientRefNumber",
            (*envelope)->dmRecipientRefNumber);
    EXTRACT_STRING("isds:dmSenderRefNumber", (*envelope)->dmSenderRefNumber);
    EXTRACT_STRING("isds:dmRecipientIdent", (*envelope)->dmRecipientIdent);
    EXTRACT_STRING("isds:dmSenderIdent", (*envelope)->dmSenderIdent);

    /* Extract envelope elements regarding law refference */
    EXTRACT_LONGINT("isds:dmLegalTitleLaw", (*envelope)->dmLegalTitleLaw, 0);
    EXTRACT_LONGINT("isds:dmLegalTitleYear", (*envelope)->dmLegalTitleYear, 0);
    EXTRACT_STRING("isds:dmLegalTitleSect", (*envelope)->dmLegalTitleSect);
    EXTRACT_STRING("isds:dmLegalTitlePar", (*envelope)->dmLegalTitlePar);
    EXTRACT_STRING("isds:dmLegalTitlePoint", (*envelope)->dmLegalTitlePoint);

    /* Extract envelope other elements */
    EXTRACT_BOOLEAN("isds:dmPersonalDelivery", (*envelope)->dmPersonalDelivery);
    EXTRACT_BOOLEAN("isds:dmAllowSubstDelivery",
            (*envelope)->dmAllowSubstDelivery);

leave:
    if (err) isds_envelope_free(envelope);
    xmlXPathFreeObject(result);
    return err;
}



/* Convert XSD gMessageEnvelope group of elements from XML tree into
 * isds_envelope structure. The envelope is automatically allocated but not
 * reallocated. The date are just appended into envelope structure.
 * @context is ISDS context
 * @envelope is automically allocated message envelope structure
 * @xpath_ctx is XPath context with current node as gMessageEnvelope parent
 * In case of error @envelope will be freed. */
static isds_error append_GMessageEnvelope(struct isds_ctx *context,
        struct isds_envelope **envelope, xmlXPathContextPtr xpath_ctx) {
    isds_error err = IE_SUCCESS;
    xmlXPathObjectPtr result = NULL;

    if (!context) return IE_INVALID_CONTEXT;
    if (!envelope) return IE_INVAL;
    if (!xpath_ctx) return IE_INVAL;


    if (!*envelope) {
        /* Allocate envelope */
        *envelope = calloc(1, sizeof(**envelope));
        if (!*envelope) {
            err = IE_NOMEM;
            goto leave;
        }
    } else {
        /* Else free former data */
        zfree((*envelope)->dmID);
        zfree((*envelope)->dbIDSender);
        zfree((*envelope)->dmSender);
        zfree((*envelope)->dmSenderAddress);
        zfree((*envelope)->dmSenderType);
        zfree((*envelope)->dmRecipient);
        zfree((*envelope)->dmRecipientAddress);
        zfree((*envelope)->dmAmbiguousRecipient);
    }

    /* Extract envelope elements added by ISDS
     * (XSD: gMessageEnvelope type) */
    EXTRACT_STRING("isds:dmID", (*envelope)->dmID);
    EXTRACT_STRING("isds:dbIDSender", (*envelope)->dbIDSender);
    EXTRACT_STRING("isds:dmSender", (*envelope)->dmSender);
    EXTRACT_STRING("isds:dmSenderAddress", (*envelope)->dmSenderAddress);
    /* XML Schema does not guaratee enumratation. It's plain xs:int. */
    EXTRACT_LONGINT("isds:dmSenderType", (*envelope)->dmSenderType, 0);
    EXTRACT_STRING("isds:dmRecipient", (*envelope)->dmRecipient);
    EXTRACT_STRING("isds:dmRecipientAddress", (*envelope)->dmRecipientAddress);
    EXTRACT_BOOLEAN("isds:dmAmbiguousRecipient",
            (*envelope)->dmAmbiguousRecipient);
    
    /* Extract envelope elements added by sender and ISDS
     * (XSD: gMessageEnvelope type) */
    err = append_GMessageEnvelopeSub(context, envelope, xpath_ctx);
    if (err) goto leave;

leave:
    if (err) isds_envelope_free(envelope);
    xmlXPathFreeObject(result);
    return err;
}


/* Convert other envelope elements from XML tree into isds_envelope structure:
 * dmMessageStatus, dmAttachmentSize, dmDeliveryTime, dmAcceptanceTime. 
 * The envelope is automatically allocated but not reallocated.
 * The data are just appended into envelope structure.
 * @context is ISDS context
 * @envelope is automically allocated message envelope structure
 * @xpath_ctx is XPath context with current node as parent desired elements
 * In case of error @envelope will be freed. */
static isds_error append_status_size_times(struct isds_ctx *context,
        struct isds_envelope **envelope, xmlXPathContextPtr xpath_ctx) {
    isds_error err = IE_SUCCESS;
    xmlXPathObjectPtr result = NULL;
    char *string = NULL;
    unsigned long int *unumber = NULL;

    if (!context) return IE_INVALID_CONTEXT;
    if (!envelope) return IE_INVAL;
    if (!xpath_ctx) return IE_INVAL;


    if (!*envelope) {
        /* Allocate new */
        *envelope = calloc(1, sizeof(**envelope));
        if (!*envelope) {
            err = IE_NOMEM;
            goto leave;
        }
    } else {
        /* Free old data */
        zfree((*envelope)->dmMessageStatus);
        zfree((*envelope)->dmAttachmentSize);
        zfree((*envelope)->dmDeliveryTime);
        zfree((*envelope)->dmAcceptanceTime);
    }

    
    /* dmMessageStatus element is mandatory */
    EXTRACT_ULONGINT("sisds:dmMessageStatus", unumber, 0);
    if (!unumber) {
        isds_log_message(context,
                _("Missing mandatory sisds:dmMessageStatus integer"));
        err = IE_ISDS;
        goto leave;
    }
    err = uint2isds_message_status(context, unumber,
            &((*envelope)->dmMessageStatus));
    if (err) {
        if (err == IE_ENUM) err = IE_ISDS;
        goto leave;
    }
    free(unumber); unumber = NULL;
    
    EXTRACT_ULONGINT("sisds:dmAttachmentSize", (*envelope)->dmAttachmentSize,
            0);

    EXTRACT_STRING("sisds:dmDeliveryTime", string);
    if (string) {
        err = timestring2timeval((xmlChar *) string,
                &((*envelope)->dmDeliveryTime));
        if (err) {
            char *string_locale = utf82locale(string);
            if (err == IE_DATE) err = IE_ISDS;
            isds_printf_message(context,
                    _("Could not convert dmDeliveryTime as ISO time: %s"),
                    string_locale);
            free(string_locale);
            goto leave;
        }
    }

    EXTRACT_STRING("sisds:dmAcceptanceTime", string);
    if (string) {
        err = timestring2timeval((xmlChar *) string,
                &((*envelope)->dmAcceptanceTime));
        if (err) {
            char *string_locale = utf82locale(string);
            if (err == IE_DATE) err = IE_ISDS;
            isds_printf_message(context,
                    _("Could not convert dmAcceptanceTime as ISO time: %s"),
                    string_locale);
            free(string_locale);
            goto leave;
        }
    }

leave:
    if (err) isds_envelope_free(envelope);
    free(unumber);
    free(string);
    xmlXPathFreeObject(result);
    return err;
}


/* Extract message document into reallocated document structure
 * @context is ISDS context
 * @document is automically reallocated message documents structure
 * @xpath_ctx is XPath context with current node as isds:dmFile
 * In case of error @document will be freed. */
static isds_error extract_document(struct isds_ctx *context,
        struct isds_document **document, xmlXPathContextPtr xpath_ctx) {
    isds_error err = IE_SUCCESS;
    xmlXPathObjectPtr result = NULL;
    xmlNodePtr file_node = xpath_ctx->node;
    char *string = NULL;

    if (!context) return IE_INVALID_CONTEXT;
    if (!document) return IE_INVAL;
    isds_document_free(document);
    if (!xpath_ctx) return IE_INVAL;

    *document = calloc(1, sizeof(**document));
    if (!*document) {
        err = IE_NOMEM;
        goto leave;
    }

    /* Extract document metadata */
    EXTRACT_STRING_ATTRIBUTE("dmMimeType", (*document)->dmMimeType, 1)

    EXTRACT_STRING_ATTRIBUTE("dmFileMetaType", string, 1)
    err = string2isds_FileMetaType((xmlChar*)string,
            &((*document)->dmFileMetaType));
    if (err) {
        char *meta_type_locale = utf82locale(string);
        isds_printf_message(context,
                _("Document has invalid dmFileMetaType attribute value: %s"),
                meta_type_locale);
        free(meta_type_locale);
        err = IE_ISDS;
        goto leave;
    }
    zfree(string);

    EXTRACT_STRING_ATTRIBUTE("dmFileGuid", (*document)->dmFileGuid, 0)
    EXTRACT_STRING_ATTRIBUTE("dmUpFileGuid", (*document)->dmUpFileGuid, 0)
    EXTRACT_STRING_ATTRIBUTE("dmFileDescr", (*document)->dmFileDescr, 0)
    EXTRACT_STRING_ATTRIBUTE("dmFormat", (*document)->dmFormat, 0)
    

    /* Extract document data.
     * Base64 encoded blob or XML subtree must be presented. */

    /* Check from dmEncodedContent */
    result = xmlXPathEvalExpression(BAD_CAST "isds:dmEncodedContent",
            xpath_ctx);
    if (!result) {
        err = IE_XML;
        goto leave;
    }

    if (!xmlXPathNodeSetIsEmpty(result->nodesetval)) {
        /* Here we have Base64 blob */

        if (result->nodesetval->nodeNr > 1) {
            isds_printf_message(context,
                    _("Document has more dmEncodedContent elements"));
            err = IE_ISDS;
            goto leave;
        }

        xmlXPathFreeObject(result); result = NULL;
        EXTRACT_STRING("isds:dmEncodedContent", string);

        /* Decode non-emptys document */
        if (string && string[0] != '\0') {
            (*document)->data_length = b64decode(string, &((*document)->data));
            if ((*document)->data_length == (size_t) -1) {
                isds_printf_message(context,
                        _("Error while Base64-decoding document content"));
                err = IE_ERROR;
                goto leave;
            }
        }
    } else {
        /* No Base64 blob, try XML document */
        xmlXPathFreeObject(result); result = NULL;
        result = xmlXPathEvalExpression(BAD_CAST "isds:dmXMLContent",
                xpath_ctx);
        if (!result) {
            err = IE_XML;
            goto leave;
        }

        if (!xmlXPathNodeSetIsEmpty(result->nodesetval)) {
            /* Here we have XML document */

            if (result->nodesetval->nodeNr > 1) {
                isds_printf_message(context,
                        _("Document has more dmXMLContent elements"));
                err = IE_ISDS;
                goto leave;
            }

            /* FIXME: Serialize the tree rooted at result's node */
            isds_printf_message(context,
                    _("XML documents not yet supported"));
            err = IE_NOTSUP;
            goto leave;
        } else {
            /* No bas64 blob, nor XML document */
            isds_printf_message(context,
                    _("Document has no dmEncodedContent, nor dmXMLContent "
                        "element"));
            err = IE_ISDS;
            goto leave;
        }
    }


leave:
    if (err) isds_document_free(document);
    free(string);
    xmlXPathFreeObject(result);
    xpath_ctx->node = file_node;
    return err;
}



/* Extract message documents into reallocated list of documents
 * @context is ISDS context
 * @documents is automically reallocated message documents list structure
 * @xpath_ctx is XPath context with current node as XSD tFilesArray
 * In case of error @documents will be freed. */
static isds_error extract_documents(struct isds_ctx *context,
        struct isds_list **documents, xmlXPathContextPtr xpath_ctx) {
    isds_error err = IE_SUCCESS;
    xmlXPathObjectPtr result = NULL;
    xmlNodePtr file;
    xmlNodePtr files_node = xpath_ctx->node;
    struct isds_list *document, *prev_document;

    if (!context) return IE_INVALID_CONTEXT;
    if (!documents) return IE_INVAL;
    isds_list_free(documents);
    if (!xpath_ctx) return IE_INVAL;

    /* Find documents */
    result = xmlXPathEvalExpression(BAD_CAST "isds:dmFile", xpath_ctx);
    if (!result) {
        err = IE_XML;
        goto leave;
    }

    /* No match */
    if (xmlXPathNodeSetIsEmpty(result->nodesetval)) {
        isds_printf_message(context,
                _("Message does not contain any document"));
        err = IE_ISDS;
        goto leave;
    }


    /* Iterate over documents */
    for (int i = 0; i < result->nodesetval->nodeNr; i++) {
        file = result->nodesetval->nodeTab[i];

        /* Allocate and append list item */
        document = calloc(1, sizeof(*document));
        if (!document) {
            err = IE_NOMEM;
            goto leave;
        }
        document->destructor = (void (*)(void **))isds_document_free;
        if (i == 0) *documents = document;
        else prev_document->next = document;
        prev_document = document;

        /* Extract document */
        xpath_ctx->node =  result->nodesetval->nodeTab[i];
        err = extract_document(context,
                (struct isds_document **) &(document->data), xpath_ctx);
        if (err) goto leave;
    }

    
leave:
    if (err) isds_list_free(documents);
    xmlXPathFreeObject(result);
    xpath_ctx->node = files_node;
    return err;
}


/* Convert isds:dmRecord XML tree into structure
 * @context is ISDS context
 * @envelope is automically reallocated message envelope structure
 * @xpath_ctx is XPath context with current node as isds:dmRecord element
 * In case of error @envelope will be freed. */
static isds_error extract_DmRecord(struct isds_ctx *context,
        struct isds_envelope **envelope, xmlXPathContextPtr xpath_ctx) {
    isds_error err = IE_SUCCESS;
    xmlXPathObjectPtr result = NULL;

    if (!context) return IE_INVALID_CONTEXT;
    if (!envelope) return IE_INVAL;
    isds_envelope_free(envelope);
    if (!xpath_ctx) return IE_INVAL;


    *envelope = calloc(1, sizeof(**envelope));
    if (!*envelope) {
        err = IE_NOMEM;
        goto leave;
    }


    /* Extract tRecord data */
    EXTRACT_ULONGINT("isds:dmOrdinal", (*envelope)->dmOrdinal, 0);

    /* Get dmMessageStatus, dmAttachmentSize, dmDeliveryTime,
     * dmAcceptanceTime. */
    err = append_status_size_times(context, envelope, xpath_ctx);
    if (err) goto leave;

    /* Extract envelope elements added by sender and ISDS
     * (XSD: gMessageEnvelope type) */
    err = append_GMessageEnvelope(context, envelope, xpath_ctx);
    if (err) goto leave;
    /* dmOVM can not be obtained from ISDS */
    
leave:
    if (err) isds_envelope_free(envelope);
    xmlXPathFreeObject(result);
    return err;
}


/* Find and convert isds:dmHash XML tree into structure
 * @context is ISDS context
 * @envelope is automically reallocated message hash structure
 * @xpath_ctx is XPath context with current node containing isds:dmHash child
 * In case of error @hash will be freed. */
static isds_error find_and_extract_DmHash(struct isds_ctx *context,
        struct isds_hash **hash, xmlXPathContextPtr xpath_ctx) {
    isds_error err = IE_SUCCESS;
    xmlNodePtr old_ctx_node;
    xmlXPathObjectPtr result = NULL;
    char *string = NULL;

    if (!context) return IE_INVALID_CONTEXT;
    if (!hash) return IE_INVAL;
    isds_hash_free(hash);
    if (!xpath_ctx) return IE_INVAL;


    *hash = calloc(1, sizeof(**hash));
    if (!*hash) {
        err = IE_NOMEM;
        goto leave;
    }

    old_ctx_node = xpath_ctx->node;

    /* Locate dmHash */
    err = move_xpathctx_to_child(context, BAD_CAST "sisds:dmHash", xpath_ctx);
    if (err == IE_NOEXIST || err == IE_NOTUNIQ) {
        err = IE_ISDS;
        goto leave;
    }
    if (err) {
        err = IE_ERROR;
        goto leave;
    }

    /* Get hash algorithm */
    EXTRACT_STRING_ATTRIBUTE("algorithm", string, 1);
    err = string2isds_hash_algorithm((xmlChar*) string, &(*hash)->algorithm);
    if (err) {
        if (err == IE_ENUM) {
            char *string_locale = utf82locale(string);
            isds_printf_message(context, _("Unsported hash algorithm: %s"),
                    string_locale);
            free(string_locale);
        }
        goto leave;
    }
    zfree(string);

    /* Get hash value */
    EXTRACT_STRING(".", string);
    if (!string) {
        isds_printf_message(context, _("tHash element is missing hash value"));
        err = IE_ISDS;
        goto leave;
    }
    (*hash)->length = b64decode(string, &((*hash)->value));
    if ((*hash)->length == (size_t) -1) {
        isds_printf_message(context,
                _("Error while Base64-decoding hash value"));
        err = IE_ERROR;
        goto leave;
    }
    
leave:
    if (err) isds_hash_free(hash);
    free(string);
    xmlXPathFreeObject(result);
    xpath_ctx->node = old_ctx_node;
    return err;
}


/* Find and append isds:dmQTimestamp XML tree into envelope
 * @context is ISDS context
 * @envelope is automically allocated evnelope structure
 * @xpath_ctx is XPath context with current node containing isds:dmQTimestamp
 * child
 * In case of error @envelope will be freed. */
static isds_error find_and_append_DmQTimestamp(struct isds_ctx *context,
        struct isds_envelope **envelope, xmlXPathContextPtr xpath_ctx) {
    isds_error err = IE_SUCCESS;
    xmlXPathObjectPtr result = NULL;
    char *string = NULL;

    if (!context) return IE_INVALID_CONTEXT;
    if (!envelope) return IE_INVAL;
    if (!xpath_ctx) {
        isds_envelope_free(envelope);
        return IE_INVAL;
    }

    if (!*envelope) {
        *envelope = calloc(1, sizeof(**envelope));
        if (!*envelope) {
            err = IE_NOMEM;
            goto leave;
        }
    } else {
        zfree((*envelope)->timestamp);
        (*envelope)->timestamp_length = 0;
    }

    /* Get dmQTimestamp */
    EXTRACT_STRING("sisds:dmQTimestamp", string);
    if (!string) {
        isds_printf_message(context, _("Missing dmQTimestamp element content"));
        err = IE_ISDS;
        goto leave;
    }
    (*envelope)->timestamp_length =
        b64decode(string, &((*envelope)->timestamp));
    if ((*envelope)->timestamp_length == (size_t) -1) {
        isds_printf_message(context,
                _("Error while Base64-decoding timestamp value"));
        err = IE_ERROR;
        goto leave;
    }
    
leave:
    if (err) isds_envelope_free(envelope);
    free(string);
    xmlXPathFreeObject(result);
    return err;
}


/* Convert XSD tReturnedMessage XML tree into message structure.
 * It doea not store XML tree into message->raw.
 * @context is ISDS context
 * @include_documents Use true if documents must be extracted
 * (tReturnedMessage XSD type), use false if documents shall be ommited
 * (tReturnedMessageEnvelope).
 * @message is automically reallocated message structure
 * @xpath_ctx is XPath context with current node as tReturnedMessage element
 * type
 * In case of error @message will be freed. */
static isds_error extract_TReturnedMessage(struct isds_ctx *context,
        const _Bool include_documents, struct isds_message **message,
        xmlXPathContextPtr xpath_ctx) {
    isds_error err = IE_SUCCESS;
    xmlNodePtr message_node;

    if (!context) return IE_INVALID_CONTEXT;
    if (!message) return IE_INVAL;
    isds_message_free(message);
    if (!xpath_ctx) return IE_INVAL;


    *message = calloc(1, sizeof(**message));
    if (!*message) {
        err = IE_NOMEM;
        goto leave;
    }

    /* Save message XPATH context node */
    message_node = xpath_ctx->node;


    /* Extract dmDM */
    err = move_xpathctx_to_child(context, BAD_CAST "isds:dmDm", xpath_ctx);
    if (err == IE_NOEXIST || err == IE_NOTUNIQ) { err = IE_ISDS; goto leave; }
    if (err) { err = IE_ERROR; goto leave; }
    err = append_GMessageEnvelope(context, &((*message)->envelope), xpath_ctx);
    if (err) goto leave;

    if (include_documents) {
        /* Extract dmFiles */
        err = move_xpathctx_to_child(context, BAD_CAST "isds:dmFiles",
                xpath_ctx);
        if (err == IE_NOEXIST || err == IE_NOTUNIQ) {
            err = IE_ISDS; goto leave;
        }
        if (err) { err = IE_ERROR; goto leave; }
        err = extract_documents(context, &((*message)->documents), xpath_ctx);
        if (err) goto leave;
    }


    /* Restore context to message */
    xpath_ctx->node = message_node;

    /* Extract dmHash */
    err = find_and_extract_DmHash(context, &(*message)->envelope->hash,
            xpath_ctx);
    if (err) goto leave;

    /* Extract dmQTimestamp, */
    err = find_and_append_DmQTimestamp(context, &(*message)->envelope,
            xpath_ctx);
    if (err) goto leave;
   
    /* Get dmMessageStatus, dmAttachmentSize, dmDeliveryTime,
     * dmAcceptanceTime. */
    err = append_status_size_times(context, &((*message)->envelope), xpath_ctx);
    if (err) goto leave;
    
leave:
    if (err) isds_message_free(message);
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
    xmlNodePtr new_file = NULL, file = NULL, node;
    xmlAttrPtr attribute_node;
    xmlChar *base64data = NULL;

    if (!context) return IE_INVALID_CONTEXT;
    if (!document || !dm_files) return IE_INVAL;

    /* Allocate new dmFile */
    new_file = xmlNewNode(dm_files->ns, BAD_CAST "dmFile");
    if (!new_file) {
        isds_printf_message(context, _("Could not allocate main dmFile"));
        err = IE_ERROR;
        goto leave;
    }
    /* Append the new dmFile.
     * XXX: Main document must go first */
    if (document->dmFileMetaType == FILEMETATYPE_MAIN && dm_files->children)
        file = xmlAddPrevSibling(dm_files->children, new_file);
    else 
        file = xmlAddChild(dm_files, new_file);

    if (!file) {
        xmlFreeNode(new_file); new_file = NULL;
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
    INSERT_STRING(file, "dmEncodedContent", base64data);
    free(base64data);

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
    if (register_namespaces(xpath_ctx, MESSAGE_NS_UNSIGNED)) {
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
    if (register_namespaces(xpath_ctx, MESSAGE_NS_UNSIGNED)) {
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
    if (register_namespaces(xpath_ctx, MESSAGE_NS_UNSIGNED)) {
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
    _Bool message_is_complete = 0;

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

    /* Check for document hieararchy */
    err = check_documents_hierarchy(context, outgoing_message->documents);
    if (err) goto leave;

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

    /* Signal we can serilize message since now */
    message_is_complete = 1;

    

    isds_log(ILF_ISDS, ILL_DEBUG, _("Sending CreateMessage request to ISDS\n"));

    /* Sent request */
    err = isds(context, SERVICE_DM_OPERATIONS, request, &response);
   
    /* Dont' destroy request, we want to privode it to application later */

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
    if (register_namespaces(xpath_ctx, MESSAGE_NS_UNSIGNED)) {
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
    /* TODO: Serialize message into structure member raw */
    /* XXX: Each web service transport message in different format.
     * Therefore it's not possible to save them directly.
     * To save them, one must figure out common format.
     * We can leave it on application, or we can implement the ESS format. */
    /*if (message_is_complete) {
        if (outgoing_message->envelope->dmID) {
        */
            /* Add assigned message ID as first child*/
            /*xmlNodePtr dmid_text = xmlNewText(
                    (xmlChar *) outgoing_message->envelope->dmID);
            if (!dmid_text) goto serialization_failed;

            xmlNodePtr dmid_element = xmlNewNode(envelope->ns,
                    BAD_CAST "dmID");
            if (!dmid_element) {
                xmlFreeNode(dmid_text);
                goto serialization_failed;
            }

            xmlNodePtr dmid_element_with_text =
                xmlAddChild(dmid_element, dmid_text);
            if (!dmid_element_with_text) {
                xmlFreeNode(dmid_element);
                xmlFreeNode(dmid_text);
                goto serialization_failed;
            }

            node = xmlAddPrevSibling(envelope->childern,
                    dmid_element_with_text);
            if (!node) {
                xmlFreeNodeList(dmid_element_with_text);
                goto serialization_failed;
            }
            */

            /* Serialize message with ID into raw */
            /*buffer = serialize_element(envelope)*/
/*        }

serialization_failed:
        

    }*/

    /* Clean up */
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


/* Get list of messages. This is common core for getting sent or received
 * messaeges.
 * Any criterion argument can be NULL, if you don't care about it.
 * @context is session context. Must not be NULL.
 * @outgoing_direction is true if you want list of outgoing messages,
 * it's false if you want incoming messages.
 * @from_time is minimal time and date of message sending inclusive.
 * @to_time is maximal time and date of message sending inclusive
 * @organization_unit_number is number of sender/recipient respectively.
 * @status_filter is bit field of isds_message_status values. Use special
 * value MESSAGESTATE_ANY to signal you don't care. (It's defined as union of
 * all values, you can use bitwise arithmetic if you want.)
 * @offset is index of first message we are interested in. First message is 1.
 * Set to 0 (or 1) if you don't care.
 * @number is maximal length of list you want to get as input value, outputs
 * number of messages matching these criteria. Can be NULL if you don't care
 * (applies to output value either).
 * @messages is automatically reallocated list of isds_message's. Be ware that
 * it returns only brief overview (envelope and some other fields) about each
 * message, not the complete message. FIXME: Specify exact fields.
 * The list is sorted by delivery time in ascending order.
 * Use NULL if
 * you don't care about don't need the data (useful if you want to know only
 * the @number). If you provide &NULL, list will be allocated on heap, if you
 * provide pointer to non-NULL, list will be freed automacally at first. Also
 * in case of error the list will be NULLed.
 * @return IE_SUCCESS or appropriate error code. */
static isds_error isds_get_list_of_messages(struct isds_ctx *context,
        _Bool outgoing_direction,
        const struct timeval *from_time, const struct timeval *to_time,
        const long int *organization_unit_number,
        const unsigned int status_filter,
        const unsigned long int offset, unsigned long int *number,
        struct isds_list **messages) {

    isds_error err = IE_SUCCESS;
    xmlNsPtr isds_ns = NULL;
    xmlNodePtr request = NULL, node;
    xmlDocPtr response = NULL;
    xmlChar *code = NULL, *message = NULL;
    xmlXPathContextPtr xpath_ctx = NULL;
    xmlXPathObjectPtr result = NULL;
    xmlChar *string = NULL;
    long unsigned int count = 0;

    if (!context) return IE_INVALID_CONTEXT;
   
    /* Free former message list if any */
    if (messages) isds_list_free(messages);

    /* Check if connection is established
     * TODO: This check should be done donwstairs. */
    if (!context->curl) return IE_CONNECTION_CLOSED;

    /* Build GetListOf*Messages request */
    request = xmlNewNode(NULL,
            (outgoing_direction) ?
                BAD_CAST "GetListOfSentMessages" :
                BAD_CAST "GetListOfReceivedMessages"
            );
    if (!request) {
        isds_log_message(context,
                (outgoing_direction) ?
                _("Could not build GetListOfSentMessages request") :
                _("Could not build GetListOfReceivedMessages request")
                );
        return IE_ERROR;
    }
    isds_ns = xmlNewNs(request, BAD_CAST ISDS_NS, NULL);
    if(!isds_ns) {
        isds_log_message(context, _("Could not create ISDS name space"));
        xmlFreeNode(request);
        return IE_ERROR;
    }
    xmlSetNs(request, isds_ns);

   
    if (from_time) {
        err = timeval2timestring(from_time, &string);
        if (err) goto leave;
    }
    INSERT_STRING(request, "dmFromTime", string);
    free(string); string = NULL;

    if (to_time) {
        err = timeval2timestring(to_time, &string);
        if (err) goto leave;
    }
    INSERT_STRING(request, "dmToTime", string);
    free(string); string = NULL;

    if (outgoing_direction) {
        INSERT_LONGINT(request, "dmSenderOrgUnitNum",
                organization_unit_number, string);
    } else {
        INSERT_LONGINT(request, "dmRecipientOrgUnitNum",
                organization_unit_number, string);
    }

    if (status_filter > MESSAGESTATE_ANY) {
        isds_printf_message(context,
                _("Invalid message state filter value: %ld"), status_filter);
        err = IE_INVAL;
        goto leave;
    }
    INSERT_ULONGINTNOPTR(request, "dmStatusFilter", status_filter, string);

    if (offset > 0 ) {
        INSERT_ULONGINTNOPTR(request, "dmOffset", offset, string);
    } else {
        INSERT_STRING(request, "dmOffset", "1");
    }

    /* number 0 means no limit */
    if (number && *number == 0) {
        INSERT_STRING(request, "dmLimit", NULL);
    } else {
        INSERT_ULONGINT(request, "dmLimit", number, string);
    }


    isds_log(ILF_ISDS, ILL_DEBUG,
            (outgoing_direction) ?
                _("Sending GetListOfSentMessages request to ISDS\n") :
                _("Sending GetListOfReceivedMessages request to ISDS\n")
            );

    /* Sent request */
    err = isds(context, SERVICE_DM_INFO, request, &response);
    xmlFreeNode(request); request = NULL;
    
    if (err) {
        isds_log(ILF_ISDS, ILL_DEBUG,
                (outgoing_direction) ?
                    _("Processing ISDS response on GetListOfSentMessages "
                        "request failed\n") :
                    _("Processing ISDS response on GetListOfReceivedMessages "
                        "request failed\n")
                );
        goto leave;
    }

    /* Check for response status */
    err = isds_response_status(context, SERVICE_DM_INFO, response,
            &code, &message, NULL);
    if (err) {
        isds_log(ILF_ISDS, ILL_DEBUG,
                (outgoing_direction) ?
                    _("ISDS response on GetListOfSentMessages request "
                        "is missing status\n") :
                    _("ISDS response on GetListOfReceivedMessages request "
                        "is missing status\n")
                );
        goto leave;
    }

    /* Request processed, but nothing found */
    if (xmlStrcmp(code, BAD_CAST "0000")) {
        char *code_locale = utf82locale((char*)code);
        char *message_locale = utf82locale((char*)message);
        isds_log(ILF_ISDS, ILL_DEBUG,
                (outgoing_direction) ?
                    _("Server refused GetListOfSentMessages request "
                        "(code=%s, message=%s)\n") :
                    _("Server refused GetListOfReceivedMessages request "
                        "(code=%s, message=%s)\n"),
                code_locale, message_locale);
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
    if (register_namespaces(xpath_ctx, MESSAGE_NS_UNSIGNED)) {
        err = IE_ERROR;
        goto leave;
    }
    result = xmlXPathEvalExpression(
            (outgoing_direction) ?
                BAD_CAST "/isds:GetListOfSentMessagesResponse/"
                "isds:dmRecords/isds:dmRecord" :
                BAD_CAST "/isds:GetListOfReceivedMessagesResponse/"
                "isds:dmRecords/isds:dmRecord",
            xpath_ctx);
    if (!result) {
        err = IE_ERROR;
        goto leave;
    }
    
    /* Fill output arguments in */
    if (!xmlXPathNodeSetIsEmpty(result->nodesetval)) {
        struct isds_envelope *envelope;
        struct isds_list *item = NULL, *last_item = NULL;

        for (count = 0; count < result->nodesetval->nodeNr; count++) {
            /* Create new message  */
            item = calloc(1, sizeof(*item));
            if (!item) {
                err = IE_NOMEM;
                goto leave;
            }
            item->destructor = (void(*)(void**)) &isds_message_free;
            item->data = calloc(1, sizeof(struct isds_message));
            if (!item->data) {
                isds_list_free(&item);
                err = IE_NOMEM;
                goto leave;
            }

            /* Extract envelope data */
            xpath_ctx->node = result->nodesetval->nodeTab[count];
            envelope = NULL;
            err = extract_DmRecord(context, &envelope, xpath_ctx);
            if (err) {
                isds_list_free(&item);
                goto leave;
            }

            /* Attach extracted envelope */
            ((struct isds_message *) item->data)->envelope = envelope;

            /* Append new message into the list */
            if (!*messages) { 
                *messages = last_item = item;
            } else {
                last_item->next = item;
                last_item = item;
            }
        }
    }
    if (number) *number = count;

leave:
    if (err) {
        isds_list_free(messages);
    }

    free(string);
    xmlXPathFreeObject(result);
    xmlXPathFreeContext(xpath_ctx);

    free(code);
    free(message);
    xmlFreeDoc(response);
    xmlFreeNode(request);

    if (!err)
        isds_log(ILF_ISDS, ILL_DEBUG,
                (outgoing_direction) ?
                    _("GetListOfSentMessages request processed by server "
                        "successfully.\n") :
                    _("GetListOfReceivedMessages request processed by server "
                        "successfully.\n")
                );
    return err;
}


/* Get list of outgoing (already sent) messages.
 * Any criterion argument can be NULL, if you don't care about it.
 * @context is session context. Must not be NULL.
 * @from_time is minimal time and date of message sending inclusive.
 * @to_time is maximal time and date of message sending inclusive
 * @dmSenderOrgUnitNum is the same as isds_envelope.dmSenderOrgUnitNum
 * @status_filter is bit field of isds_message_status values. Use special
 * value MESSAGESTATE_ANY to signal you don't care. (It's defined as union of
 * all values, you can use bitwise arithmetic if you want.)
 * @offset is index of first message we are interested in. First message is 1.
 * Set to 0 (or 1) if you don't care.
 * @number is maximal length of list you want to get as input value, outputs
 * number of messages matching these criteria. Can be NULL if you don't care
 * (applies to output value either).
 * @messages is automatically reallocated list of isds_message's. Be ware that
 * it returns only brief overview (envelope and some other fields) about each
 * message, not the complete message. FIXME: Specify exact fields.
 * The list is sorted by delivery time in ascending order.
 * Use NULL if you don't care about the metadata (useful if you want to know
 * only the @number). If you provide &NULL, list will be allocated on heap,
 * if you provide pointer to non-NULL, list will be freed automacally at first.
 * Also in case of error the list will be NULLed.
 * @return IE_SUCCESS or appropriate error code. */
isds_error isds_get_list_of_sent_messages(struct isds_ctx *context,
        const struct timeval *from_time, const struct timeval *to_time,
        const long int *dmSenderOrgUnitNum, const unsigned int status_filter,
        const unsigned long int offset, unsigned long int *number,
        struct isds_list **messages) {

    return isds_get_list_of_messages(
            context, 1,
            from_time, to_time, dmSenderOrgUnitNum, status_filter,
            offset, number,
            messages);
}


/* Get list of incoming (addressed to you) messages.
 * Any criterion argument can be NULL, if you don't care about it.
 * @context is session context. Must not be NULL.
 * @from_time is minimal time and date of message sending inclusive.
 * @to_time is maximal time and date of message sending inclusive
 * @dmRecipientOrgUnitNum is the same as isds_envelope.dmRecipientOrgUnitNum
 * @status_filter is bit field of isds_message_status values. Use special
 * value MESSAGESTATE_ANY to signal you don't care. (It's defined as union of
 * all values, you can use bitwise arithmetic if you want.)
 * @offset is index of first message we are interested in. First message is 1.
 * Set to 0 (or 1) if you don't care.
 * @number is maximal length of list you want to get as input value, outputs
 * number of messages matching these criteria. Can be NULL if you don't care
 * (applies to output value either).
 * @messages is automatically reallocated list of isds_message's. Be ware that
 * it returns only brief overview (envelope and some other fields) about each
 * message, not the complete message. FIXME: Specify exact fields.
 * Use NULL if you don't care about the metadata (useful if you want to know
 * only the @number). If you provide &NULL, list will be allocated on heap,
 * if you provide pointer to non-NULL, list will be freed automacally at first.
 * Also in case of error the list will be NULLed.
 * @return IE_SUCCESS or appropriate error code. */
isds_error isds_get_list_of_received_messages(struct isds_ctx *context,
        const struct timeval *from_time, const struct timeval *to_time,
        const long int *dmRecipientOrgUnitNum,
        const unsigned int status_filter,
        const unsigned long int offset, unsigned long int *number,
        struct isds_list **messages) {

    return isds_get_list_of_messages(
            context, 0,
            from_time, to_time, dmRecipientOrgUnitNum, status_filter,
            offset, number,
            messages);
}


/* Build ISDS request of XSD tIDMessInput type, sent it and check for error
 * code
 * @context is session context
 * @service is ISDS WS service handler
 * @service_name is name of SERVICE_DM_OPERATIONS
 * @message_id is message ID to send as service argument to ISDS
 * @response is server SOAP body response as XML document
 * @code is ISDS status code
 * @status_message is ISDS status message
 * @return error coded from lower layer, context message will be set up
 * appropriately. */
static isds_error build_send_check_message_request(struct isds_ctx *context,
        const isds_service service, const xmlChar *service_name,
        const char *message_id,
        xmlDocPtr *response, xmlChar **code, xmlChar **status_message) {

    isds_error err = IE_SUCCESS;
    char *service_name_locale = NULL, *message_id_locale = NULL;
    xmlNodePtr request = NULL, node;
    xmlNsPtr isds_ns = NULL;

    if (!context) return IE_INVALID_CONTEXT;
    if (!service_name || !message_id) return IE_INVAL;
    if (!response || !code || !status_message) return IE_INVAL;

    /* Free output argument */
    xmlFreeDoc(*response);
    free(*code);
    free(*status_message);


    /* Check if connection is established
     * TODO: This check should be done donwstairs. */
    if (!context->curl) return IE_CONNECTION_CLOSED;

    service_name_locale = utf82locale((char*)service_name);
    message_id_locale = utf82locale(message_id);
    if (!service_name_locale || !message_id_locale) {
        err = IE_NOMEM;
        goto leave;
    }

    /* Build request */
    request = xmlNewNode(NULL, service_name);
    if (!request) {
        isds_printf_message(context,
                _("Could not build %s request"), service_name_locale);
        err = IE_ERROR;
        goto leave;
    }
    isds_ns = xmlNewNs(request, BAD_CAST ISDS_NS, NULL);
    if(!isds_ns) {
        isds_log_message(context, _("Could not create ISDS name space"));
        err = IE_ERROR;
        goto leave;
    }
    xmlSetNs(request, isds_ns);


    /* Add requested ID */
    err = validate_message_id_length(context, (xmlChar *) message_id);
    if (err) goto leave;
    INSERT_STRING(request, "dmID", message_id);


    isds_log(ILF_ISDS, ILL_DEBUG,
                _("Sending %s request for %s message ID to ISDS\n"),
                service_name_locale, message_id_locale);

    /* Send request */
    err = isds(context, service, request, response);
    xmlFreeNode(request); request = NULL;
    
    if (err) {
        isds_log(ILF_ISDS, ILL_DEBUG,
                    _("Processing ISDS response on %s request failed\n"),
                    service_name_locale);
        goto leave;
    }

    /* Check for response status */
    err = isds_response_status(context, service, *response,
            code, status_message, NULL);
    if (err) {
        isds_log(ILF_ISDS, ILL_DEBUG,
                    _("ISDS response on %s request is missing status\n"),
                    service_name_locale);
        goto leave;
    }

    /* Request processed, but nothing found */
    if (xmlStrcmp(*code, BAD_CAST "0000")) {
        char *code_locale = utf82locale((char*) *code);
        char *status_message_locale = utf82locale((char*) *status_message);
        isds_log(ILF_ISDS, ILL_DEBUG,
                    _("Server refused %s request for %s message ID "
                        "(code=%s, message=%s)\n"),
                service_name_locale, message_id_locale,
                code_locale, status_message_locale);
        isds_log_message(context, status_message_locale);
        free(code_locale);
        free(status_message_locale);
        err = IE_ISDS;
        goto leave;
    }

leave:
    free(message_id_locale);
    free(service_name_locale);
    xmlFreeNode(request);
    return err;
}


/* Download incoming message envelope identified by ID.
 * @context is session context
 * @message_id is message identifier (you can get them from
 * isds_get_list_of_received_messages())
 * @message is automatically reallocated message retrieved from ISDS.
 * It will miss documents per se. Use isds_get_received_message(), if you are
 * interrested in documents (content) too.
 * Returned hash and timestamp require documents to be verifiable. */
isds_error isds_get_received_envelope(struct isds_ctx *context,
        const char *message_id, struct isds_message **message) {

    isds_error err = IE_SUCCESS;
    xmlDocPtr response = NULL;
    xmlChar *code = NULL, *status_message = NULL;
    xmlXPathContextPtr xpath_ctx = NULL;
    xmlXPathObjectPtr result = NULL;

    if (!context) return IE_INVALID_CONTEXT;
   
    /* Free former message if any */
    if (message) isds_message_free(message);

    /* Do request and check for success */
    err = build_send_check_message_request(context, SERVICE_DM_INFO,
            BAD_CAST "MessageEnvelopeDownload", message_id,
            &response, &code, &status_message);
    if (err) goto leave;

    /* Extract data */
    xpath_ctx = xmlXPathNewContext(response);
    if (!xpath_ctx) {
        err = IE_ERROR;
        goto leave;
    }
    if (register_namespaces(xpath_ctx, MESSAGE_NS_UNSIGNED)) {
        err = IE_ERROR;
        goto leave;
    }
    result = xmlXPathEvalExpression(
            BAD_CAST "/isds:MessageEnvelopeDownloadResponse/"
                "isds:dmReturnedMessageEnvelope",
            xpath_ctx);
    if (!result) {
        err = IE_ERROR;
        goto leave;
    }
    /* Empty response */
    if (xmlXPathNodeSetIsEmpty(result->nodesetval)) {
        char *message_id_locale = utf82locale((char*) message_id);
        isds_printf_message(context,
                _("Server did not return any envelope for ID `%s' "
                    "on MessageEnvelopeDownload request"), message_id_locale);
        free(message_id_locale);
        err = IE_ISDS;
        goto leave;
    }
    /* More envelops */
    if (result->nodesetval->nodeNr > 1) {
        char *message_id_locale = utf82locale((char*) message_id);
        isds_printf_message(context,
                _("Server did return more envelopes for ID `%s' "
                    "on MessageEnvelopeDownload request"), message_id_locale);
        free(message_id_locale);
        err = IE_ISDS;
        goto leave;
    }
    /* One message */
    xpath_ctx->node = result->nodesetval->nodeTab[0];

    /* Extract the envelope (= message without documents, hence 0) */
    err = extract_TReturnedMessage(context, 0, message, xpath_ctx);
    if (err) goto leave;

     /* Save XML blob */
    err = serialize_subtree(context, xpath_ctx->node, &(*message)->raw,
            &(*message)->raw_length);

leave:
    if (err) {
        isds_message_free(message);
    }

    xmlXPathFreeObject(result);
    xmlXPathFreeContext(xpath_ctx);

    free(code);
    free(status_message);
    xmlFreeDoc(response);

    if (!err)
        isds_log(ILF_ISDS, ILL_DEBUG,
                    _("MessageEnvelopeDownload request processed by server "
                        "successfully.\n")
                );
    return err;
}


/* Dwwnload incoming message identified by ID.
 * @context is session context
 * @message_id is message identifier (you can get them from
 * isds_get_list_of_received_messages())
 * @message is automatically reallocated message retrieved from ISDS */
isds_error isds_get_received_message(struct isds_ctx *context,
        const char *message_id, struct isds_message **message) {

    isds_error err = IE_SUCCESS;
    xmlDocPtr response = NULL;
    xmlChar *code = NULL, *status_message = NULL;
    xmlXPathContextPtr xpath_ctx = NULL;
    xmlXPathObjectPtr result = NULL;

    if (!context) return IE_INVALID_CONTEXT;
   
    /* Free former message if any */
    if (message) isds_message_free(message);

    /* Do request and check for success */
    err = build_send_check_message_request(context, SERVICE_DM_OPERATIONS,
            BAD_CAST "MessageDownload", message_id,
            &response, &code, &status_message);
    if (err) goto leave;

    /* Extract data */
    xpath_ctx = xmlXPathNewContext(response);
    if (!xpath_ctx) {
        err = IE_ERROR;
        goto leave;
    }
    if (register_namespaces(xpath_ctx, MESSAGE_NS_UNSIGNED)) {
        err = IE_ERROR;
        goto leave;
    }
    result = xmlXPathEvalExpression(
            BAD_CAST "/isds:MessageDownloadResponse/isds:dmReturnedMessage",
            xpath_ctx);
    if (!result) {
        err = IE_ERROR;
        goto leave;
    }
    /* Empty response */
    if (xmlXPathNodeSetIsEmpty(result->nodesetval)) {
        char *message_id_locale = utf82locale((char*) message_id);
        isds_printf_message(context,
                _("Server did not return any message for ID `%s' "
                    "on MessageDownload request"), message_id_locale);
        free(message_id_locale);
        err = IE_ISDS;
        goto leave;
    }
    /* More messages */
    if (result->nodesetval->nodeNr > 1) {
        char *message_id_locale = utf82locale((char*) message_id);
        isds_printf_message(context,
                _("Server did return more messages for ID `%s' "
                    "on MessageDownload request"), message_id_locale);
        free(message_id_locale);
        err = IE_ISDS;
        goto leave;
    }
    /* One message */
    xpath_ctx->node = result->nodesetval->nodeTab[0];

    /* Extract the message */
    err = extract_TReturnedMessage(context, 1, message, xpath_ctx);
    if (err) goto leave;

     /* Save XML blob */
    err = serialize_subtree(context, xpath_ctx->node, &(*message)->raw,
            &(*message)->raw_length);

leave:
    if (err) {
        isds_message_free(message);
    }

    xmlXPathFreeObject(result);
    xmlXPathFreeContext(xpath_ctx);

    free(code);
    free(status_message);
    xmlFreeDoc(response);

    if (!err)
        isds_log(ILF_ISDS, ILL_DEBUG,
                    _("MessageDownload request processed by server "
                        "successfully.\n")
                );
    return err;
}


/* Load signed message from buffer.
 * @context is session context
 * @outgoing is true if message is outgoing, false if message is incoming
 * @buffer is DER encoded PKCS#7 structure with signed message. You can
 * retrieve such data from message->raw after calling
 * isds_get_signed_{received,sent}_message().
 * @length is length of buffer in bytes.
 * @message is automatically reallocated message parsed from @buffer.
 * @strategy selects how buffer will be attached into raw isds_message member.
 * */
isds_error isds_load_signed_message(struct isds_ctx *context,
        const _Bool outgoing, const void *buffer, const size_t length,
        struct isds_message **message, const isds_buffer_strategy strategy) {

    isds_error err = IE_SUCCESS;
    xmlDocPtr message_doc = NULL;
    xmlXPathContextPtr xpath_ctx = NULL;
    xmlXPathObjectPtr result = NULL;
    void *xml_stream = NULL;
    size_t xml_stream_length = 0;

    if (!context) return IE_INVALID_CONTEXT;
    if (!message) return IE_INVAL;
    isds_message_free(message);
    if (!buffer) return IE_INVAL;


    /* Extract message from PKCS#7 structure */
    err = extract_cms_data(context, buffer, length,
            &xml_stream, &xml_stream_length);
    if (err) goto leave;

    isds_log(ILF_ISDS, ILL_DEBUG, (outgoing) ?
                _("Signed outgoing message content:\n%.*s\nEnd of message\n") :
                _("Signed incoming message content:\n%.*s\nEnd of message\n"),
            xml_stream_length, xml_stream);

    /* Convert extracted messages XML stream into XPath context */
    message_doc = xmlParseMemory(xml_stream, xml_stream_length);
    if (!message_doc) {
        err = IE_XML;
        goto leave;
    }
    xpath_ctx = xmlXPathNewContext(message_doc);
    if (!xpath_ctx) {
        err = IE_ERROR;
        goto leave;
    }
    /* XXX: Name spaces mangled for outgoing direction:
     * http://isds.czechpoint.cz/v20/SentMessage:
     *
     * <q:MessageDownloadResponse
     *      xmlns:q="http://isds.czechpoint.cz/v20/SentMessage">
     *   <q:dmReturnedMessage>
     *      <p:dmDm xmlns:p="http://isds.czechpoint.cz/v20">
     *          <p:dmID>151916</p:dmID>
     *          ...
     *      </p:dmDm>
     *      <q:dmHash algorithm="SHA-1">...</q:dmHash>
     *      ...
     *      <q:dmAttachmentSize>260</q:dmAttachmentSize>
     *   </q:dmReturnedMessage>
     * </q:MessageDownloadResponse>
     *
     * XXX: Name spaces mangled for incoming direction:
     * http://isds.czechpoint.cz/v20/message:
     *
     * <q:MessageDownloadResponse
     *      xmlns:q="http://isds.czechpoint.cz/v20/message">
     *   <q:dmReturnedMessage>
     *      <p:dmDm xmlns:p="http://isds.czechpoint.cz/v20">
     *          <p:dmID>151916</p:dmID>
     *          ...
     *      </p:dmDm>
     *      <q:dmHash algorithm="SHA-1">...</q:dmHash>
     *      ...
     *      <q:dmAttachmentSize>260</q:dmAttachmentSize>
     *   </q:dmReturnedMessage>
     * </q:MessageDownloadResponse>
     *
     * Stupidity of ISDS developers is unlimited */
    if (register_namespaces(xpath_ctx, (outgoing) ?
                MESSAGE_NS_SIGNED_OUTGOING : MESSAGE_NS_SIGNED_INCOMING)) {
        err = IE_ERROR;
        goto leave;
    }
    /* XXX: Embeded message XML document is always rooted as
     * /sisds:MessageDownloadResponse (even outgoing message). */
    result = xmlXPathEvalExpression( 
            BAD_CAST "/sisds:MessageDownloadResponse/sisds:dmReturnedMessage",
            xpath_ctx);
    if (!result) {
        err = IE_ERROR;
        goto leave;
    }
    /* Empty embedded message */
    if (xmlXPathNodeSetIsEmpty(result->nodesetval)) {
        isds_printf_message(context,
                _("XML document embedded into PKCS#7 structure is not "
                    "sisds:dmReturnedMessage document"));
        err = IE_ISDS;
        goto leave;
    }
    /* More embedded messages */
    if (result->nodesetval->nodeNr > 1) {
        isds_printf_message(context,
                _("Embeded XML document into PKCS#7 structure has more "
                    "root isds:dmReturnedMessage elements"));
        err = IE_ISDS;
        goto leave;
    }
    /* One embedded message */
    xpath_ctx->node = result->nodesetval->nodeTab[0];

    /* Extract the message */
    err = extract_TReturnedMessage(context, 1, message, xpath_ctx);
    if (err) goto leave;

    /* Append raw CMS structure into message */
    switch (strategy) {
        case BUFFER_DONT_STORE:
            break;
        case BUFFER_COPY:
            (*message)->raw = malloc(length);
            if (!(*message)->raw) {
                err = IE_NOMEM;
                goto leave;
            }
            memcpy((*message)->raw, buffer, length);
            (*message)->raw_length = length;
            break;
        case BUFFER_MOVE:
            (*message)->raw = (void *) buffer;
            (*message)->raw_length = length;
            break;
        default:
            err = IE_ENUM;
            goto leave;
    }


leave:
    if (err) {
        if (*message && strategy == BUFFER_MOVE) (*message)->raw = NULL;
        isds_message_free(message);
    }

    xmlFreeDoc(message_doc);
    cms_data_free(xml_stream);
    xmlXPathFreeObject(result);
    xmlXPathFreeContext(xpath_ctx);

    if (!err)
        isds_log(ILF_ISDS, ILL_DEBUG,
                _("Signed message loaded successfully.\n"));
    return err;
}


/* Download signed incoming/outgoing message identified by ID.
 * @context is session context
 * @output is true for outging message, false for incoming message
 * @message_id is message identifier (you can get them from
 * isds_get_list_of_{sent,received}_messages())
 * @message is automatically reallocated message retrieved from ISDS. The raw
 * memeber will be filled with PKCS#7 structure in DER format. */
_hidden isds_error isds_get_signed_message(struct isds_ctx *context,
        const _Bool outgoing, const char *message_id,
        struct isds_message **message) {

    isds_error err = IE_SUCCESS;
    xmlDocPtr response = NULL;
    xmlChar *code = NULL, *status_message = NULL;
    xmlXPathContextPtr xpath_ctx = NULL;
    xmlXPathObjectPtr result = NULL;
    char *encoded_structure = NULL;
    void *raw = NULL;
    size_t raw_length = 0;

    if (!context) return IE_INVALID_CONTEXT;
    if (!message) return IE_INVAL;
    isds_message_free(message);

    /* Do request and check for success */
    err = build_send_check_message_request(context, SERVICE_DM_OPERATIONS,
            (outgoing) ? BAD_CAST "SignedSentMessageDownload" :
                BAD_CAST "SignedMessageDownload",
            message_id, &response, &code, &status_message);
    if (err) goto leave;

    /* Extract data */
    xpath_ctx = xmlXPathNewContext(response);
    if (!xpath_ctx) {
        err = IE_ERROR;
        goto leave;
    }
    if (register_namespaces(xpath_ctx, MESSAGE_NS_UNSIGNED)) {
        err = IE_ERROR;
        goto leave;
    }
    result = xmlXPathEvalExpression(
            (outgoing) ? BAD_CAST
                "/isds:SignedSentMessageDownloadResponse/isds:dmSignature" :
                BAD_CAST "/isds:SignedMessageDownloadResponse/isds:dmSignature",
            xpath_ctx);
    if (!result) {
        err = IE_ERROR;
        goto leave;
    }
    /* Empty response */
    if (xmlXPathNodeSetIsEmpty(result->nodesetval)) {
        char *message_id_locale = utf82locale((char*) message_id);
        isds_printf_message(context,
                (outgoing) ?
                    _("Server did not return any message for ID `%s' "
                        "on SignedSentMessageDownload request") :
                    _("Server did not return any message for ID `%s' "
                        "on SignedMessageDownload request"),
                message_id_locale);
        free(message_id_locale);
        err = IE_ISDS;
        goto leave;
    }
    /* More reponses */
    if (result->nodesetval->nodeNr > 1) {
        char *message_id_locale = utf82locale((char*) message_id);
        isds_printf_message(context,
                (outgoing) ?
                    _("Server did return more messages for ID `%s' "
                        "on SignedSentMessageDownload request") :
                    _("Server did return more messages for ID `%s' "
                        "on SignedMessageDownload request"),
                message_id_locale);
        free(message_id_locale);
        err = IE_ISDS;
        goto leave;
    }
    /* One response */
    xpath_ctx->node = result->nodesetval->nodeTab[0];

    /* Extract PKCS#7 structure */
    EXTRACT_STRING(".", encoded_structure);
    if (!encoded_structure) {
        isds_log_message(context, _("dmSignature element is empty"));
    }

    /* Here we have message as standalone CMS in encoded_structure.
     * We don't need any other data, free them: */
    xmlXPathFreeObject(result); result = NULL;
    xmlXPathFreeContext(xpath_ctx); xpath_ctx = NULL;
    zfree(code);
    zfree(status_message);
    xmlFreeDoc(response); response = NULL;


    /* Decode PKCS#7 to DER format */
    raw_length = b64decode(encoded_structure, &raw);
    if (raw_length == (size_t) -1) {
        isds_log_message(context,
                _("Error while Base64-decoding PKCS#7 structure"));
        err = IE_ERROR;
        goto leave;
    }
    zfree(encoded_structure);
  
    /* Parse message */
    err = isds_load_signed_message(context, outgoing, raw, raw_length,
            message, BUFFER_MOVE);
    if (err) goto leave;

    raw = NULL;

leave:
    if (err) {
        isds_message_free(message);
    }

    free(encoded_structure);
    xmlXPathFreeObject(result);
    xmlXPathFreeContext(xpath_ctx);
    free(raw);

    free(code);
    free(status_message);
    xmlFreeDoc(response);

    if (!err)
        isds_log(ILF_ISDS, ILL_DEBUG,
                    (outgoing) ?
                        _("SignedSentMessageDownload request processed by server "
                            "successfully.\n") :
                        _("SignedMessageDownload request processed by server "
                            "successfully.\n")
                );
    return err;
}


/* Download signed incoming message identified by ID.
 * @context is session context
 * @message_id is message identifier (you can get them from
 * isds_get_list_of_received_messages())
 * @message is automatically reallocated message retrieved from ISDS. The raw
 * memeber will be filled with PKCS#7 structure in DER format. */
isds_error isds_get_signed_received_message(struct isds_ctx *context,
        const char *message_id, struct isds_message **message) {
    return isds_get_signed_message(context, 0, message_id, message);
}


/* Download signed outgoing message identified by ID.
 * @context is session context
 * @message_id is message identifier (you can get them from
 * isds_get_list_of_sent_messages())
 * @message is automatically reallocated message retrieved from ISDS. The raw
 * memeber will be filled with PKCS#7 structure in DER format. */
isds_error isds_get_signed_sent_message(struct isds_ctx *context,
        const char *message_id, struct isds_message **message) {
    return isds_get_signed_message(context, 1, message_id, message);
}


/* Retrieve hash of message identified by ID stored in ISDS.
 * @context is session context
 * @message_id is message identifier
 * @hash is automatically reallocated message hash downloaded from ISDS.
 * Message must exist in system and must not be deleted. */
isds_error isds_download_message_hash(struct isds_ctx *context,
        const char *message_id, struct isds_hash **hash) {

    isds_error err = IE_SUCCESS;
    xmlDocPtr response = NULL;
    xmlChar *code = NULL, *status_message = NULL;
    xmlXPathContextPtr xpath_ctx = NULL;
    xmlXPathObjectPtr result = NULL;

    if (!context) return IE_INVALID_CONTEXT;
   
    isds_hash_free(hash);

    err = build_send_check_message_request(context, SERVICE_DM_INFO,
            BAD_CAST "VerifyMessage", message_id,
            &response, &code, &status_message);
    if (err) goto leave;


    /* Extract data */
    xpath_ctx = xmlXPathNewContext(response);
    if (!xpath_ctx) {
        err = IE_ERROR;
        goto leave;
    }
    if (register_namespaces(xpath_ctx, MESSAGE_NS_UNSIGNED)) {
        err = IE_ERROR;
        goto leave;
    }
    result = xmlXPathEvalExpression(
            BAD_CAST "/isds:VerifyMessageResponse",
            xpath_ctx);
    if (!result) {
        err = IE_ERROR;
        goto leave;
    }
    /* Empty response */
    if (xmlXPathNodeSetIsEmpty(result->nodesetval)) {
        char *message_id_locale = utf82locale((char*) message_id);
        isds_printf_message(context,
                _("Server did not return any response for ID `%s' "
                    "on VerifyMessage request"), message_id_locale);
        free(message_id_locale);
        err = IE_ISDS;
        goto leave;
    }
    /* More responses */
    if (result->nodesetval->nodeNr > 1) {
        char *message_id_locale = utf82locale((char*) message_id);
        isds_printf_message(context,
                _("Server did return more responses for ID `%s' "
                    "on VerifyMessage request"), message_id_locale);
        free(message_id_locale);
        err = IE_ISDS;
        goto leave;
    }
    /* One response */
    xpath_ctx->node = result->nodesetval->nodeTab[0];

    /* Extract the hash */
    err = find_and_extract_DmHash(context, hash, xpath_ctx);

leave:
    if (err) {
        isds_hash_free(hash);
    }

    xmlXPathFreeObject(result);
    xmlXPathFreeContext(xpath_ctx);

    free(code);
    free(status_message);
    xmlFreeDoc(response);

    if (!err)
        isds_log(ILF_ISDS, ILL_DEBUG,
                    _("VerifyMessage request processed by server "
                        "successfully.\n")
                );
    return err;
}


/* Mark message as read. This is a transactional commit function to acknoledge
 * to ISDS the message has been downloaded and processed by client properly.
 * @context is session context
 * @message_id is message identifier. */
isds_error isds_mark_message_read(struct isds_ctx *context,
        const char *message_id) {

    isds_error err = IE_SUCCESS;
    xmlDocPtr response = NULL;
    xmlChar *code = NULL, *status_message = NULL;

    if (!context) return IE_INVALID_CONTEXT;
   
    /* Do request and check for success */
    err = build_send_check_message_request(context, SERVICE_DM_INFO,
            BAD_CAST "MarkMessageAsDownloaded", message_id,
            &response, &code, &status_message);

    free(code);
    free(status_message);
    xmlFreeDoc(response);

    if (!err)
        isds_log(ILF_ISDS, ILL_DEBUG,
                    _("MarkMessageAsDownloaded request processed by server "
                        "successfully.\n")
                );
    return err;
}


#undef INSERT_STRING_ATTRIBUTE
#undef INSERT_ULONGINTNOPTR
#undef INSERT_ULONGINT
#undef INSERT_LONGINT
#undef INSERT_BOOLEAN
#undef INSERT_STRING
#undef EXTRACT_STRING_ATTRIBUTE
#undef EXTRACT_ULONGINT
#undef EXTRACT_LONGINT
#undef EXTRACT_BOOLEAN
#undef EXTRACT_STRING


/* Compute hash of message from raw representation and store it into envelope.
 * Original hash structure will be destroyed in envelope.
 * @context is session context
 * @message is message carrying raw XML message blob
 * @algorithm is desired hash algorithm to use */
isds_error isds_compute_message_hash(struct isds_ctx *context,
        struct isds_message *message, const isds_hash_algorithm algorithm) {
    isds_error err = IE_SUCCESS;
    xmlDocPtr message_doc = NULL;
    xmlXPathContextPtr xpath_ctx = NULL;
    xmlXPathObjectPtr result = NULL;
    char *buffer = 0;
    size_t length;
    struct isds_hash *new_hash = NULL;
    

    if (!context) return IE_INVALID_CONTEXT;
    if (!message) return IE_INVAL;
    
    if (!message->raw) {
        isds_log_message(context,
                _("Message does not carry raw XML representation"));
        return IE_INVAL;
    }

    /* Parse raw message */
    message_doc = xmlParseMemory(message->raw, message->raw_length);
    if (!message_doc) {
        isds_log_message(context,
                _("Message does not carry well-formed XML representation"));
        err = IE_XML;
        goto leave;
    }

    /* Find dmDM element */
    xpath_ctx = xmlXPathNewContext(message_doc);
    if (!xpath_ctx) {
        err = IE_ERROR;
        goto leave;
    }
    if (register_namespaces(xpath_ctx, MESSAGE_NS_UNSIGNED)) {
        err = IE_ERROR;
        goto leave;
    }
    result = xmlXPathEvalExpression(
            BAD_CAST "/isds:dmReturnedMessage/isds:dmDm", xpath_ctx);
    if (!result) {
        err = IE_ERROR;
        goto leave;
    }
    if (xmlXPathNodeSetIsEmpty(result->nodesetval)) {
        isds_log_message(context,
                _("Raw message does not contain isds:dmDm element"));
        err = IE_XML;
        goto leave;
    }
    if (result->nodesetval->nodeNr > 1) {
        isds_log_message(context,
                _("Raw message contains more isds:dmDm elements"));
        err = IE_XML;
        goto leave;
    }
    xpath_ctx->node = result->nodesetval->nodeTab[0];

    /* XXX: We need all childern of isds:dmDm: elements, text nodes, PIs,
     * CDATA, comments. Is asterisk sufficient? */
    result = xmlXPathEvalExpression(BAD_CAST "*", xpath_ctx);
    if (!result) {
        err = IE_ERROR;
        goto leave;
    }

    /* Extract dmDM content as bit stream */
    err = dump_nodeset(context, message_doc, result->nodesetval,
            (void**) &buffer, &length);
    if (err) goto leave;

    /* Free memory */
    xmlXPathFreeObject(result); result = NULL;
    xmlXPathFreeContext(xpath_ctx); xpath_ctx = NULL;
    xmlFreeDoc(message_doc); message_doc = NULL;

    /* TODO: Compute hash */
    new_hash = calloc(1, sizeof(*new_hash));
    if (!new_hash) {
        err = IE_NOMEM;
        goto leave;
    }
    new_hash->algorithm = algorithm;
    err = compute_hash(buffer, length, new_hash);
    if (err) {
        isds_log_message(context, _("Could not compute message hash"));
        goto leave;
    }

    /* Save cumputed hash */
    if (!message->envelope) {
        message->envelope = calloc(1, sizeof(*message->envelope));
        if (!message->envelope) {
            err = IE_NOMEM;
            goto leave;
        }
    }
    isds_hash_free(&message->envelope->hash);
    message->envelope->hash = new_hash;
    
leave:
    if (err) {
        isds_hash_free(&new_hash);
    }

    free(buffer);
    xmlXPathFreeObject(result);
    xmlXPathFreeContext(xpath_ctx);
    xmlFreeDoc(message_doc);
    return err;
}


/* Search for document by document ID in list of documents. IDs are compared
 * as UTF-8 string.
 * @documents is list of isds_documents
 * @id is document identifier
 * @return first matching document or NULL. */
const struct isds_document *isds_find_document_by_id(
        const struct isds_list *documents, const char *id) {
    const struct isds_list *item;
    const struct isds_document *document;

    for (item = documents; item; item = item->next) {
        document = (struct isds_document *) item->data;
        if (!document) continue;

        if (!xmlStrcmp((xmlChar *) id, (xmlChar *) document->dmFileGuid))
            return document;
    }

    return NULL;
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


/* Makes known all relevant namespaces to given XPath context
 * @xpat_ctx is XPath context
 * @message_ns selects propper message name space. Unsisnged and signed
 * messages differs.
 * prefix and to URI ISDS_NS */
_hidden isds_error register_namespaces(xmlXPathContextPtr xpath_ctx,
        const message_ns_type message_ns) {
    const xmlChar *message_namespace = NULL;

    if (!xpath_ctx) return IE_ERROR;

    switch(message_ns) {
        case MESSAGE_NS_UNSIGNED:
            message_namespace = BAD_CAST ISDS_NS; break;
        case MESSAGE_NS_SIGNED_INCOMING:
            message_namespace = BAD_CAST SISDS_INCOMING_NS; break;
        case MESSAGE_NS_SIGNED_OUTGOING:
            message_namespace = BAD_CAST SISDS_OUTGOING_NS; break;
        default:
            return IE_ENUM;
    }

    if (xmlXPathRegisterNs(xpath_ctx, BAD_CAST "soap", BAD_CAST SOAP_NS))
        return IE_ERROR;
    if (xmlXPathRegisterNs(xpath_ctx, BAD_CAST "isds", BAD_CAST ISDS_NS))
        return IE_ERROR;
    if (xmlXPathRegisterNs(xpath_ctx, BAD_CAST "sisds", message_namespace))
        return IE_ERROR;
    if (xmlXPathRegisterNs(xpath_ctx, BAD_CAST "xs", BAD_CAST SCHEMA_NS))
        return IE_ERROR;
    return IE_SUCCESS;
}

