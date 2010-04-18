#define _XOPEN_SOURCE 500   /* strdup from string.h, strptime from time.h */
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdarg.h>
#include <ctype.h>
#include <stdint.h>
#include "isds_priv.h"
#include "utils.h"
#include "soap.h"
#include "validator.h"
#include "crypto.h"
#include <gpg-error.h> /* Because of ksba or gpgme */
#include "physxml.h"

/* Locators */
/* Base URL of production ISDS instance */
const char isds_locator[] = "https://www.mojedatovaschranka.cz/";

/* Base URL of production ISDS instance */
const char isds_testing_locator[] = "https://www.czebox.cz/";


/* Deallocate structure isds_pki_credentials and NULL it.
 * Passphrase is discarded.
 * @pki  credentials to to free */
void isds_pki_credentials_free(struct isds_pki_credentials **pki) {
    if(!pki || !*pki) return;

    free((*pki)->engine);
    free((*pki)->certificate);
    free((*pki)->key);

    if ((*pki)->passphrase) {
        memset((*pki)->passphrase, 0, strlen((*pki)->passphrase));
        free((*pki)->passphrase);
    }
    
    zfree((*pki));
}


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

/* Deallocate structure isds_DbUserInfo recursively and NULL it */
void isds_DbUserInfo_free(struct isds_DbUserInfo **db_user_info) {
    if (!db_user_info || !*db_user_info) return;

    free((*db_user_info)->userID);
    free((*db_user_info)->userType);
    free((*db_user_info)->userPrivils);
    isds_PersonName_free(&((*db_user_info)->personName));
    isds_Address_free(&((*db_user_info)->address));
    free((*db_user_info)->biDate);
    free((*db_user_info)->ic);
    free((*db_user_info)->firmName);
    free((*db_user_info)->caStreet);
    free((*db_user_info)->caCity);
    free((*db_user_info)->caZipCode);
    
    zfree(*db_user_info);
}


/* Deallocate struct isds_event recursively and NULL it */
void isds_event_free(struct isds_event **event) {
    if (!event || !*event) return;

    free((*event)->time);
    free((*event)->type);
    free((*event)->description);
    zfree(*event);
}


/* Deallocate struct isds_envelope recursively and NULL it */
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
    free((*envelope)->dmType);

    free((*envelope)->dmOrdinal);
    free((*envelope)->dmMessageStatus);
    free((*envelope)->dmDeliveryTime);
    free((*envelope)->dmAcceptanceTime);
    isds_hash_free(&(*envelope)->hash);
    free((*envelope)->timestamp);
    isds_list_free(&(*envelope)->events);

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


/* Deallocate struct isds_message recursively and NULL it */
void isds_message_free(struct isds_message **message) {
    if (!message || !*message) return;

    free((*message)->raw);
    isds_envelope_free(&((*message)->envelope));
    isds_list_free(&((*message)->documents));

    free(*message);
    *message = NULL;
}


/* Deallocate struct isds_document recursively and NULL it */
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


/* Deallocate struct isds_message_copy recursively and NULL it */
void isds_message_copy_free(struct isds_message_copy **copy) {
    if (!copy || !*copy) return;

    free((*copy)->dbIDRecipient);
    free((*copy)->dmRecipientOrgUnit);
    free((*copy)->dmRecipientOrgUnitNum);
    free((*copy)->dmToHands);

    free((*copy)->dmStatus);
    free((*copy)->dmID);

    zfree(*copy);
}


/* Deallocate struct isds_approval recursively and NULL it */
void isds_approval_free(struct isds_approval **approval) {
    if (!approval || !*approval) return;

    free((*approval)->refference);

    zfree(*approval);
}


/* *DUP_OR_ERROR macros needs error label */
#define STRDUP_OR_ERROR(new, template) { \
    if (!template) { \
        (new) = NULL; \
    } else { \
        (new) = strdup(template); \
        if (!new) goto error; \
    } \
}

#define FLATDUP_OR_ERROR(new, template) { \
    if (!template) { \
        (new) = NULL; \
    } else { \
        (new) = malloc(sizeof(*(new))); \
        if (!new) goto error; \
        memcpy((new), (template), sizeof(*(template))); \
    } \
}

/* Copy structure isds_pki_credentials recursively. */
struct isds_pki_credentials *isds_pki_credentials_duplicate(
        const struct isds_pki_credentials *template) {
    struct isds_pki_credentials *new = NULL;

    if(!template) return NULL;

    new = calloc(1, sizeof(*new));
    if (!new) return NULL;

    STRDUP_OR_ERROR(new->engine, template->engine);
    new->certificate_format = template->certificate_format;
    STRDUP_OR_ERROR(new->certificate, template->certificate);
    new->key_format = template->key_format;
    STRDUP_OR_ERROR(new->key, template->key);
    STRDUP_OR_ERROR(new->passphrase, template->passphrase);
    
    return new;

error:
    isds_pki_credentials_free(&new);
    return NULL;
}


/* Copy structure isds_PersonName recursively */
struct isds_PersonName *isds_PersonName_duplicate(
        const struct isds_PersonName *template) {
    struct isds_PersonName *new = NULL;

    if (!template) return NULL;

    new = calloc(1, sizeof(*new));
    if (!new) return NULL;

    STRDUP_OR_ERROR(new->pnFirstName, template->pnFirstName);
    STRDUP_OR_ERROR(new->pnMiddleName, template->pnMiddleName);
    STRDUP_OR_ERROR(new->pnLastName, template->pnLastName);
    STRDUP_OR_ERROR(new->pnLastNameAtBirth, template->pnLastNameAtBirth);

    return new;
    
error:
    isds_PersonName_free(&new);
    return NULL;
}

    
/* Copy structure isds_BirthInfo recursively */
static struct isds_BirthInfo *isds_BirthInfo_duplicate(
        const struct isds_BirthInfo *template) {
    struct isds_BirthInfo *new = NULL;

    if (!template) return NULL;

    new = calloc(1, sizeof(*new));
    if (!new) return NULL;

    FLATDUP_OR_ERROR(new->biDate, template->biDate);
    STRDUP_OR_ERROR(new->biCity, template->biCity);
    STRDUP_OR_ERROR(new->biCounty, template->biCounty);
    STRDUP_OR_ERROR(new->biState, template->biState);
    
    return new;
    
error:
    isds_BirthInfo_free(&new);
    return NULL;
}


/* Copy structure isds_Address recursively */
struct isds_Address *isds_Address_duplicate(
        const struct isds_Address *template) {
    struct isds_Address *new = NULL;

    if (!template) return NULL;

    new = calloc(1, sizeof(*new));
    if (!new) return NULL;

    STRDUP_OR_ERROR(new->adCity, template->adCity);
    STRDUP_OR_ERROR(new->adStreet, template->adStreet);
    STRDUP_OR_ERROR(new->adNumberInStreet, template->adNumberInStreet);
    STRDUP_OR_ERROR(new->adNumberInMunicipality,
            template->adNumberInMunicipality);
    STRDUP_OR_ERROR(new->adZipCode, template->adZipCode);
    STRDUP_OR_ERROR(new->adState, template->adState);
    
    return new;
    
error:
    isds_Address_free(&new);
    return NULL;
}


/* Copy structure isds_DbOwnerInfo recursively */
struct isds_DbOwnerInfo *isds_DbOwnerInfo_duplicate(
        const struct isds_DbOwnerInfo *template) {
    struct isds_DbOwnerInfo *new = NULL;
    if (!template) return NULL;

    new = calloc(1, sizeof(*new));
    if (!new) return NULL;

    STRDUP_OR_ERROR(new->dbID, template->dbID);
    FLATDUP_OR_ERROR(new->dbType, template->dbType);
    STRDUP_OR_ERROR(new->ic, template->ic);

    if (template->personName) {
        if (!(new->personName =
                    isds_PersonName_duplicate(template->personName)))
            goto error;
    }

    STRDUP_OR_ERROR(new->firmName, template->firmName);

    if (template->birthInfo) {
        if (!(new->birthInfo =
                    isds_BirthInfo_duplicate(template->birthInfo)))
            goto error;
    }

    if (template->address) {
        if (!(new->address = isds_Address_duplicate(template->address)))
            goto error;
    }
    
    STRDUP_OR_ERROR(new->nationality, template->nationality);
    STRDUP_OR_ERROR(new->email, template->email);
    STRDUP_OR_ERROR(new->telNumber, template->telNumber);
    STRDUP_OR_ERROR(new->identifier, template->identifier);
    STRDUP_OR_ERROR(new->registryCode, template->registryCode);
    FLATDUP_OR_ERROR(new->dbState, template->dbState);
    FLATDUP_OR_ERROR(new->dbEffectiveOVM, template->dbEffectiveOVM);
    FLATDUP_OR_ERROR(new->dbOpenAddressing, template->dbOpenAddressing);

    return new;
    
error:
    isds_DbOwnerInfo_free(&new);
    return NULL;
}


/* Copy structure isds_DbUserInfo recursively */
struct isds_DbUserInfo *isds_DbUserInfo_duplicate(
        const struct isds_DbUserInfo *template) {
    struct isds_DbUserInfo *new = NULL;
    if (!template) return NULL;

    new = calloc(1, sizeof(*new));
    if (!new) return NULL;

    STRDUP_OR_ERROR(new->userID, template->userID);
    FLATDUP_OR_ERROR(new->userType, template->userType);
    FLATDUP_OR_ERROR(new->userPrivils, template->userPrivils);

    if (template->personName) {
        if (!(new->personName =
                    isds_PersonName_duplicate(template->personName)))
            goto error;
    }

    if (template->address) {
        if (!(new->address = isds_Address_duplicate(template->address)))
            goto error;
    }
    
    FLATDUP_OR_ERROR(new->biDate, template->biDate);
    STRDUP_OR_ERROR(new->ic, template->ic);
    STRDUP_OR_ERROR(new->firmName, template->firmName);
    STRDUP_OR_ERROR(new->caStreet, template->caStreet);
    STRDUP_OR_ERROR(new->caCity, template->caCity);
    STRDUP_OR_ERROR(new->caZipCode, template->caZipCode);

    return new;
    
error:
    isds_DbUserInfo_free(&new);
    return NULL;
}

#undef FLATDUP_OR_ERROR
#undef STRDUP_OR_ERROR 


/* Initialize ISDS library.
 * Global function, must be called before other functions.
 * If it failes you can not use ISDS library and must call isds_cleanup() to
 * free partially inititialized global variables. */
isds_error isds_init(void) {
    /* NULL global variables */
    log_facilities = ILF_ALL;
    log_level = ILL_WARNING;
    log_callback = NULL;
    log_callback_data = NULL;

#if ENABLE_NLS
    /* Initialize gettext */
    bindtextdomain(PACKAGE, LOCALEDIR);
#endif

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
    if (init_gpgme(&version_gpgme)) {
        isds_log(ILF_ISDS, ILL_CRIT,
                _("GPGME library initialization failed\n"));
        return IE_ERROR;
    }

    /* Initialize gcrypt */
    if (init_gcrypt(&version_gcrypt)) {
        isds_log(ILF_ISDS, ILL_CRIT,
                _("gcrypt library initialization failed\n"));
        return IE_ERROR;
    }

    /* This can _exit() current program. Find not so assertive check. */
    LIBXML_TEST_VERSION;

    /* Check expat */
    if (init_expat(&version_expat)) {
        isds_log(ILF_ISDS, ILL_CRIT,
                _("expat library initialization failed\n"));
        return IE_ERROR;
    }

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


/* Return version string of this library. Version of dependecies can be
 * embedded. Do no try to parse it. You must free it. */
char *isds_version(void) {
    char *buffer = NULL;

    isds_asprintf(&buffer, _("%s (%s, GPGME %s, gcrypt %s, %s, libxml2 %s)"),
            PACKAGE_VERSION, curl_version(), version_gpgme, version_gcrypt,
            version_expat, xmlParserVersion);
    return buffer;
}


/* Return text description of ISDS error */
const char *isds_strerror(const isds_error error) {
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
        case IE_2SMALL:
            return(_("Too small")); break;
        case IE_NOTUNIQ:
            return(_("Value not unique")); break;
        case IE_NOTEQUAL:
            return(_("Values not uqual")); break;
        case IE_PARTIAL_SUCCESS:
            return(_("Some suboperations failed")); break;
        case IE_ABORTED:
            return(_("Operation aborted")); break;
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


/* Close possibly opened connection to Czech POINT document deposit without
 * reseting long_message buffer.
 * XXX: Do not use czp_close_connection() if you do not want to destroy log
 * message.
 * @context is Czech POINT session context. */
static isds_error czp_do_close_connection(struct isds_ctx *context) {
    if (!context) return IE_INVALID_CONTEXT;
    close_connection(context);
    return IE_SUCCESS;
}


/* Discard credentials.
 * Only that. It does not cause log out, connection close or similar. */
static isds_error discard_credentials(struct isds_ctx *context) {
    if(!context) return IE_INVALID_CONTEXT;

    if (context->username) {
        memset(context->username, 0, strlen(context->username));
        zfree(context->username);
    }
    if (context->password) {
        memset(context->password, 0, strlen(context->password));
        zfree(context->password);
    }
    isds_pki_credentials_free(&context->pki_credentials);

    return IE_SUCCESS;
}


/* Destroy ISDS context and free memmory.
 * @context will be NULLed on success. */
isds_error isds_ctx_free(struct isds_ctx **context) {
    if (!context || !*context) {
        return IE_INVALID_CONTEXT;
    }
  
    /* Discard credentials and close connection */
    switch ((*context)->type) {
        case CTX_TYPE_NONE: break;
        case CTX_TYPE_ISDS: isds_logout(*context); break;
        case CTX_TYPE_CZP:
        case CTX_TYPE_TESTING_REQUEST_COLLECTOR:
                            czp_do_close_connection(*context); break;
    }

    /* For sure */
    discard_credentials(*context);

    /* Free other structures */
    free((*context)->tls_verify_server);
    free((*context)->tls_ca_file);
    free((*context)->tls_ca_dir);
    free((*context)->tls_crl_file);
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


/* Register callback function libisds calls when new global log message is
 * produced by library. Library logs to stderr by default.
 * @callback is function provided by application libsds will call. See type
 * defition for @callback argument explanation. Pass NULL to revert logging to
 * default behaviour.
 * @data is application specific data @callback gets as last argument */
void isds_set_log_callback(isds_log_callback callback, void *data) {
    log_callback = callback;
    log_callback_data = data;
}


/* Log @message in class @facility with log @level into global log. @message
 * is printf(3) formating string, variadic arguments may be neccessary.
 * For debugging purposes. */
_hidden isds_error isds_log(const isds_log_facility facility,
        const isds_log_level level, const char *message, ...) {
    va_list ap;
    char *buffer = NULL;
    int length;

    if (level > log_level) return IE_SUCCESS;
    if (!(log_facilities & facility)) return IE_SUCCESS;
    if (!message) return IE_INVAL;

    if (log_callback) {
        /* Pass message to application supplied callback function */
        va_start(ap, message);
        length = isds_vasprintf(&buffer, message, ap);
        va_end(ap);

        if (length == -1) {
            return IE_ERROR;
        }
        if (length > 0) {
            log_callback(facility, level, buffer, length, log_callback_data);
        }
        free(buffer);
    } else {
        /* Default: Log it to stderr */
        va_start(ap, message);
        vfprintf(stderr, message, ap);
        va_end(ap);
        /* Line buffered printf is default.
         * fflush(stderr);*/
    }

    return IE_SUCCESS;
}


/* Set timeout in miliseconds for each network job like connecting to server
 * or sending message. Use 0 to disable timeout limits. */
isds_error isds_set_timeout(struct isds_ctx *context,
        const unsigned int timeout) {
    if (!context) return IE_INVALID_CONTEXT;
    zfree(context->long_message);

    context->timeout = timeout;

    if (context->curl) {
        CURLcode curl_err;

        curl_err = curl_easy_setopt(context->curl, CURLOPT_NOSIGNAL, 1);
        if (!curl_err) 
#if HAVE_DECL_CURLOPT_TIMEOUT_MS /* Since curl-7.16.2 */
            curl_err = curl_easy_setopt(context->curl, CURLOPT_TIMEOUT_MS,
                    context->timeout);
#else
            curl_err = curl_easy_setopt(context->curl, CURLOPT_TIMEOUT,
                    context->timeout / 1000);
#endif /* not HAVE_DECL_CURLOPT_TIMEOUT_MS */
        if (curl_err) return IE_ERROR;
    }

    return IE_SUCCESS;
}


/* Register callback function libisds calls periodocally during HTTP data
 * transfer.
 * @context is session context
 * @callback is function provided by application libsds will call. See type
 * defition for @callback argument explanation.
 * @data is application specific data @callback gets as last argument */
isds_error isds_set_progress_callback(struct isds_ctx *context,
        isds_progress_callback callback, void *data) {
    if (!context) return IE_INVALID_CONTEXT;
    zfree(context->long_message);

    context->progress_callback = callback;
    context->progress_callback_data = data;

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
    zfree(context->long_message);

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
        case ITLS_CRL_FILE:
#if HAVE_DECL_CURLOPT_CRLFILE /* Since curl-7.19.0 */
            REPLACE_VA_STRING(context->tls_crl_file);
#else
            isds_log_message(ILF_SEC, ILL_ERR,
                    _("Curl library does not support CRL definition"));
            err = IE_NOTSUP;
#endif  /* not HAVE_DECL_CURLOPT_CRLFILE */
            break;

        default:
            err = IE_ENUM; goto leave;
    }

#undef REPLACE_VA_STRING

leave:
    va_end(ap);
    return err;
}


/* Connect and log in into ISDS server.
 * All required arguments will be copied, you do not have to keep them after
 * that.
 * ISDS supports four different authentication methods. Exact method is
 * selected on @username, @passwors and @pki_credentials arguments:
 *   - If @pki_credentials == NULL, @username and @password must be supplied
 *   - If @pki_credentials != NULL, then
 *      - If @username == NULL, only certificate will be used
 *      - If @username != NULL, then
 *          - If @password == NULL, then certificate will be used and
 *            @username shifts meaning to box ID. This is used for hosted
 *            services.
 *          - Otherwise all three arguments will be used.
 * Please note, that differen cases requires different certificate type
 * (system qualified one or commercial non qualified one). This library does
 * not check such political issues. Please see ISDS Specification for more
 * details.
 * @url is base address of ISDS web service. Pass NULL or extern isds_locator
 * variable to use production ISDS instance. You can pass extern
 * isds_testing_locator variable to select testing instance. 
 * @username is user name of ISDS user or box ID
 * @password is user's secret password
 * @pki_credentials defines public key cryptographic material to use in client
 * authentication. */
isds_error isds_login(struct isds_ctx *context, const char *url,
        const char *username, const char *password,
        const struct isds_pki_credentials *pki_credentials) {
    isds_error err = IE_NOT_LOGGED_IN;
    isds_error soap_err;
    xmlNsPtr isds_ns = NULL;
    xmlNodePtr request = NULL;
    xmlNodePtr response = NULL;

    if (!context) return IE_INVALID_CONTEXT;
    zfree(context->long_message);

    /* Close connection if already logged in */
    if (context->curl) {
        close_connection(context);
    }

    /* Default locator is offical system */
    if (!url) url = isds_locator;

    /* Store configuration */
    context->type = CTX_TYPE_ISDS;
    zfree(context->url);

    /* Mangle base URI according requested authentication method */
    if (!pki_credentials) {
        isds_log(ILF_SEC, ILL_INFO,
                _("Selected authentication method: no certificate, "
                    "username and password\n"));
        if (!username || !password) {
            isds_log_message(context,
                    _("Both username and password must be supplied"));
            return IE_INVAL;
        }
        context->url = strdup(url);
    } else {
        if (!username) {
            isds_log(ILF_SEC, ILL_INFO,
                    _("Selected authentication method: system certificate, "
                        "no username and no password\n"));
            password = NULL;
            context->url = astrcat(url, "cert/");
        } else {
            if (!password) {
                isds_log(ILF_SEC, ILL_INFO,
                        _("Selected authentication method: system certificate, "
                            "box ID and no password\n"));
                context->url = astrcat(url, "hspis/");
            } else {
                isds_log(ILF_SEC, ILL_INFO,
                        _("Selected authentication method: commercial "
                            "certificate, username and password\n"));
                context->url = astrcat(url, "cert/");
            }
        }
    }
    if (!(context->url))
        return IE_NOMEM;

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
    if (username) context->username = strdup(username);
    if (password) context->password = strdup(password);
    context->pki_credentials = isds_pki_credentials_duplicate(pki_credentials);
    if ((username && !context->username) || (password && !context->password) ||
            (pki_credentials && !context->pki_credentials)) {
        discard_credentials(context);
        xmlFreeNode(request);
        return IE_NOMEM;
    }

    isds_log(ILF_ISDS, ILL_DEBUG, _("Logging user %s into server %s\n"),
            username, url);

    /* Send login request */
    soap_err = soap(context, "DS/dz", request, &response, NULL, NULL);
   
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
    zfree(context->long_message);

    /* Close connection */
    if (context->curl) {
        close_connection(context);

        /* Discard credentials for sure. They should not survive isds_login(),
         * even successful .*/
        discard_credentials(context);
        zfree(context->url);

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
    zfree(context->long_message);

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
    soap_err = soap(context, "DS/dz", request, &response, NULL, NULL);
   
    /* Destroy login request */
    xmlFreeNode(request);

    if (soap_err) {
        isds_log(ILF_ISDS, ILL_DEBUG,
                _("ISDS server could not be contacted\n"));
        xmlFreeNodeList(response);
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
    zfree(context->long_message);

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
    err = isds(context, SERVICE_DM_OPERATIONS, request, &response, NULL, NULL);
   
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
 * @buffer is automatically reallocated buffer where serialize to
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

#if 0
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
#endif

#if 0
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
    xmlSaveCtxtPtr save_ctx = NULL;
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
    if (xmlSubstituteEntitiesDefault(1)) {
        isds_log_message(context, _("Could not disable attribute escaping"));
        err = IE_ERROR;
        goto leave;
    }
    /* Last argument means:
     * 0                    to not format the XML tree
     * XML_SAVE_NO_EMPTY    ISDS does not produce shorten tags */
    save_ctx = xmlSaveToBuffer(xml_buffer, "UTF-8",
            XML_SAVE_NO_DECL|XML_SAVE_NO_EMPTY|XML_SAVE_NO_XHTML);
    if (!save_ctx) {
        isds_log_message(context, _("Could not create XML serializer"));
        err = IE_ERROR;
        goto leave;
    }
    /*if (xmlSaveSetAttrEscape(save_ctx, NULL)) {
        isds_log_message(context, _("Could not disable attribute escaping"));
        err = IE_ERROR;
        goto leave;
    }*/

   
    /* Itearate over all nodes */
    for (int i = 0; i < nodeset->nodeNr; i++) {
        /* Serialize node.
         * XXX: xmlNodeDump() appends to xml_buffer. */
        /*if (-1 ==
                xmlNodeDump(xml_buffer, document, nodeset->nodeTab[i], 0, 0)) {
                */
        /* XXX: According LibXML documentation, this function does not return
         * meaningfull value yet */
        xmlSaveTree(save_ctx, nodeset->nodeTab[i]);
        if (-1 == xmlSaveFlush(save_ctx)) {
            isds_log_message(context,
                    _("Could not serialize XML subtree"));
            err = IE_ERROR;
            goto leave;
        }
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
    return err;
}
#endif


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


/* Convert UTF-8 @string represantion of ISDS userType to enum @type */
static isds_error string2isds_UserType(xmlChar *string, isds_UserType *type) {
    if (!string || !type) return IE_INVAL;

    if (!xmlStrcmp(string, BAD_CAST "PRIMARY_USER"))
        *type = USERTYPE_PRIMARY;
    else if (!xmlStrcmp(string, BAD_CAST "ENTRUSTED_USER"))
        *type = USERTYPE_ENTRUSTED;
    else if (!xmlStrcmp(string, BAD_CAST "ADMINISTRATOR"))
        *type = USERTYPE_ADMINISTRATOR;
    else if (!xmlStrcmp(string, BAD_CAST "OFFICIAL"))
        *type = USERTYPE_OFFICIAL;
    else
        return IE_ENUM;
    return IE_SUCCESS;
}


/* Convert ISDS userType enum @type to UTF-8 string.
 * @Return pointer to static string, or NULL if unkwnow enum value */
static const xmlChar *isds_UserType2string(const isds_UserType type) {
    switch(type) {
        case USERTYPE_PRIMARY: return(BAD_CAST "PRIMARY_USER"); break;
        case USERTYPE_ENTRUSTED: return(BAD_CAST "ENTRUSTED_USER"); break;
        case USERTYPE_ADMINISTRATOR: return(BAD_CAST "ADMINISTRATOR"); break;
        case USERTYPE_OFFICIAL: return(BAD_CAST "OFFICIAL"); break;
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
    else if (!xmlStrcmp(string, BAD_CAST "SHA-224"))
        *algorithm = HASH_ALGORITHM_SHA_224;
    else if (!xmlStrcmp(string, BAD_CAST "SHA-256"))
        *algorithm = HASH_ALGORITHM_SHA_256;
    else if (!xmlStrcmp(string, BAD_CAST "SHA-384"))
        *algorithm = HASH_ALGORITHM_SHA_384;
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

    if (*number < 1 || *number > 10) {
        isds_printf_message(context, _("Invalid messsage status value: %lu"),
                *number);
        return IE_ENUM;
    }

    *status = malloc(sizeof(**status));
    if (!*status) return IE_NOMEM;

    **status = 1 << *number;
    return IE_SUCCESS;
}


/* Convert event description string into isds_event memebers type and
 * description
 * @string is raw event decsription starting with event prefix
 * @event is structure where to store type and stripped description to
 * @return standard error code, unknown prefix is not classified as an error.
 * */
static isds_error eventstring2event(const xmlChar *string,
        struct isds_event* event) {
    const xmlChar *known_prefixes[] = {
        BAD_CAST "EV1:",
        BAD_CAST "EV2:",
        BAD_CAST "EV3:",
        BAD_CAST "EV4:"
    };
    const isds_event_type types[] = {
        EVENT_ACCEPTED_BY_RECIPIENT,
        EVENT_ACCEPTED_BY_FICTION,
        EVENT_UNDELIVERABLE,
        EVENT_COMMERCIAL_ACCEPTED
    };
    unsigned int index;
    size_t length;

    if (!string || !event) return IE_INVAL;

    if (!event->type) {
        event->type = malloc(sizeof(*event->type));
        if (!(event->type)) return IE_NOMEM;
    }
    zfree(event->description);

    for (index = 0; index < sizeof(known_prefixes)/sizeof(known_prefixes[0]);
            index++) {
        length = xmlUTF8Strlen(known_prefixes[index]);

        if (!xmlStrncmp(string, known_prefixes[index], length)) {
            /* Prefix is known */
            *event->type = types[index];

            /* Strip prefix from description and spaces */
            /* TODO: Recognize all wite spaces from UCS blank class and
             * operate on UTF-8 chars. */
            for (; string[length] != '\0' && string[length] == ' '; length++);
            event->description = strdup((char *) (string + length));
            if (!(event->description)) return IE_NOMEM;

            return IE_SUCCESS;
        }
    }
    
    /* Unknown event prefix.
     * XSD allows any string */
    char *string_locale = utf82locale((char *) string);
    isds_log(ILF_ISDS, ILL_WARNING,
            _("Uknown delivery info event prefix: %s\n"), string_locale);
    free(string_locale);

    *event->type = EVENT_UKNOWN;
    event->description = strdup((char *) string);
    if (!(event->description)) return IE_NOMEM;

    return IE_SUCCESS;
}


/* Following EXTRACT_* macros expects @result, @xpath_ctx, @err, @context
 * and leave lable */
#define EXTRACT_STRING(element, string) { \
    result = xmlXPathEvalExpression(BAD_CAST element "/text()", xpath_ctx); \
    if (!result) { \
        err = IE_ERROR; \
        goto leave; \
    } \
    if (!xmlXPathNodeSetIsEmpty(result->nodesetval)) { \
        if (result->nodesetval->nodeNr > 1) { \
            isds_printf_message(context, _("Multiple %s element"), element); \
            err = IE_ERROR; \
            goto leave; \
        } \
        (string) = (char *) \
            xmlXPathCastNodeSetToString(result->nodesetval); \
        if (!(string)) { \
            err = IE_ERROR; \
            goto leave; \
        } \
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
                        _("%s value is not valid boolean: %s"), \
                        element, string_locale); \
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
                        _("%s is not valid integer: %s"), \
                        element, string_locale); \
                free(string_locale); \
                free(string); \
                err = IE_ISDS; \
                goto leave; \
            } \
             \
            if (number == LONG_MIN || number == LONG_MAX) { \
                char *string_locale = utf82locale((char *)string); \
                isds_printf_message(context, \
                        _("%s value out of range of long int: %s"), \
                        element, string_locale); \
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
                        _("%s is not valid integer: %s"), \
                        element, string_locale); \
                free(string_locale); \
                free(string); \
                err = IE_ISDS; \
                goto leave; \
            } \
             \
            if (number == LONG_MIN || number == LONG_MAX) { \
                char *string_locale = utf82locale((char *)string); \
                isds_printf_message(context, \
                        _("%s value out of range of long int: %s"), \
                        element, string_locale); \
                free(string_locale); \
                free(string); \
                err = IE_ERROR; \
                goto leave; \
            } \
             \
            free(string); string = NULL; \
            if (number < 0) { \
                isds_printf_message(context, \
                        _("%s value is negative: %ld"), element, number); \
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

#define EXTRACT_STRING_ATTRIBUTE(attribute, string, required) { \
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
    } \
}


#define INSERT_STRING_WITH_NS(parent, ns, element, string) \
    { \
        node = xmlNewTextChild(parent, ns, BAD_CAST (element), \
                (xmlChar *) (string)); \
        if (!node) { \
            isds_printf_message(context, \
                    _("Could not add %s child to %s element"), \
                    element, (parent)->name); \
            err = IE_ERROR; \
            goto leave; \
        } \
    }

#define INSERT_STRING(parent, element, string) \
    { INSERT_STRING_WITH_NS(parent, NULL, element, string) }

#define INSERT_SCALAR_BOOLEAN(parent, element, boolean) \
    { \
        if (boolean) { INSERT_STRING(parent, element, "true"); } \
        else { INSERT_STRING(parent, element, "false"); } \
    }

#define INSERT_BOOLEAN(parent, element, booleanPtr) \
    { \
        if (booleanPtr) { \
            INSERT_SCALAR_BOOLEAN(parent, element, (*(booleanPtr))); \
        } else { \
            INSERT_STRING(parent, element, NULL); \
        } \
    }

#define INSERT_LONGINT(parent, element, longintPtr, buffer) { \
    if ((longintPtr)) { \
        /* FIXME: locale sensitive */ \
        if (-1 == isds_asprintf((char **)&(buffer), "%ld", *(longintPtr))) { \
            err = IE_NOMEM; \
            goto leave; \
        } \
        INSERT_STRING(parent, element, buffer) \
        free(buffer); (buffer) = NULL; \
    } else { INSERT_STRING(parent, element, NULL) } \
}

#define INSERT_ULONGINT(parent, element, ulongintPtr, buffer) { \
    if ((ulongintPtr)) { \
        /* FIXME: locale sensitive */ \
        if (-1 == isds_asprintf((char **)&(buffer), "%lu", *(ulongintPtr))) { \
            err = IE_NOMEM; \
            goto leave; \
        } \
        INSERT_STRING(parent, element, buffer) \
        free(buffer); (buffer) = NULL; \
    } else { INSERT_STRING(parent, element, NULL) } \
}

#define INSERT_ULONGINTNOPTR(parent, element, ulongint, buffer) \
    { \
        /* FIXME: locale sensitive */ \
        if (-1 == isds_asprintf((char **)&(buffer), "%lu", ulongint)) { \
            err = IE_NOMEM; \
            goto leave; \
        } \
        INSERT_STRING(parent, element, buffer) \
        free(buffer); (buffer) = NULL; \
    }

#define INSERT_STRING_ATTRIBUTE(parent, attribute, string) \
    { \
        attribute_node = xmlNewProp((parent), BAD_CAST (attribute), \
                (xmlChar *) (string)); \
        if (!attribute_node) { \
            isds_printf_message(context, _("Could not add %s " \
                        "attribute to %s element"), \
                    (attribute), (parent)->name); \
            err = IE_ERROR; \
            goto leave; \
        } \
    }

#define CHECK_FOR_STRING_LENGTH(string, minimum, maximum, name) { \
    if (string) { \
        int length = xmlUTF8Strlen((xmlChar *) (string)); \
        if (length > (maximum)) { \
            isds_printf_message(context, \
                    ngettext("%s has more than %d characters", \
                        "%s has more than %d characters", (maximum)), \
                    (name), (maximum)); \
            err = IE_2BIG; \
            goto leave; \
        } \
        if (length < (minimum)) { \
            isds_printf_message(context, \
                    ngettext("%s has less than %d characters", \
                        "%s has less than %d characters", (minimum)), \
                    (name), (minimum)); \
            err = IE_2SMALL; \
            goto leave; \
        } \
    } \
}

#define INSERT_ELEMENT(child, parent, element) \
    { \
        (child) = xmlNewChild((parent), NULL, BAD_CAST (element), NULL); \
        if (!(child)) { \
            isds_printf_message(context, \
                    _("Could not add %s child to %s element"), \
                    (element), (parent)->name); \
            err = IE_ERROR; \
            goto leave; \
        } \
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
                _("%s element contains multiple %s children"),
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



/* Find and convert XSD:gPersonName group in current node into structure
 * @context is ISDS context
 * @personName is automically reallocated person name structure. If no member
 * value is found, will be freed.
 * @xpath_ctx is XPath context with current node as parent for XSD:gPersonName
 * elements
 * In case of error @personName will be freed. */
static isds_error extract_gPersonName(struct isds_ctx *context,
        struct isds_PersonName **personName, xmlXPathContextPtr xpath_ctx) {
    isds_error err = IE_SUCCESS;
    xmlXPathObjectPtr result = NULL;

    if (!context) return IE_INVALID_CONTEXT;
    if (!personName) return IE_INVAL;
    isds_PersonName_free(personName);
    if (!xpath_ctx) return IE_INVAL;


    *personName = calloc(1, sizeof(**personName));
    if (!*personName) {
        err = IE_NOMEM;
        goto leave;
    }

    EXTRACT_STRING("isds:pnFirstName", (*personName)->pnFirstName);
    EXTRACT_STRING("isds:pnMiddleName", (*personName)->pnMiddleName);
    EXTRACT_STRING("isds:pnLastName", (*personName)->pnLastName);
    EXTRACT_STRING("isds:pnLastNameAtBirth", (*personName)->pnLastNameAtBirth);

    if (!(*personName)->pnFirstName && !(*personName)->pnMiddleName &&
            !(*personName)->pnLastName && !(*personName)->pnLastNameAtBirth)
        isds_PersonName_free(personName);

leave:
    if (err) isds_PersonName_free(personName);
    xmlXPathFreeObject(result);
    return err;
}


/* Find and convert XSD:gAddress group in current node into structure
 * @context is ISDS context
 * @address is automically reallocated address structure. If no member
 * value is found, will be freed.
 * @xpath_ctx is XPath context with current node as parent for XSD:gAddress
 * elements
 * In case of error @address will be freed. */
static isds_error extract_gAddress(struct isds_ctx *context,
        struct isds_Address **address, xmlXPathContextPtr xpath_ctx) {
    isds_error err = IE_SUCCESS;
    xmlXPathObjectPtr result = NULL;

    if (!context) return IE_INVALID_CONTEXT;
    if (!address) return IE_INVAL;
    isds_Address_free(address);
    if (!xpath_ctx) return IE_INVAL;


    *address = calloc(1, sizeof(**address));
    if (!*address) {
        err = IE_NOMEM;
        goto leave;
    }

    EXTRACT_STRING("isds:adCity", (*address)->adCity);
    EXTRACT_STRING("isds:adStreet", (*address)->adStreet);
    EXTRACT_STRING("isds:adNumberInStreet", (*address)->adNumberInStreet);
    EXTRACT_STRING("isds:adNumberInMunicipality",
            (*address)->adNumberInMunicipality);
    EXTRACT_STRING("isds:adZipCode", (*address)->adZipCode);
    EXTRACT_STRING("isds:adState", (*address)->adState);

    if (!(*address)->adCity && !(*address)->adStreet &&
            !(*address)->adNumberInStreet &&
            !(*address)->adNumberInMunicipality &&
            !(*address)->adZipCode && !(*address)->adState)
        isds_Address_free(address);

leave:
    if (err) isds_Address_free(address);
    xmlXPathFreeObject(result);
    return err;
}


/* Find and convert isds:biDate element in current node into structure
 * @context is ISDS context
 * @biDate is automically reallocated birth date structure. If no member
 * value is found, will be freed.
 * @xpath_ctx is XPath context with current node as parent for isds:biDate
 * element
 * In case of error @biDate will be freed. */
static isds_error extract_BiDate(struct isds_ctx *context,
        struct tm **biDate, xmlXPathContextPtr xpath_ctx) {
    isds_error err = IE_SUCCESS;
    xmlXPathObjectPtr result = NULL;
    char *string = NULL;

    if (!context) return IE_INVALID_CONTEXT;
    if (!biDate) return IE_INVAL;
    zfree(*biDate);
    if (!xpath_ctx) return IE_INVAL;

    EXTRACT_STRING("isds:biDate", string);
    if (string) {
        *biDate = calloc(1, sizeof(**biDate));
        if (!*biDate) {
            err = IE_NOMEM;
            goto leave;
        }
        err = datestring2tm((xmlChar *)string, *biDate);
        if (err) {
            if (err == IE_NOTSUP) {
                err = IE_ISDS;
                char *string_locale = utf82locale(string);
                isds_printf_message(context,
                        _("Invalid isds:biDate value: %s"), string_locale);
                free(string_locale);
            }
            goto leave;
        }
    }

leave:
    if (err) zfree(*biDate);
    free(string);
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
            zfree((*db_owner_info)->dbType);
            if (err == IE_ENUM) {
                err = IE_ISDS;
                char *string_locale = utf82locale(string);
                isds_printf_message(context, _("Unknown isds:dbType: %s"), 
                    string_locale);
                free(string_locale);
            }
            goto leave;
        }
        zfree(string);
    }

    EXTRACT_STRING("isds:ic", (*db_owner_info)->ic);

    err = extract_gPersonName(context, &(*db_owner_info)->personName,
            xpath_ctx);
    if (err) goto leave;

    EXTRACT_STRING("isds:firmName", (*db_owner_info)->firmName);

    (*db_owner_info)->birthInfo =
        calloc(1, sizeof(*((*db_owner_info)->birthInfo)));
    if (!(*db_owner_info)->birthInfo) {
        err = IE_NOMEM;
        goto leave;
    }
    err = extract_BiDate(context, &(*db_owner_info)->birthInfo->biDate,
            xpath_ctx);
    if (err) goto leave;
    EXTRACT_STRING("isds:biCity", (*db_owner_info)->birthInfo->biCity);
    EXTRACT_STRING("isds:biCounty", (*db_owner_info)->birthInfo->biCounty);
    EXTRACT_STRING("isds:biState", (*db_owner_info)->birthInfo->biState);
    if (!(*db_owner_info)->birthInfo->biDate &&
            !(*db_owner_info)->birthInfo->biCity &&
            !(*db_owner_info)->birthInfo->biCounty &&
            !(*db_owner_info)->birthInfo->biState)
        isds_BirthInfo_free(&(*db_owner_info)->birthInfo);

    err = extract_gAddress(context, &(*db_owner_info)->address, xpath_ctx);
    if (err) goto leave;

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


/* Insert struct isds_DbOwnerInfo data (box description) into XML tree
 * @context is sesstion context
 * @owner is libsids structure with box description
 * @db_owner_info is XML element of XSD:tDbOwnerInfo */
static isds_error insert_DbOwnerInfo(struct isds_ctx *context,
        const struct isds_DbOwnerInfo *owner, xmlNodePtr db_owner_info) {

    isds_error err = IE_SUCCESS;
    xmlNodePtr node;
    xmlChar *string = NULL;

    if (!context) return IE_INVALID_CONTEXT;
    if (!owner || !db_owner_info) return IE_INVAL;


    /* Build XSD:tDbOwnerInfo */
    CHECK_FOR_STRING_LENGTH(owner->dbID, 0, 7, "dbID")
    INSERT_STRING(db_owner_info, "dbID", owner->dbID);

    /* dbType */
    if (owner->dbType) {
        const xmlChar *type_string = isds_DbType2string(*(owner->dbType));
        if (!type_string) {
            isds_printf_message(context, _("Invalid dbType value: %d"),
                    *(owner->dbType));
            err = IE_ENUM;
            goto leave;
        }
        INSERT_STRING(db_owner_info, "dbType", type_string);
    }
    INSERT_STRING(db_owner_info, "ic", owner->ic);
    if (owner->personName) {
        INSERT_STRING(db_owner_info, "pnFirstName",
                owner->personName->pnFirstName);
        INSERT_STRING(db_owner_info, "pnMiddleName",
                owner->personName->pnMiddleName);
        INSERT_STRING(db_owner_info, "pnLastName",
                owner->personName->pnLastName);
        INSERT_STRING(db_owner_info, "pnLastNameAtBirth",
                owner->personName->pnLastNameAtBirth);
    }
    INSERT_STRING(db_owner_info, "firmName", owner->firmName);
    if (owner->birthInfo) {
        if (owner->birthInfo->biDate) {
            if (!tm2datestring(owner->birthInfo->biDate, &string))
                INSERT_STRING(db_owner_info, "biDate", string);
            free(string); string = NULL;
        }
        INSERT_STRING(db_owner_info, "biCity", owner->birthInfo->biCity);
        INSERT_STRING(db_owner_info, "biCounty", owner->birthInfo->biCounty);
        INSERT_STRING(db_owner_info, "biState", owner->birthInfo->biState);
    }
    if (owner->address) {
        INSERT_STRING(db_owner_info, "adCity", owner->address->adCity);
        INSERT_STRING(db_owner_info, "adStreet", owner->address->adStreet);
        INSERT_STRING(db_owner_info, "adNumberInStreet",
                owner->address->adNumberInStreet);
        INSERT_STRING(db_owner_info, "adNumberInMunicipality",
                owner->address->adNumberInMunicipality);
        INSERT_STRING(db_owner_info, "adZipCode", owner->address->adZipCode);
        INSERT_STRING(db_owner_info, "adState", owner->address->adState);
    }
    INSERT_STRING(db_owner_info, "nationality", owner->nationality);
    INSERT_STRING(db_owner_info, "email", owner->email);
    INSERT_STRING(db_owner_info, "telNumber", owner->telNumber);

    CHECK_FOR_STRING_LENGTH(owner->identifier, 0, 20, "identifier")
    INSERT_STRING(db_owner_info, "identifier", owner->identifier);

    CHECK_FOR_STRING_LENGTH(owner->registryCode, 0, 5, "registryCode")
    INSERT_STRING(db_owner_info, "registryCode", owner->registryCode);

    INSERT_LONGINT(db_owner_info, "dbState", owner->dbState, string);

    INSERT_BOOLEAN(db_owner_info, "dbEffectiveOVM", owner->dbEffectiveOVM);
    INSERT_BOOLEAN(db_owner_info, "dbOpenAddressing",
            owner->dbOpenAddressing);

leave:
    free(string);
    return err;
}


/* Convert XSD:tDbUserInfo XML tree into structure
 * @context is ISDS context
 * @db_user_info is automically reallocated user info structure
 * @xpath_ctx is XPath context with current node as XSD:tDbUserInfo element
 * In case of error @db_user_info will be freed. */
static isds_error extract_DbUserInfo(struct isds_ctx *context,
        struct isds_DbUserInfo **db_user_info, xmlXPathContextPtr xpath_ctx) {
    isds_error err = IE_SUCCESS;
    xmlXPathObjectPtr result = NULL;
    char *string = NULL;

    if (!context) return IE_INVALID_CONTEXT;
    if (!db_user_info) return IE_INVAL;
    isds_DbUserInfo_free(db_user_info);
    if (!xpath_ctx) return IE_INVAL;


    *db_user_info = calloc(1, sizeof(**db_user_info));
    if (!*db_user_info) {
        err = IE_NOMEM;
        goto leave;
    }

    EXTRACT_STRING("isds:userID", (*db_user_info)->userID);
    
    EXTRACT_STRING("isds:userType", string);
    if (string) {
        (*db_user_info)->userType =
            calloc(1, sizeof(*((*db_user_info)->userType)));
        if (!(*db_user_info)->userType) {
            err = IE_NOMEM;
            goto leave;
        }
        err = string2isds_UserType((xmlChar *)string,
                (*db_user_info)->userType);
        if (err) {
            zfree((*db_user_info)->userType);
            if (err == IE_ENUM) {
                err = IE_ISDS;
                char *string_locale = utf82locale(string);
                isds_printf_message(context,
                        _("Unknown isds:userType value: %s"), string_locale);
                free(string_locale);
            }
            goto leave;
        }
        zfree(string);
    }

    EXTRACT_LONGINT("isds:userPrivils", (*db_user_info)->userPrivils, 0); 

    (*db_user_info)->personName =
        calloc(1, sizeof(*((*db_user_info)->personName)));
    if (!(*db_user_info)->personName) {
        err = IE_NOMEM;
        goto leave;
    }

    err = extract_gPersonName(context, &(*db_user_info)->personName,
            xpath_ctx);
    if (err) goto leave;

    err = extract_gAddress(context, &(*db_user_info)->address, xpath_ctx);
    if (err) goto leave;

    err = extract_BiDate(context, &(*db_user_info)->biDate, xpath_ctx);
    if (err) goto leave;

    EXTRACT_STRING("isds:ic", (*db_user_info)->ic);
    EXTRACT_STRING("isds:firmName", (*db_user_info)->firmName);

    EXTRACT_STRING("isds:caStreet", (*db_user_info)->caStreet);
    EXTRACT_STRING("isds:caCity", (*db_user_info)->caCity);
    EXTRACT_STRING("isds:caZipCode", (*db_user_info)->caZipCode);

leave:
    if (err) isds_DbUserInfo_free(db_user_info);
    free(string);
    xmlXPathFreeObject(result);
    return err;
}


/* Insert struct isds_DbUserInfo data (user description) into XML tree
 * @context is sesstion context
 * @user is libsids structure with user description
 * @db_user_info is XML element of XSD:tDbUserInfo */
static isds_error insert_DbUserInfo(struct isds_ctx *context,
        const struct isds_DbUserInfo *user, xmlNodePtr db_user_info) {

    isds_error err = IE_SUCCESS;
    xmlNodePtr node;
    xmlChar *string = NULL;

    if (!context) return IE_INVALID_CONTEXT;
    if (!user || !db_user_info) return IE_INVAL;

    /* Build XSD:tDbUserInfo */
    if (user->personName) {
        INSERT_STRING(db_user_info, "pnFirstName",
                user->personName->pnFirstName);
        INSERT_STRING(db_user_info, "pnMiddleName",
                user->personName->pnMiddleName);
        INSERT_STRING(db_user_info, "pnLastName",
                user->personName->pnLastName);
        INSERT_STRING(db_user_info, "pnLastNameAtBirth",
                user->personName->pnLastNameAtBirth);
    }
    if (user->address) {
        INSERT_STRING(db_user_info, "adCity", user->address->adCity);
        INSERT_STRING(db_user_info, "adStreet", user->address->adStreet);
        INSERT_STRING(db_user_info, "adNumberInStreet",
                user->address->adNumberInStreet);
        INSERT_STRING(db_user_info, "adNumberInMunicipality",
                user->address->adNumberInMunicipality);
        INSERT_STRING(db_user_info, "adZipCode", user->address->adZipCode);
        INSERT_STRING(db_user_info, "adState", user->address->adState);
    }
    if (user->biDate) {
        if (!tm2datestring(user->biDate, &string))
            INSERT_STRING(db_user_info, "biDate", string);
        zfree(string);
    }
    CHECK_FOR_STRING_LENGTH(user->userID, 6, 12, "userID");
    INSERT_STRING(db_user_info, "userID", user->userID);

    /* userType */
    if (user->userType) {
        const xmlChar *type_string = isds_UserType2string(*(user->userType));
        if (!type_string) {
            isds_printf_message(context, _("Invalid userType value: %d"),
                    *(user->userType));
            err = IE_ENUM;
            goto leave;
        }
        INSERT_STRING(db_user_info, "userType", type_string);
    }

    INSERT_LONGINT(db_user_info, "userPrivils", user->userPrivils, string);
    CHECK_FOR_STRING_LENGTH(user->ic, 0, 8, "ic")
    INSERT_STRING(db_user_info, "ic", user->ic);
    CHECK_FOR_STRING_LENGTH(user->firmName, 0, 100, "firmName")
    INSERT_STRING(db_user_info, "firmName", user->firmName);
    INSERT_STRING(db_user_info, "caStreet", user->caStreet);
    INSERT_STRING(db_user_info, "caCity", user->caCity);
    INSERT_STRING(db_user_info, "caZipCode", user->caZipCode);

leave:
    free(string);
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
        zfree(string);
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
        zfree(string);
    }

leave:
    if (err) isds_envelope_free(envelope);
    free(unumber);
    free(string);
    xmlXPathFreeObject(result);
    return err;
}


/* Convert message type attribute of current element into isds_envelope
 * structure.
 * TODO: This function can be incorporated into append_status_size_times() as
 * they are called always together.
 * The envelope is automatically allocated but not reallocated.
 * The data are just appended into envelope structure.
 * @context is ISDS context
 * @envelope is automically allocated message envelope structure
 * @xpath_ctx is XPath context with current node as parent of attribute
 * carrying message type
 * In case of error @envelope will be freed. */
static isds_error append_message_type(struct isds_ctx *context,
        struct isds_envelope **envelope, xmlXPathContextPtr xpath_ctx) {
    isds_error err = IE_SUCCESS;

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
        zfree((*envelope)->dmType);
    }


    EXTRACT_STRING_ATTRIBUTE("dmType", (*envelope)->dmType, 0);

    if (!(*envelope)->dmType) {
        /* Use default value */
        (*envelope)->dmType = strdup("V");
        if (!(*envelope)->dmType) {
            err = IE_NOMEM;
            goto leave;
        }
    } else if (1 != xmlUTF8Strlen((xmlChar *) (*envelope)->dmType)) {
        char *type_locale = utf82locale((*envelope)->dmType);
        isds_printf_message(context,
                _("Message type in dmType attribute is not 1 character long: "
                    "%s"),
                type_locale);
        free(type_locale);
        err = IE_ISDS;
        goto leave;
    }

leave:
    if (err) isds_envelope_free(envelope);
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

    /* Get message type */
    err = append_message_type(context, envelope, xpath_ctx);
    if (err) goto leave;
    
    
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

    old_ctx_node = xpath_ctx->node;

    *hash = calloc(1, sizeof(**hash));
    if (!*hash) {
        err = IE_NOMEM;
        goto leave;
    }

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
        isds_printf_message(context,
                _("sisds:dmHash element is missing hash value"));
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

    /* Get message type */
    err = append_message_type(context, &((*message)->envelope), xpath_ctx);
    if (err) goto leave;
    
leave:
    if (err) isds_message_free(message);
    return err;
}


/* Extract message event into reallocated isds_event structure
 * @context is ISDS context
 * @event is automically reallocated message event structure
 * @xpath_ctx is XPath context with current node as isds:dmEvent
 * In case of error @event will be freed. */
static isds_error extract_event(struct isds_ctx *context,
        struct isds_event **event, xmlXPathContextPtr xpath_ctx) {
    isds_error err = IE_SUCCESS;
    xmlXPathObjectPtr result = NULL;
    xmlNodePtr event_node = xpath_ctx->node;
    char *string = NULL;

    if (!context) return IE_INVALID_CONTEXT;
    if (!event) return IE_INVAL;
    isds_event_free(event);
    if (!xpath_ctx) return IE_INVAL;

    *event = calloc(1, sizeof(**event));
    if (!*event) {
        err = IE_NOMEM;
        goto leave;
    }

    /* Extract event data.
     * All elements are optional according XSD. That's funny. */
    EXTRACT_STRING("sisds:dmEventTime", string);
    if (string) {
        err = timestring2timeval((xmlChar *) string, &((*event)->time));
        if (err) {
            char *string_locale = utf82locale(string);
            if (err == IE_DATE) err = IE_ISDS;
            isds_printf_message(context,
                    _("Could not convert dmEventTime as ISO time: %s"),
                    string_locale);
            free(string_locale);
            goto leave;
        }
        zfree(string);
    }
    
    /* dmEventDescr element has prefix and the rest */
    EXTRACT_STRING("sisds:dmEventDescr", string);
    if (string) {
        err = eventstring2event((xmlChar *) string, *event);
        if (err) goto leave;
        zfree(string);
    }

leave:
    if (err) isds_event_free(event);
    free(string);
    xmlXPathFreeObject(result);
    xpath_ctx->node = event_node;
    return err;
}


/* Convert element of XSD tEventsArray type from XML tree into
 * isds_list of isds_event's structure. The list is automatically reallocated.
 * @context is ISDS context
 * @events is automically reallocated list of event structures
 * @xpath_ctx is XPath context with current node as tEventsArray
 * In case of error @evnets will be freed. */
static isds_error extract_events(struct isds_ctx *context,
        struct isds_list **events, xmlXPathContextPtr xpath_ctx) {
    isds_error err = IE_SUCCESS;
    xmlXPathObjectPtr result = NULL;
    xmlNodePtr events_node = xpath_ctx->node;
    struct isds_list *event, *prev_event = NULL;

    if (!context) return IE_INVALID_CONTEXT;
    if (!events) return IE_INVAL;
    if (!xpath_ctx) return IE_INVAL;

    /* Free old list */
    isds_list_free(events);

    /* Find events */
    result = xmlXPathEvalExpression(BAD_CAST "sisds:dmEvent", xpath_ctx);
    if (!result) {
        err = IE_XML;
        goto leave;
    }

    /* No match */
    if (xmlXPathNodeSetIsEmpty(result->nodesetval)) {
        isds_printf_message(context,
                _("Delivery info does not contain any event"));
        err = IE_ISDS;
        goto leave;
    }


    /* Iterate over events */
    for (int i = 0; i < result->nodesetval->nodeNr; i++) {

        /* Allocate and append list item */
        event = calloc(1, sizeof(*event));
        if (!event) {
            err = IE_NOMEM;
            goto leave;
        }
        event->destructor = (void (*)(void **))isds_event_free;
        if (i == 0) *events = event;
        else prev_event->next = event;
        prev_event = event;

        /* Extract event */
        xpath_ctx->node = result->nodesetval->nodeTab[i];
        err = extract_event(context,
                (struct isds_event **) &(event->data), xpath_ctx);
        if (err) goto leave;
    }


leave:
    if (err) isds_list_free(events);
    xmlXPathFreeObject(result);
    xpath_ctx->node = events_node;
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
                _("Document has unknown dmFileMetaType: %ld"),
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
                ngettext("Not enought memory to encode %zd bytes into Base64",
                    "Not enought memory to encode %zd bytes into Base64",
                    document->data_length),
                document->data_length);
        err = IE_NOMEM;
        goto leave;
    }
    INSERT_STRING(file, "dmEncodedContent", base64data);
    free(base64data);

leave:
    return err;
}


/* Append XSD tMStatus XML tree into isds_message_copy structure.
 * The copy must pre prealocated, the date are just appended into structure.
 * @context is ISDS context
 * @copy is message copy struture
 * @xpath_ctx is XPath context with current node as tMStatus */
static isds_error append_TMStatus(struct isds_ctx *context,
        struct isds_message_copy *copy, xmlXPathContextPtr xpath_ctx) {
    isds_error err = IE_SUCCESS;
    xmlXPathObjectPtr result = NULL;
    char *code = NULL, *message = NULL;

    if (!context) return IE_INVALID_CONTEXT;
    if (!copy || !xpath_ctx) return IE_INVAL;

    /* Free old values */
    zfree(copy->dmStatus);
    zfree(copy->dmID);

    /* Get error specific to this copy */
    EXTRACT_STRING("isds:dmStatus/isds:dmStatusCode", code);
    if (!code) {
        isds_log_message(context,
                _("Missing isds:dmStatusCode under "
                    "XSD:tMStatus type element"));
        err = IE_ISDS;
        goto leave;
    }

    if (xmlStrcmp((const xmlChar *)code, BAD_CAST "0000")) {
        /* This copy failed */
        copy->error = IE_ISDS;
        EXTRACT_STRING("isds:dmStatus/isds:dmStatusMessage", message);
        if (message) {
            copy->dmStatus = astrcat3(code, ": ", message);
            if (!copy->dmStatus) {
                copy->dmStatus = code;
                code = NULL;
            }
        } else {
            copy->dmStatus = code;
            code = NULL;
        }
    } else {
        /* This copy succeeded. In this case only, message ID is valid */
        copy->error = IE_SUCCESS;

        EXTRACT_STRING("isds:dmID", copy->dmID);
        if (!copy->dmID) {
            isds_log(ILF_ISDS, ILL_ERR, _("Server accepted sent message, "
                        "but did not returned assigned message ID\n"));
            err = IE_ISDS;
        }
    }

leave:
    free(code);
    free(message);
    xmlXPathFreeObject(result);
    return err;
}


/* Insert struct isds_approval data (box approval) into XML tree
 * @context is sesstion context
 * @approval is libsids structure with approval description. NULL is
 * acceptible.
 * @parent is XML element to append @approval to */
static isds_error insert_GExtApproval(struct isds_ctx *context,
        const struct isds_approval *approval, xmlNodePtr parent) {

    isds_error err = IE_SUCCESS;
    xmlNodePtr node;

    if (!context) return IE_INVALID_CONTEXT;
    if (!parent) return IE_INVAL;

    if (!approval) return IE_SUCCESS;

    /* Build XSD:gExtApproval */
    INSERT_SCALAR_BOOLEAN(parent, "dbApproved", approval->approved);
    INSERT_STRING(parent, "dbExternRefNumber", approval->refference);

leave:
    return err;
}


/* Build ISDS request of XSD tDummyInput type, sent it and check for error
 * code
 * @context is session context
 * @service_name is name of SERVICE_DB_ACCESS
 * @response is server SOAP body response as XML document
 * @raw_response is automatically reallocated bitstream with response body. Use
 * NULL if you don't care
 * @raw_response_length is size of @raw_response in bytes
 * @code is ISDS status code
 * @status_message is ISDS status message
 * @return error coded from lower layer, context message will be set up
 * appropriately. */
static isds_error build_send_check_dbdummy_request(struct isds_ctx *context,
        const xmlChar *service_name,
        xmlDocPtr *response, void **raw_response, size_t *raw_response_length,
        xmlChar **code, xmlChar **status_message) {

    isds_error err = IE_SUCCESS;
    char *service_name_locale = NULL;
    xmlNodePtr request = NULL, node;
    xmlNsPtr isds_ns = NULL;

    if (!context) return IE_INVALID_CONTEXT;
    if (!service_name) return IE_INVAL;
    if (!response || !code || !status_message) return IE_INVAL;
    if (!raw_response_length && raw_response) return IE_INVAL;

    /* Free output argument */
    xmlFreeDoc(*response); *response = NULL;
    if (raw_response) zfree(*raw_response);
    free(*code);
    free(*status_message);


    /* Check if connection is established
     * TODO: This check should be done donwstairs. */
    if (!context->curl) return IE_CONNECTION_CLOSED;

    service_name_locale = utf82locale((char*)service_name);
    if (!service_name_locale) {
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


    /* Add XSD:tDummyInput child */
    INSERT_STRING(request, "dbDummy", NULL);


    isds_log(ILF_ISDS, ILL_DEBUG, _("Sending %s request to ISDS\n"),
            service_name_locale);

    /* Send request */
    err = isds(context, SERVICE_DB_ACCESS, request, response,
            raw_response, raw_response_length);
    xmlFreeNode(request); request = NULL;
    
    if (err) {
        isds_log(ILF_ISDS, ILL_DEBUG,
                    _("Processing ISDS response on %s request failed\n"),
                    service_name_locale);
        goto leave;
    }

    /* Check for response status */
    err = isds_response_status(context, SERVICE_DB_ACCESS, *response,
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
                    _("Server refused %s request (code=%s, message=%s)\n"),
                service_name_locale, code_locale, status_message_locale);
        isds_log_message(context, status_message_locale);
        free(code_locale);
        free(status_message_locale);
        err = IE_ISDS;
        goto leave;
    }

leave:
    free(service_name_locale);
    xmlFreeNode(request);
    return err;
}


/* Get data about logged in user and his box. */
isds_error isds_GetOwnerInfoFromLogin(struct isds_ctx *context,
        struct isds_DbOwnerInfo **db_owner_info) {
    isds_error err = IE_SUCCESS;
    xmlDocPtr response = NULL;
    xmlChar *code = NULL, *message = NULL;
    xmlXPathContextPtr xpath_ctx = NULL;
    xmlXPathObjectPtr result = NULL;
    char *string = NULL;

    if (!context) return IE_INVALID_CONTEXT;
    zfree(context->long_message);
    if (!db_owner_info) return IE_INVAL;

    /* Check if connection is established */
    if (!context->curl) return IE_CONNECTION_CLOSED;


    /* Do request and check for success */
    err = build_send_check_dbdummy_request(context,
            BAD_CAST "GetOwnerInfoFromLogin",
            &response, NULL, NULL, &code, &message);
    if (err) goto leave;


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


/* Get data about logged in user. */
isds_error isds_GetUserInfoFromLogin(struct isds_ctx *context,
        struct isds_DbUserInfo **db_user_info) {
    isds_error err = IE_SUCCESS;
    xmlDocPtr response = NULL;
    xmlChar *code = NULL, *message = NULL;
    xmlXPathContextPtr xpath_ctx = NULL;
    xmlXPathObjectPtr result = NULL;

    if (!context) return IE_INVALID_CONTEXT;
    zfree(context->long_message);
    if (!db_user_info) return IE_INVAL;

    /* Check if connection is established */
    if (!context->curl) return IE_CONNECTION_CLOSED;


    /* Do request and check for success */
    err = build_send_check_dbdummy_request(context,
            BAD_CAST "GetUserInfoFromLogin",
            &response, NULL, NULL, &code, &message);
    if (err) goto leave;


    /* Extract data */
    /* Prepare stucture */
    isds_DbUserInfo_free(db_user_info);
    *db_user_info = calloc(1, sizeof(**db_user_info));
    if (!*db_user_info) {
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
            "/isds:GetUserInfoFromLoginResponse/isds:dbUserInfo", xpath_ctx);
    if (!result) {
        err = IE_ERROR;
        goto leave;
    }
    if (xmlXPathNodeSetIsEmpty(result->nodesetval)) {
        isds_log_message(context, _("Missing dbUserInfo element"));
        err = IE_ISDS;
        goto leave;
    }
    if (result->nodesetval->nodeNr > 1) {
        isds_log_message(context, _("Multiple dbUserInfo element"));
        err = IE_ISDS;
        goto leave;
    }
    xpath_ctx->node = result->nodesetval->nodeTab[0];
    xmlXPathFreeObject(result); result = NULL;

    /* Extract it */
    err = extract_DbUserInfo(context, db_user_info, xpath_ctx);

leave:
    if (err) {
        isds_DbUserInfo_free(db_user_info);
    }

    xmlXPathFreeObject(result);
    xmlXPathFreeContext(xpath_ctx);

    free(code);
    free(message);
    xmlFreeDoc(response);

    if (!err)
        isds_log(ILF_ISDS, ILL_DEBUG,
                _("GetUserInfoFromLogin request processed by server "
                    "successfully.\n"));

    return err;
}


/* Get expiration time of current password
 * @context is session context
 * @expiration is automatically reallocated time when password expires, In
 * case of error will be nulled. */
isds_error isds_get_password_expiration(struct isds_ctx *context,
        struct timeval **expiration) {
    isds_error err = IE_SUCCESS;
    xmlDocPtr response = NULL;
    xmlChar *code = NULL, *message = NULL;
    xmlXPathContextPtr xpath_ctx = NULL;
    xmlXPathObjectPtr result = NULL;
    char *string = NULL;

    if (!context) return IE_INVALID_CONTEXT;
    zfree(context->long_message);
    if (!expiration) return IE_INVAL;

    /* Check if connection is established */
    if (!context->curl) return IE_CONNECTION_CLOSED;


    /* Do request and check for success */
    err = build_send_check_dbdummy_request(context,
            BAD_CAST "GetPasswordInfo",
            &response, NULL, NULL, &code, &message);
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

    /* Set context node */
    result = xmlXPathEvalExpression(BAD_CAST
            "/isds:GetPasswordInfoResponse", xpath_ctx);
    if (!result) {
        err = IE_ERROR;
        goto leave;
    }
    if (xmlXPathNodeSetIsEmpty(result->nodesetval)) {
        isds_log_message(context,
                _("Missing GetPasswordInfoResponse element"));
        err = IE_ISDS;
        goto leave;
    }
    if (result->nodesetval->nodeNr > 1) {
        isds_log_message(context,
                _("Multiple GetPasswordInfoResponse element"));
        err = IE_ISDS;
        goto leave;
    }
    xpath_ctx->node = result->nodesetval->nodeTab[0];
    xmlXPathFreeObject(result); result = NULL;

    /* Extract expiration date */
    EXTRACT_STRING("isds:pswExpDate", string);
    if (!string) {
        isds_log_message(context, _("Missing pswExpDate element"));
        err = IE_ISDS;
        goto leave;
    }

    err = timestring2timeval((xmlChar *) string, expiration);
    if (err) {
        char *string_locale = utf82locale(string);
        if (err == IE_DATE) err = IE_ISDS;
        isds_printf_message(context,
                _("Could not convert pswExpDate as ISO time: %s"),
                string_locale);
        free(string_locale);
        goto leave;
    }

leave:
    if (err) {
        if (*expiration) {
            zfree(*expiration);
        }
    }

    free(string);
    xmlXPathFreeObject(result);
    xmlXPathFreeContext(xpath_ctx);

    free(code);
    free(message);
    xmlFreeDoc(response);

    if (!err)
        isds_log(ILF_ISDS, ILL_DEBUG,
                _("GetPasswordInfo request processed by server "
                    "successfully.\n"));

    return err;
}


/* Change user password in ISDS.
 * User must supply old password, new password will takes effect after some
 * time, current session can continue. Password must fulfill some constraints.
 * @context is session context
 * @old_password is current password.
 * @new_password is requested new password */
isds_error isds_change_password(struct isds_ctx *context,
        const char *old_password, const char *new_password) {
    isds_error err = IE_SUCCESS;
    xmlNsPtr isds_ns = NULL;
    xmlNodePtr request = NULL, node;
    xmlDocPtr response = NULL;
    xmlChar *code = NULL, *message = NULL;

    if (!context) return IE_INVALID_CONTEXT;
    zfree(context->long_message);
    if (!old_password || !new_password) return IE_INVAL;

    /* Check if connection is established
     * TODO: This check should be done donwstairs. */
    if (!context->curl) return IE_CONNECTION_CLOSED;


    /* Build ChangeISDSPassword request */
    request = xmlNewNode(NULL, BAD_CAST "ChangeISDSPassword");
    if (!request) {
        isds_log_message(context,
                _("Could not build ChangeISDSPassword request"));
        return IE_ERROR;
    }
    isds_ns = xmlNewNs(request, BAD_CAST ISDS_NS, NULL);
    if(!isds_ns) {
        isds_log_message(context, _("Could not create ISDS name space"));
        xmlFreeNode(request);
        return IE_ERROR;
    }
    xmlSetNs(request, isds_ns);

    INSERT_STRING(request, "dbOldPassword", old_password);
    INSERT_STRING(request, "dbNewPassword", new_password);


    isds_log(ILF_ISDS, ILL_DEBUG, _("Sending CheckDataBox request to ISDS\n"));

    /* Sent request */
    err = isds(context, SERVICE_DB_ACCESS, request, &response, NULL, NULL);
   
    /* Destroy request */
    xmlFreeNode(request); request = NULL;

    if (err) {
        isds_log(ILF_ISDS, ILL_DEBUG,
                _("Processing ISDS response on ChangeISDSPassword "
                    "request failed\n"));
        goto leave;
    }

    /* Check for response status */
    err = isds_response_status(context, SERVICE_DB_ACCESS, response,
            &code, &message, NULL);
    if (err) {
        isds_log(ILF_ISDS, ILL_DEBUG,
                _("ISDS response on ChangeISDSPassword request is missing "
                    "status\n"));
        goto leave;
    }

    /* Request processed, but empty password refused */
    if (!xmlStrcmp(code, BAD_CAST "1066")) {
        char *code_locale = utf82locale((char*)code);
        char *message_locale = utf82locale((char*)message);
        isds_log(ILF_ISDS, ILL_DEBUG,
                _("Server refused empty password on ChangeISDSPassword "
                    "request (code=%s, message=%s)\n"),
                code_locale, message_locale);
        isds_log_message(context, _("Password must not be empty"));
        free(code_locale);
        free(message_locale);
        err = IE_INVAL;
        goto leave;
    }

    /* Request processed, but new password was reused */
    else if (!xmlStrcmp(code, BAD_CAST "1067")) {
        char *code_locale = utf82locale((char*)code);
        char *message_locale = utf82locale((char*)message);
        isds_log(ILF_ISDS, ILL_DEBUG,
                _("Server refused the same new password on ChangeISDSPassword "
                    "request (code=%s, message=%s)\n"),
                code_locale, message_locale);
        isds_log_message(context,
                _("New password must differ from the current one"));
        free(code_locale);
        free(message_locale);
        err = IE_INVAL;
        goto leave;
    }

    /* Other error */
    else if (xmlStrcmp(code, BAD_CAST "0000")) {
        char *code_locale = utf82locale((char*)code);
        char *message_locale = utf82locale((char*)message);
        isds_log(ILF_ISDS, ILL_DEBUG,
                _("Server refused to change password on ChangeISDSPassword "
                    "request (code=%s, message=%s)\n"),
                code_locale, message_locale);
        isds_log_message(context, message_locale);
        free(code_locale);
        free(message_locale);
        err = IE_ISDS;
        goto leave;
    }
    
    /* Otherwise password changed successfully */

leave:
    free(code);
    free(message);
    xmlFreeDoc(response);
    xmlFreeNode(request);

    if (!err)
        isds_log(ILF_ISDS, ILL_DEBUG,
                _("Password changed successfully on ChangeISDSPassword "
                    "request.\n"));

    return err;
}


/* Generic middle part with request sending and response check.
 * It sends prepared request and checks for error code.
 * @context is ISDS session context.
 * @service is ISDS service handler
 * @service_name is name in scope of given @service
 * @request is XML tree with request. Will be freed to save memory.
 * @response is XML document ouputing ISDS response.
 * @refnumber is reallocated serial number of request assigned by ISDS. Use
 * NULL, if you don't care. */
static isds_error send_destroy_request_check_response(
        struct isds_ctx *context,
        const isds_service service, const xmlChar *service_name, 
        xmlNodePtr *request, xmlDocPtr *response, xmlChar **refnumber) {
    isds_error err = IE_SUCCESS;
    char *service_name_locale = NULL;
    xmlChar *code = NULL, *message = NULL;


    if (!context) return IE_INVALID_CONTEXT;
    if (!service_name || *service_name == '\0' || !request || !*request ||
            !response)
        return IE_INVAL;

    /* Check if connection is established
     * TODO: This check should be done donwstairs. */
    if (!context->curl) return IE_CONNECTION_CLOSED;

    service_name_locale = utf82locale((char*) service_name);
    if (!service_name_locale) {
        err = IE_NOMEM;
        goto leave;
    }

    isds_log(ILF_ISDS, ILL_DEBUG, _("Sending %s request to ISDS\n"),
            service_name_locale);

    /* Send request */
    err = isds(context, service, *request, response, NULL, NULL);
    xmlFreeNode(*request); *request = NULL;
    
    if (err) {
        isds_log(ILF_ISDS, ILL_DEBUG,
                    _("Processing ISDS response on %s request failed\n"),
                    service_name_locale);
        goto leave;
    }

    /* Check for response status */
    err = isds_response_status(context, service, *response,
            &code, &message, refnumber);
    if (err) {
        isds_log(ILF_ISDS, ILL_DEBUG,
                    _("ISDS response on %s request is missing status\n"),
                    service_name_locale);
        goto leave;
    }

    /* Request processed, but server failed */
    if (xmlStrcmp(code, BAD_CAST "0000")) {
        char *code_locale = utf82locale((char*) code);
        char *message_locale = utf82locale((char*) message);
        isds_log(ILF_ISDS, ILL_DEBUG,
                    _("Server refused %s request (code=%s, message=%s)\n"),
                service_name_locale, code_locale, message_locale);
        isds_log_message(context, message_locale);
        free(code_locale);
        free(message_locale);
        err = IE_ISDS;
        goto leave;
    }


leave:
    free(code);
    free(message);
    if (err && *response) {
        xmlFreeDoc(*response);
        *response = NULL;
    }
    if (*request) {
        xmlFreeNode(*request);
        *request = NULL;
    }
    free(service_name_locale);

    return err;
}


/* Generic bottom half with request sending.
 * It sends prepared request, checks for error code, destroys response and
 * request and log success or failure.
 * @context is ISDS session context.
 * @service is ISDS service handler
 * @service_name is name in scope of given @service
 * @request is XML tree with request. Will be freed to save memory.
 * @refnumber is reallocated serial number of request assigned by ISDS. Use
 * NULL, if you don't care. */
static isds_error send_request_check_drop_response(
        struct isds_ctx *context,
        const isds_service service, const xmlChar *service_name, 
        xmlNodePtr *request, xmlChar **refnumber) {
    isds_error err = IE_SUCCESS;
    xmlDocPtr response = NULL;


    if (!context) return IE_INVALID_CONTEXT;
    if (!service_name || *service_name == '\0' || !request || !*request)
        return IE_INVAL;

    /* Send request and check response*/
    err = send_destroy_request_check_response(context,
            service, service_name, request, &response, refnumber);

    xmlFreeDoc(response);

    if (*request) {
        xmlFreeNode(*request);
        *request = NULL;
    }

    if (!err) {
        char *service_name_locale = utf82locale((char *) service_name);
        isds_log(ILF_ISDS, ILL_DEBUG,
                _("%s request processed by server successfully.\n"),
                service_name_locale);
        free(service_name_locale);
    }

    return err;
}


/* Build XSD:tCreateDBInput request type for box createing.
 * @context is session context
 * @request outputs built XML tree
 * @service_name is request name of SERVICE_DB_MANIPULATION service
 * @box is box description to create including single primary user (in case of
 * FO box type)
 * @users is list of struct isds_DbUserInfo (primary users in case of non-FO
 * box, or contact address of PFO box owner)
 * @former_names is optional undocumented string. Pass NULL if you don't care.
 * @upper_box_id is optional ID of supper box if currently created box is
 * subordinated.
 * @ceo_label is optional title of OVM box owner (e.g. mayor) NULL, if you
 * don't care.
 * @approval is optional external approval of box manipulation */
static isds_error build_CreateDBInput_request(struct isds_ctx *context,
        xmlNodePtr *request, const xmlChar *service_name,
        const struct isds_DbOwnerInfo *box, const struct isds_list *users,
        const xmlChar *former_names, const xmlChar *upper_box_id,
        const xmlChar *ceo_label, const struct isds_approval *approval) {
    isds_error err = IE_SUCCESS;
    xmlNsPtr isds_ns = NULL;
    xmlNodePtr node, dbPrimaryUsers;
    xmlChar *string = NULL;
    const struct isds_list *item;


    if (!context) return IE_INVALID_CONTEXT;
    if (!request || !service_name || service_name[0] == '\0' || !box)
        return IE_INVAL;


    /* Build DeleteDataBox request */
    *request = xmlNewNode(NULL, service_name);
    if (!*request) {
        char *service_name_locale = utf82locale((char*) service_name);
        isds_printf_message(context, _("Could build %s request"),
                service_name_locale);
        free(service_name_locale);
        return IE_ERROR;
    }
    if (context->type == CTX_TYPE_TESTING_REQUEST_COLLECTOR) {
        isds_ns = xmlNewNs(*request, BAD_CAST ISDS1_NS, NULL);
        if (!isds_ns) {
            isds_log_message(context, _("Could not create ISDS1 name space"));
            xmlFreeNode(*request);
            return IE_ERROR;
        }
    } else {
        isds_ns = xmlNewNs(*request, BAD_CAST ISDS_NS, NULL);
        if (!isds_ns) {
            isds_log_message(context, _("Could not create ISDS name space"));
            xmlFreeNode(*request);
            return IE_ERROR;
        }
    }
    xmlSetNs(*request, isds_ns);

    INSERT_ELEMENT(node, *request, "dbOwnerInfo");
    err = insert_DbOwnerInfo(context, box, node);
    if (err) goto leave;

    /* Insert users */
    /* XXX: There is bug in XSD: XSD says at least one dbUserInfo must exist,
     * verbose documentatiot allows none dbUserInfo */
    INSERT_ELEMENT(dbPrimaryUsers, *request, "dbPrimaryUsers");
    for (item = users; item; item = item->next) {
        if (item->data) {
            INSERT_ELEMENT(node, dbPrimaryUsers, "dbUserInfo");
            err = insert_DbUserInfo(context,
                    (struct isds_DbUserInfo *) item->data, node);
            if (err) goto leave;
        }
    }

    INSERT_STRING(*request, "dbFormerNames", former_names);
    INSERT_STRING(*request, "dbUpperDBId", upper_box_id);
    INSERT_STRING(*request, "dbCEOLabel", ceo_label);

    err = insert_GExtApproval(context, approval, *request);
    if (err) goto leave;

leave:
    if (err) {
        xmlFreeNode(*request);
        *request = NULL;
    }
    free(string);
    return err;
}


/* Create new box.
 * @context is session context
 * @box is box description to create including single primary user (in case of
 * FO box type). It outputs box ID assigned by ISDS in dbID element.
 * @users is list of struct isds_DbUserInfo (primary users in case of non-FO
 * box, or contact address of PFO box owner)
 * @former_names is optional undocumented string. Pass NULL if you don't care.
 * @upper_box_id is optional ID of supper box if currently created box is
 * subordinated.
 * @ceo_label is optional title of OVM box owner (e.g. mayor)
 * @approval is optional external approval of box manipulation
 * @refnumber is reallocated serial number of request assigned by ISDS. Use
 * NULL, if you don't care.*/
isds_error isds_add_box(struct isds_ctx *context,
        struct isds_DbOwnerInfo *box, const struct isds_list *users,
        const char *former_names, const char *upper_box_id,
        const char *ceo_label, const struct isds_approval *approval,
        char **refnumber) {
    isds_error err = IE_SUCCESS;
    xmlNodePtr request = NULL;
    xmlDocPtr response = NULL;
    xmlXPathContextPtr xpath_ctx = NULL;
    xmlXPathObjectPtr result = NULL;


    if (!context) return IE_INVALID_CONTEXT;
    zfree(context->long_message);
    if (!box) return IE_INVAL;

    /* Scratch box ID */
    zfree(box->dbID);

    /* Build CreateDataBox request */
    err = build_CreateDBInput_request(context,
            &request, BAD_CAST "CreateDataBox",
            box, users, (xmlChar *) former_names, (xmlChar *) upper_box_id,
            (xmlChar *) ceo_label, approval);
    if (err) goto leave;

    /* Send it to server and process response */
    err = send_destroy_request_check_response(context,
            SERVICE_DB_MANIPULATION, BAD_CAST "CreateDataBox", &request,
            &response, (xmlChar **) refnumber);

    /* Extract box ID */
    xpath_ctx = xmlXPathNewContext(response);
    if (!xpath_ctx) {
        err = IE_ERROR;
        goto leave;
    }
    if (register_namespaces(xpath_ctx, MESSAGE_NS_UNSIGNED)) {
        err = IE_ERROR;
        goto leave;
    }
    EXTRACT_STRING("/isds:CreateDataBoxResponse/dbID", box->dbID);

leave:
    xmlXPathFreeObject(result);
    xmlXPathFreeContext(xpath_ctx);
    xmlFreeDoc(response);
    xmlFreeNode(request);

    if (!err) {
        isds_log(ILF_ISDS, ILL_DEBUG,
                _("CreateDataBox request processed by server successfully.\n"));
    }

    return err;
}


/* Notify ISDS about new PFO entity.
 * This function has no real effect.
 * @context is session context
 * @box is PFO description including single primary user.
 * @users is list of struct isds_DbUserInfo (contact address of PFO box owner)
 * @former_names is optional undocumented string. Pass NULL if you don't care.
 * @upper_box_id is optional ID of supper box if currently created box is
 * subordinated.
 * @ceo_label is optional title of OVM box owner (e.g. mayor)
 * @approval is optional external approval of box manipulation
 * @refnumber is reallocated serial number of request assigned by ISDS. Use
 * NULL, if you don't care.*/
isds_error isds_add_pfoinfo(struct isds_ctx *context,
        const struct isds_DbOwnerInfo *box, const struct isds_list *users,
        const char *former_names, const char *upper_box_id,
        const char *ceo_label, const struct isds_approval *approval,
        char **refnumber) {
    isds_error err = IE_SUCCESS;
    xmlNodePtr request = NULL;

    if (!context) return IE_INVALID_CONTEXT;
    zfree(context->long_message);
    if (!box) return IE_INVAL;

    /* Build CreateDataBoxPFOInfo request */
    err = build_CreateDBInput_request(context,
            &request, BAD_CAST "CreateDataBoxPFOInfo",
            box, users, (xmlChar *) former_names, (xmlChar *) upper_box_id,
            (xmlChar *) ceo_label, approval);
    if (err) goto leave;

    /* Send it to server and process response */
    err = send_request_check_drop_response(context,
            SERVICE_DB_MANIPULATION, BAD_CAST "CreateDataBox", &request,
            (xmlChar **) refnumber);
leave:
    xmlFreeNode(request);
    return err;
}


/* Remove given given box permanetly.
 * @context is session context
 * @box is box description to delete
 * @since is date of box owner cancalation. Only tm_year, tm_mon and tm_mday
 * carry sane value.
 * @approval is optional external approval of box manipulation
 * @refnumber is reallocated serial number of request assigned by ISDS. Use
 * NULL, if you don't care.*/
isds_error isds_delete_box(struct isds_ctx *context,
        const struct isds_DbOwnerInfo *box, const struct tm *since,
        const struct isds_approval *approval, char **refnumber) {
    isds_error err = IE_SUCCESS;
    xmlNsPtr isds_ns = NULL;
    xmlNodePtr request = NULL;
    xmlNodePtr node;
    xmlChar *string = NULL;


    if (!context) return IE_INVALID_CONTEXT;
    zfree(context->long_message);
    if (!box || !since) return IE_INVAL;


    /* Build DeleteDataBox request */
    request = xmlNewNode(NULL, BAD_CAST "DeleteDataBox");
    if (!request) {
        isds_log_message(context,
                _("Could build DeleteDataBox request"));
        return IE_ERROR;
    }
    isds_ns = xmlNewNs(request, BAD_CAST ISDS_NS, NULL);
    if(!isds_ns) {
        isds_log_message(context, _("Could not create ISDS name space"));
        xmlFreeNode(request);
        return IE_ERROR;
    }
    xmlSetNs(request, isds_ns);

    INSERT_ELEMENT(node, request, "dbOwnerInfo");
    err = insert_DbOwnerInfo(context, box, node);
    if (err) goto leave;

    err = tm2datestring(since, &string);
    if (err) {
        isds_log_message(context,
                _("Could not convert `since' argument to ISO date string"));
        goto leave;
    }
    INSERT_STRING(request, "dbOwnerTerminationDate", string);
    zfree(string);

    err = insert_GExtApproval(context, approval, request);
    if (err) goto leave;


    /* Send it to server and process response */
    err = send_request_check_drop_response(context, SERVICE_DB_MANIPULATION,
            BAD_CAST "DeleteDataBox", &request, (xmlChar **) refnumber);

leave:
    xmlFreeNode(request);
    free(string);
    return err;
}


/* Update data about given box.
 * @context is session context
 * @old_box current box description
 * @new_box are updated data about @old_box
 * @approval is optional external approval of box manipulation
 * @refnumber is reallocated serial number of request assigned by ISDS. Use
 * NULL, if you don't care.*/
isds_error isds_UpdateDataBoxDescr(struct isds_ctx *context,
        const struct isds_DbOwnerInfo *old_box,
        const struct isds_DbOwnerInfo *new_box,
        const struct isds_approval *approval, char **refnumber) {
    isds_error err = IE_SUCCESS;
    xmlNsPtr isds_ns = NULL;
    xmlNodePtr request = NULL;
    xmlNodePtr node;


    if (!context) return IE_INVALID_CONTEXT;
    zfree(context->long_message);
    if (!old_box || !new_box) return IE_INVAL;


    /* Build UpdateDataBoxDescr request */
    request = xmlNewNode(NULL, BAD_CAST "UpdateDataBoxDescr");
    if (!request) {
        isds_log_message(context,
                _("Could build UpdateDataBoxDescr request"));
        return IE_ERROR;
    }
    isds_ns = xmlNewNs(request, BAD_CAST ISDS_NS, NULL);
    if(!isds_ns) {
        isds_log_message(context, _("Could not create ISDS name space"));
        xmlFreeNode(request);
        return IE_ERROR;
    }
    xmlSetNs(request, isds_ns);

    INSERT_ELEMENT(node, request, "dbOldOwnerInfo");
    err = insert_DbOwnerInfo(context, old_box, node);
    if (err) goto leave;

    INSERT_ELEMENT(node, request, "dbNewOwnerInfo");
    err = insert_DbOwnerInfo(context, new_box, node);
    if (err) goto leave;

    err = insert_GExtApproval(context, approval, request);
    if (err) goto leave;


    /* Send it to server and process response */
    err = send_request_check_drop_response(context, SERVICE_DB_MANIPULATION,
            BAD_CAST "UpdateDataBoxDescr", &request, (xmlChar **) refnumber);

leave:
    xmlFreeNode(request);

    return err;
}


/* Build ISDS request of XSD tIdDbInput type, sent it and check for error
 * code
 * @context is session context
 * @service is SOAP service
 * @service_name is name of request in @service
 * @box_id is box ID of interrest
 * @approval is optional external approval of box manipulation
 * @response is server SOAP body response as XML document
 * @refnumber is reallocated serial number of request assigned by ISDS. Use
 * NULL, if you don't care.
 * @return error coded from lower layer, context message will be set up
 * appropriately. */
static isds_error build_send_dbid_request_check_response(
        struct isds_ctx *context, const isds_service service,
        const xmlChar *service_name, const xmlChar *box_id,
        const struct isds_approval *approval,
        xmlDocPtr *response, xmlChar **refnumber) {

    isds_error err = IE_SUCCESS;
    char *service_name_locale = NULL, *box_id_locale = NULL;
    xmlNodePtr request = NULL, node;
    xmlNsPtr isds_ns = NULL;

    if (!context) return IE_INVALID_CONTEXT;
    if (!service_name || !box_id) return IE_INVAL;
    if (!response) return IE_INVAL;

    /* Free output argument */
    xmlFreeDoc(*response); *response = NULL;
   
    /* Prepare strings */
    service_name_locale = utf82locale((char*)service_name);
    if (!service_name_locale) {
        err = IE_NOMEM;
        goto leave;
    }
    box_id_locale = utf82locale((char*)box_id);
    if (!box_id_locale) {
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

    /* Add XSD:tIdDbInput children */
    INSERT_STRING(request, "dbID", box_id);
    err = insert_GExtApproval(context, approval, request);
    if (err) goto leave;

    /* Send request and check response*/
    err = send_destroy_request_check_response(context,
            service, service_name, &request, response, refnumber);

leave:
    free(service_name_locale);
    free(box_id_locale);
    xmlFreeNode(request);
    return err;
}


/* Get data about all users assigned to given box.
 * @context is session context
 * @box_id is box ID
 * @users is automatically reallocated list of struct isds_DbUserInfo */
isds_error isds_GetDataBoxUsers(struct isds_ctx *context, const char *box_id,
        struct isds_list **users) {
    isds_error err = IE_SUCCESS;
    xmlDocPtr response = NULL;
    xmlXPathContextPtr xpath_ctx = NULL;
    xmlXPathObjectPtr result = NULL;
    int i;
    struct isds_list *item, *prev_item = NULL;

    if (!context) return IE_INVALID_CONTEXT;
    zfree(context->long_message);
    if (!users || !box_id) return IE_INVAL;


    /* Do request and check for success */
    err = build_send_dbid_request_check_response(context,
            SERVICE_DB_MANIPULATION, BAD_CAST "GetDataBoxUsers",
            BAD_CAST box_id, NULL, &response, NULL);
    if (err) goto leave;


    /* Extract data */
    /* Prepare stucture */
    isds_list_free(users);
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
            "/isds:GetDataBoxUsersResponse/isds:dbUsers/isds:dbUserInfo",
            xpath_ctx);
    if (!result) {
        err = IE_ERROR;
        goto leave;
    }
    if (xmlXPathNodeSetIsEmpty(result->nodesetval)) {
        isds_log_message(context, _("Missing dbUserInfo element"));
        err = IE_ISDS;
        goto leave;
    }

    /* Iterate over all users */
    for (i = 0; i < result->nodesetval->nodeNr; i++) {

        /* Prepare structure */
        item = calloc(1, sizeof(*item));
        if (!item) {
            err = IE_NOMEM;
            goto leave;
        }
        item->destructor = (void(*)(void**))isds_DbUserInfo_free;
        if (i == 0) *users = item;
        else prev_item->next = item;
        prev_item = item;

        /* Extract it */
        xpath_ctx->node = result->nodesetval->nodeTab[i];
        err = extract_DbUserInfo(context,
                (struct isds_DbUserInfo **) (&item->data), xpath_ctx);
        if (err) goto leave;
    }

leave:
    if (err) {
        isds_list_free(users);
    }

    xmlXPathFreeObject(result);
    xmlXPathFreeContext(xpath_ctx);
    xmlFreeDoc(response);

    if (!err)
        isds_log(ILF_ISDS, ILL_DEBUG,
                _("GetDataBoxUsers request processed by server "
                    "successfully.\n"));

    return err;
}


/* Update data about user assigned to given box.
 * @context is session context
 * @box is box identification
 * @old_user identifies user to update
 * @new_user are updated data about @old_user
 * @refnumber is reallocated serial number of request assigned by ISDS. Use
 * NULL, if you don't care.*/
isds_error isds_UpdateDataBoxUser(struct isds_ctx *context,
        const struct isds_DbOwnerInfo *box,
        const struct isds_DbUserInfo *old_user,
        const struct isds_DbUserInfo *new_user,
        char **refnumber) {
    isds_error err = IE_SUCCESS;
    xmlNsPtr isds_ns = NULL;
    xmlNodePtr request = NULL;
    xmlNodePtr node;


    if (!context) return IE_INVALID_CONTEXT;
    zfree(context->long_message);
    if (!box || !old_user || !new_user) return IE_INVAL;


    /* Build UpdateDataBoxUser request */
    request = xmlNewNode(NULL, BAD_CAST "UpdateDataBoxUser");
    if (!request) {
        isds_log_message(context,
                _("Could build UpdateDataBoxUser request"));
        return IE_ERROR;
    }
    isds_ns = xmlNewNs(request, BAD_CAST ISDS_NS, NULL);
    if(!isds_ns) {
        isds_log_message(context, _("Could not create ISDS name space"));
        xmlFreeNode(request);
        return IE_ERROR;
    }
    xmlSetNs(request, isds_ns);

    INSERT_ELEMENT(node, request, "dbOwnerInfo");
    err = insert_DbOwnerInfo(context, box, node);
    if (err) goto leave;

    INSERT_ELEMENT(node, request, "dbOldUserInfo");
    err = insert_DbUserInfo(context, old_user, node);
    if (err) goto leave;

    INSERT_ELEMENT(node, request, "dbNewUserInfo");
    err = insert_DbUserInfo(context, new_user, node);
    if (err) goto leave;

    /* Send it to server and process response */
    err = send_request_check_drop_response(context, SERVICE_DB_MANIPULATION,
            BAD_CAST "UpdateDataBoxUser", &request, (xmlChar **) refnumber);

leave:
    xmlFreeNode(request);

    return err;
}


/* Reset credentials of user assigned to given box.
 * @context is session context
 * @box is box identification
 * @user identifies user to reset password
 * @fee_paid is true if fee has been paid, false otherwise
 * @approval is optional external approval of box manipulation
 * @token is NULL if new password should be delivered off-line to the user.
 * It is valid pointer if user should obtain new password on-line on dedicated
 * web server. Then it output automatically reallocated token user needs to
 * use to athtorize on the web server to view his new password. 
 * @refnumber is reallocated serial number of request assigned by ISDS. Use
 * NULL, if you don't care.*/
isds_error isds_reset_password(struct isds_ctx *context,
        const struct isds_DbOwnerInfo *box,
        const struct isds_DbUserInfo *user,
        const _Bool fee_paid, const struct isds_approval *approval,
        char **token, char **refnumber) {
    isds_error err = IE_SUCCESS;
    xmlNsPtr isds_ns = NULL;
    xmlNodePtr request = NULL, node;
    xmlDocPtr response = NULL;
    xmlXPathContextPtr xpath_ctx = NULL;
    xmlXPathObjectPtr result = NULL;


    if (!context) return IE_INVALID_CONTEXT;
    zfree(context->long_message);
    if (!box || !user) return IE_INVAL;

    if (token) zfree(*token);


    /* Build NewAccessData request */
    request = xmlNewNode(NULL, BAD_CAST "NewAccessData");
    if (!request) {
        isds_log_message(context,
                _("Could build NewAccessData request"));
        return IE_ERROR;
    }
    isds_ns = xmlNewNs(request, BAD_CAST ISDS_NS, NULL);
    if(!isds_ns) {
        isds_log_message(context, _("Could not create ISDS name space"));
        xmlFreeNode(request);
        return IE_ERROR;
    }
    xmlSetNs(request, isds_ns);

    INSERT_ELEMENT(node, request, "dbOwnerInfo");
    err = insert_DbOwnerInfo(context, box, node);
    if (err) goto leave;

    INSERT_ELEMENT(node, request, "dbUserInfo");
    err = insert_DbUserInfo(context, user, node);
    if (err) goto leave;

    INSERT_SCALAR_BOOLEAN(request, "dbFeePaid", fee_paid);

    if (token) {
        INSERT_SCALAR_BOOLEAN(request, "dbVirtual", 1);
    } else {
        INSERT_SCALAR_BOOLEAN(request, "dbVirtual", 0);
    }

    err = insert_GExtApproval(context, approval, request);
    if (err) goto leave;

    /* Send request and check reposne*/
    err = send_destroy_request_check_response(context,
            SERVICE_DB_MANIPULATION, BAD_CAST "NewAccessData", &request,
            &response, (xmlChar **) refnumber);
    if (err) goto leave;


    /* Extract optional token */
    if (token) {
        xpath_ctx = xmlXPathNewContext(response);
        if (!xpath_ctx) {
            err = IE_ERROR;
            goto leave;
        }
        if (register_namespaces(xpath_ctx, MESSAGE_NS_UNSIGNED)) {
            err = IE_ERROR;
            goto leave;
        }

        EXTRACT_STRING("/isds:NewAccessDataResponse/dbAccessDataId", *token);
    }

leave:
    xmlXPathFreeObject(result);
    xmlXPathFreeContext(xpath_ctx);
    xmlFreeDoc(response);
    xmlFreeNode(request);

    if (!err)
        isds_log(ILF_ISDS, ILL_DEBUG,
                _("NewAccessData request processed by server "
                    "successfully.\n"));

    return err;
}


/* Build ISDS request of XSD tAddDBUserInput type, sent it, check for error
 * code, destroy response and log success.
 * @context is ISDS session context.
 * @service_name is name of SERVICE_DB_MANIPULATION service
 * @box is box identification
 * @user identifies user to removve
 * @approval is optional external approval of box manipulation
 * @refnumber is reallocated serial number of request assigned by ISDS. Use
 * NULL, if you don't care. */
static isds_error build_send_manipulationboxuser_request_check_drop_response(
        struct isds_ctx *context, const xmlChar *service_name,
        const struct isds_DbOwnerInfo *box, const struct isds_DbUserInfo *user,
        const struct isds_approval *approval, xmlChar **refnumber) {
    isds_error err = IE_SUCCESS;
    xmlNsPtr isds_ns = NULL;
    xmlNodePtr request = NULL, node;


    if (!context) return IE_INVALID_CONTEXT;
    zfree(context->long_message);
    if (!service_name || service_name[0] == '\0' || !box || !user)
        return IE_INVAL;


    /* Build NewAccessData request */
    request = xmlNewNode(NULL, service_name);
    if (!request) {
        char *service_name_locale = utf82locale((char *) service_name);
        isds_printf_message(context, _("Could build %s request"),
                service_name_locale);
        free(service_name_locale);
        return IE_ERROR;
    }
    isds_ns = xmlNewNs(request, BAD_CAST ISDS_NS, NULL);
    if(!isds_ns) {
        isds_log_message(context, _("Could not create ISDS name space"));
        xmlFreeNode(request);
        return IE_ERROR;
    }
    xmlSetNs(request, isds_ns);

    INSERT_ELEMENT(node, request, "dbOwnerInfo");
    err = insert_DbOwnerInfo(context, box, node);
    if (err) goto leave;

    INSERT_ELEMENT(node, request, "dbUserInfo");
    err = insert_DbUserInfo(context, user, node);
    if (err) goto leave;

    err = insert_GExtApproval(context, approval, request);
    if (err) goto leave;

    /* Send request and check reposne*/
    err = send_request_check_drop_response (context,
            SERVICE_DB_MANIPULATION, service_name, &request, refnumber);

leave:
    xmlFreeNode(request);
    return err;
}


/* Assign new user to given box.
 * @context is session context
 * @box is box identification
 * @user defines new user to add
 * @approval is optional external approval of box manipulation
 * @refnumber is reallocated serial number of request assigned by ISDS. Use
 * NULL, if you don't care.*/
isds_error isds_add_user(struct isds_ctx *context,
        const struct isds_DbOwnerInfo *box, const struct isds_DbUserInfo *user,
        const struct isds_approval *approval, char **refnumber) {
    return build_send_manipulationboxuser_request_check_drop_response(context,
            BAD_CAST "AddDataBoxUser", box, user, approval,
            (xmlChar **) refnumber);
}


/* Remove user assigned to given box.
 * @context is session context
 * @box is box identification
 * @user identifies user to removve
 * @approval is optional external approval of box manipulation
 * @refnumber is reallocated serial number of request assigned by ISDS. Use
 * NULL, if you don't care.*/
isds_error isds_delete_user(struct isds_ctx *context,
        const struct isds_DbOwnerInfo *box, const struct isds_DbUserInfo *user,
        const struct isds_approval *approval, char **refnumber) {
    return build_send_manipulationboxuser_request_check_drop_response(context,
            BAD_CAST "DeleteDataBoxUser", box, user, approval,
            (xmlChar **) refnumber);
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
    xmlNodePtr db_owner_info;
    xmlXPathContextPtr xpath_ctx = NULL;
    xmlXPathObjectPtr result = NULL;
    xmlChar *string = NULL;


    if (!context) return IE_INVALID_CONTEXT;
    zfree(context->long_message);
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
        isds_log_message(context, _("Could not add dbOwnerInfo child to "
                    "FindDataBox element"));
        xmlFreeNode(request);
        return IE_ERROR;
    }

    err = insert_DbOwnerInfo(context, criteria, db_owner_info);
    if (err) goto leave;


    isds_log(ILF_ISDS, ILL_DEBUG, _("Sending FindDataBox request to ISDS\n"));

    /* Sent request */
    err = isds(context, SERVICE_DB_SEARCH, request, &response, NULL, NULL);
   
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
    zfree(context->long_message);
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
        isds_log_message(context, _("Could not add dbID child to "
                    "CheckDataBox element"));
        xmlFreeNode(request);
        return IE_ERROR;
    }


    isds_log(ILF_ISDS, ILL_DEBUG, _("Sending CheckDataBox request to ISDS\n"));

    /* Sent request */
    err = isds(context, SERVICE_DB_SEARCH, request, &response, NULL, NULL);
   
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


/* Build ISDS request of XSD tIdDbInput type, sent it, check for error
 * code, destroy response and log success.
 * @context is ISDS session context.
 * @service_name is name of SERVICE_DB_MANIPULATION service
 * @box_id is UTF-8 encoded box identifier as zero terminated string 
 * @approval is optional external approval of box manipulation
 * @refnumber is reallocated serial number of request assigned by ISDS. Use
 * NULL, if you don't care. */
static isds_error build_send_manipulationdbid_request_check_drop_response(
        struct isds_ctx *context, const xmlChar *service_name, 
        const xmlChar *box_id, const struct isds_approval *approval,
        xmlChar **refnumber) {
    isds_error err = IE_SUCCESS;
    xmlDocPtr response = NULL;

    if (!context) return IE_INVALID_CONTEXT;
    zfree(context->long_message);
    if (!service_name || *service_name == '\0' || !box_id) return IE_INVAL;

    /* Check if connection is established */
    if (!context->curl) return IE_CONNECTION_CLOSED;

    /* Do request and check for success */
    err = build_send_dbid_request_check_response(context,
            SERVICE_DB_MANIPULATION, service_name, box_id, approval,
            &response, refnumber);
    xmlFreeDoc(response);

    if (!err) {
        char *service_name_locale = utf82locale((char *) service_name);
        isds_log(ILF_ISDS, ILL_DEBUG,
                _("%s request processed by server successfully.\n"),
                service_name_locale);
        free(service_name_locale);
    }

    return err;
}


/* Switch box into state where box can receive commercial messages (off by
 * default)
 * @context is ISDS session context.
 * @box_id is UTF-8 encoded box identifier as zero terminated string
 * @allow is true for enable, false for disable commercial messages income 
 * @approval is optional external approval of box manipulation
 * @refnumber is reallocated serial number of request assigned by ISDS. Use
 * NULL, if you don't care. */
isds_error isds_switch_commercial_receiving(struct isds_ctx *context,
        const char *box_id, const _Bool allow,
        const struct isds_approval *approval, char **refnumber) {
    return build_send_manipulationdbid_request_check_drop_response(context, 
            (allow) ? BAD_CAST "SetOpenAddressing" :
                BAD_CAST "ClearOpenAddressing",
            BAD_CAST box_id, approval, (xmlChar **) refnumber);
}


/* Switch box into / out of state where non-OVM box can act as OVM (e.g. force
 * message acceptance). This is just a box permission. Sender must apply
 * such role by sending each message.
 * @context is ISDS session context.
 * @box_id is UTF-8 encoded box identifier as zero terminated string
 * @allow is true for enable, false for disable OVM role permission
 * @approval is optional external approval of box manipulation
 * @refnumber is reallocated serial number of request assigned by ISDS. Use
 * NULL, if you don't care. */
isds_error isds_switch_effective_ovm(struct isds_ctx *context,
        const char *box_id, const _Bool allow,
        const struct isds_approval *approval, char **refnumber) {
    return build_send_manipulationdbid_request_check_drop_response(context, 
            (allow) ? BAD_CAST "SetEffectiveOVM" :
                BAD_CAST "ClearEffectiveOVM",
            BAD_CAST box_id, approval, (xmlChar **) refnumber);
}


/* Build ISDS request of XSD tOwnerInfoInput type, sent it, check for error
 * code, destroy response and log success.
 * @context is ISDS session context.
 * @service_name is name of SERVICE_DB_MANIPULATION service
 * @owner is structure describing box
 * @approval is optional external approval of box manipulation
 * @refnumber is reallocated serial number of request assigned by ISDS. Use
 * NULL, if you don't care. */
static isds_error build_send_manipulationdbowner_request_check_drop_response(
        struct isds_ctx *context, const xmlChar *service_name, 
        const struct isds_DbOwnerInfo *owner,
        const struct isds_approval *approval, xmlChar **refnumber) {
    isds_error err = IE_SUCCESS;
    char *service_name_locale = NULL;
    xmlNodePtr request = NULL, db_owner_info;
    xmlNsPtr isds_ns = NULL;


    if (!context) return IE_INVALID_CONTEXT;
    zfree(context->long_message);
    if (!service_name || *service_name == '\0' || !owner) return IE_INVAL;

    service_name_locale = utf82locale((char*)service_name);
    if (!service_name_locale) {
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


    /* Add XSD:tOwnerInfoInput child*/
    INSERT_ELEMENT(db_owner_info, request, "dbOwnerInfo");
    err = insert_DbOwnerInfo(context, owner, db_owner_info);
    if (err) goto leave;

    /* Add XSD:gExtApproval*/
    err = insert_GExtApproval(context, approval, request);
    if (err) goto leave;

    /* Send it to server and process response */
    err = send_request_check_drop_response(context, SERVICE_DB_MANIPULATION,
            service_name, &request, refnumber);

leave:
    xmlFreeNode(request);
    free(service_name_locale);

    return err;
}


/* Switch box accessibility state on request of box owner.
 * Despite the name, owner must do the request off-line. This function is
 * designed for such off-line meeting points (e.g. Czech POINT).
 * @context is ISDS session context.
 * @box identifies box to swith accesibilty state.
 * @allow is true for making accesibale, false to disallow access.
 * @approval is optional external approval of box manipulation
 * @refnumber is reallocated serial number of request assigned by ISDS. Use
 * NULL, if you don't care. */
isds_error isds_switch_box_accessibility_on_owner_request(
        struct isds_ctx *context, const struct isds_DbOwnerInfo *box,
        const _Bool allow, const struct isds_approval *approval,
        char **refnumber) {
    return build_send_manipulationdbowner_request_check_drop_response(context,
            (allow) ? BAD_CAST "EnableOwnDataBox" :
                BAD_CAST "DisableOwnDataBox",
            box, approval, (xmlChar **) refnumber);
}


/* Disable box accessibility on law enforcement (e.g. by prison) since exact
 * date.
 * @context is ISDS session context.
 * @box identifies box to swith accesibilty state.
 * @since is date since accesseibility has been denied. This can be past too.
 * Only tm_year, tm_mon and tm_mday carry sane value.
 * @approval is optional external approval of box manipulation
 * @refnumber is reallocated serial number of request assigned by ISDS. Use
 * NULL, if you don't care. */
isds_error isds_disable_box_accessibility_externaly(
        struct isds_ctx *context, const struct isds_DbOwnerInfo *box,
        const struct tm *since, const struct isds_approval *approval,
        char **refnumber) {
    isds_error err = IE_SUCCESS;
    char *service_name_locale = NULL;
    xmlNodePtr request = NULL, node;
    xmlNsPtr isds_ns = NULL;
    xmlChar *string = NULL;


    if (!context) return IE_INVALID_CONTEXT;
    zfree(context->long_message);
    if (!box || !since) return IE_INVAL;

    /* Build request */
    request = xmlNewNode(NULL, BAD_CAST "DisableDataBoxExternally");
    if (!request) {
        isds_printf_message(context,
                _("Could not build %s request"), "DisableDataBoxExternally");
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


    /* Add @box identification */
    INSERT_ELEMENT(node, request, "dbOwnerInfo");
    err = insert_DbOwnerInfo(context, box, node);
    if (err) goto leave;

    /* Add @since date */
    err = tm2datestring(since, &string);
    if(err) {
        isds_log_message(context,
                _("Could not convert `since' argument to ISO date string"));
        goto leave;
    }
    INSERT_STRING(request, "dbOwnerDisableDate", string);
    zfree(string);

    /* Add @approval */
    err = insert_GExtApproval(context, approval, request);
    if (err) goto leave;

    /* Send it to server and process response */
    err = send_request_check_drop_response(context, SERVICE_DB_MANIPULATION,
            BAD_CAST "DisableDataBoxExternally", &request,
            (xmlChar **) refnumber);

leave:
    free(string);
    xmlFreeNode(request);
    free(service_name_locale);

    return err;
}


/* Insert struct isds_message data (envelope (recipient data optional) and
 * documents) into XML tree
 * @context is sesstion context
 * @outgoing_message is libsids structure with message data
 * @create_message is XML CreateMessage or CreateMultipleMessage element
 * @process_recipient true for recipient data serialization, false for no
 * serialization */
static isds_error insert_envelope_files(struct isds_ctx *context,
        const struct isds_message *outgoing_message, xmlNodePtr create_message,
        const _Bool process_recipient) {

    isds_error err = IE_SUCCESS;
    xmlNodePtr envelope, dm_files, node;
    xmlChar *string = NULL;

    if (!context) return IE_INVALID_CONTEXT;
    if (!outgoing_message || !create_message) return IE_INVAL;


    /* Build envelope */
    envelope = xmlNewChild(create_message, NULL, BAD_CAST "dmEnvelope", NULL);
    if (!envelope) {
        isds_printf_message(context, _("Could not add dmEnvelope child to "
                    "%s element"), create_message->name);
        return IE_ERROR;
    }

    if (!outgoing_message->envelope) {
        isds_log_message(context, _("Outgoing message is missing envelope"));
        err = IE_INVAL;
        goto leave;
    }

    INSERT_STRING(envelope, "dmSenderOrgUnit",
            outgoing_message->envelope->dmSenderOrgUnit);
    INSERT_LONGINT(envelope, "dmSenderOrgUnitNum",
            outgoing_message->envelope->dmSenderOrgUnitNum, string);

    if (process_recipient) {
        if (!outgoing_message->envelope->dbIDRecipient) {
            isds_log_message(context,
                    _("Outgoing message is missing recipient box identifier"));
            err = IE_INVAL;
            goto leave;
        }
        INSERT_STRING(envelope, "dbIDRecipient",
                outgoing_message->envelope->dbIDRecipient);

        INSERT_STRING(envelope, "dmRecipientOrgUnit",
                outgoing_message->envelope->dmRecipientOrgUnit);
        INSERT_LONGINT(envelope, "dmRecipientOrgUnitNum",
                outgoing_message->envelope->dmRecipientOrgUnitNum, string);
        INSERT_STRING(envelope, "dmToHands",
                outgoing_message->envelope->dmToHands);
    }

    CHECK_FOR_STRING_LENGTH(outgoing_message->envelope->dmAnnotation, 0, 255,
            "dmAnnotation");
    INSERT_STRING(envelope, "dmAnnotation",
            outgoing_message->envelope->dmAnnotation);

    CHECK_FOR_STRING_LENGTH(outgoing_message->envelope->dmRecipientRefNumber,
            0, 50, "dmRecipientRefNumber");
    INSERT_STRING(envelope, "dmRecipientRefNumber",
            outgoing_message->envelope->dmRecipientRefNumber);

    CHECK_FOR_STRING_LENGTH(outgoing_message->envelope->dmSenderRefNumber,
            0, 50, "dmSenderRefNumber");
    INSERT_STRING(envelope, "dmSenderRefNumber",
            outgoing_message->envelope->dmSenderRefNumber);

    CHECK_FOR_STRING_LENGTH(outgoing_message->envelope->dmRecipientIdent,
            0, 50, "dmRecipientIdent");
    INSERT_STRING(envelope, "dmRecipientIdent",
            outgoing_message->envelope->dmRecipientIdent);

    CHECK_FOR_STRING_LENGTH(outgoing_message->envelope->dmSenderIdent,
            0, 50, "dmSenderIdent");
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

    /* ???: Should we require value for dbEffectiveOVM sender?
     * ISDS has default as true */
    INSERT_BOOLEAN(envelope, "dmOVM", outgoing_message->envelope->dmOVM);


    /* Append dmFiles */
    if (!outgoing_message->documents) {
        isds_log_message(context,
                _("Outgoing message is missing list of documents"));
        err = IE_INVAL;
        goto leave;
    }
    dm_files = xmlNewChild(create_message, NULL, BAD_CAST "dmFiles", NULL);
    if (!dm_files) {
        isds_printf_message(context, _("Could not add dmFiles child to "
                    "%s element"), create_message->name);
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
                    _("List of documents contains empty item"));
            err = IE_INVAL;
            goto leave;
        }
        /* FIXME: Check for dmFileMetaType and for document references.
         * Only first document can be of MAIN type */
        err = insert_document(context, (struct isds_document*) item->data,
                dm_files);

        if (err) goto leave;
    }
    
leave:
    free(string);
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
    xmlNodePtr request = NULL;
    xmlDocPtr response = NULL;
    xmlChar *code = NULL, *message = NULL;
    xmlXPathContextPtr xpath_ctx = NULL;
    xmlXPathObjectPtr result = NULL;
    _Bool message_is_complete = 0;

    if (!context) return IE_INVALID_CONTEXT;
    zfree(context->long_message);
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

    /* Append envelope and files */
    err = insert_envelope_files(context, outgoing_message, request, 1);
    if (err) goto leave;


    /* Signal we can serilize message since now */
    message_is_complete = 1;
    

    isds_log(ILF_ISDS, ILL_DEBUG, _("Sending CreateMessage request to ISDS\n"));

    /* Sent request */
    err = isds(context, SERVICE_DM_OPERATIONS, request, &response, NULL, NULL);
   
    /* Dont' destroy request, we want to provide it to application later */

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

    /* Request processed, but refused by server or server failed */
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
                    "but did not return assigned message ID\n"));
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


/* Send a message via ISDS to a multiple recipents
 * @context is session context
 * @outgoing_message is message to send; Some memebers are mandatory,
 * some are optional and some are irrelevant (especialy data
 * about sender). Data about recipient will be substituted by ISDS from
 * @copies. Included pointer to isds_list documents must
 * contain at least one document of FILEMETATYPE_MAIN.
 * @copies is list of isds_message_copy structures addressing all desired
 * recipients. This is read-write structure, some members will be filled with
 * valid data from ISDS (message IDs, error codes, error descriptions).
 * @return
 *  ISDS_SUCCESS if all messages have been sent
 *  ISDS_PARTIAL_SUCCESS if sending of some messages has failed (failed and
 *      succesed messages can be identified by copies->data->error),
 *  or other error code if something other goes wrong. */
isds_error isds_send_message_to_multiple_recipients(struct isds_ctx *context,
        const struct isds_message *outgoing_message,
        struct isds_list *copies) {

    isds_error err = IE_SUCCESS, append_err;
    xmlNsPtr isds_ns = NULL;
    xmlNodePtr request = NULL, recipients, recipient, node;
    struct isds_list *item;
    struct isds_message_copy *copy;
    xmlDocPtr response = NULL;
    xmlChar *code = NULL, *message = NULL;
    xmlXPathContextPtr xpath_ctx = NULL;
    xmlXPathObjectPtr result = NULL;
    xmlChar *string = NULL;
    int i;

    if (!context) return IE_INVALID_CONTEXT;
    zfree(context->long_message);
    if (!outgoing_message || !copies) return IE_INVAL;

    /* Check if connection is established
     * TODO: This check should be done donwstairs. */
    if (!context->curl) return IE_CONNECTION_CLOSED;


    /* Build CreateMultipleMessage request */
    request = xmlNewNode(NULL, BAD_CAST "CreateMultipleMessage");
    if (!request) {
        isds_log_message(context,
                _("Could not build CreateMultipleMessage request"));
        return IE_ERROR;
    }
    isds_ns = xmlNewNs(request, BAD_CAST ISDS_NS, NULL);
    if(!isds_ns) {
        isds_log_message(context, _("Could not create ISDS name space"));
        xmlFreeNode(request);
        return IE_ERROR;
    }
    xmlSetNs(request, isds_ns);


    /* Build recipients */
    recipients = xmlNewChild(request, NULL, BAD_CAST "dmRecipients", NULL);
    if (!recipients) {
        isds_log_message(context, _("Could not add dmRecipients child to "
                    "CreateMultipleMessage element"));
        xmlFreeNode(request);
        return IE_ERROR;
    }

    /* Insert each recipient */
    for (item = copies; item; item = item->next) {
        copy = (struct isds_message_copy *) item->data;
        if (!copy) {
            isds_log_message(context,
                    _("`copies' list item contains empty data"));
            err = IE_INVAL;
            goto leave;
        }

        recipient = xmlNewChild(recipients, NULL, BAD_CAST "dmRecipient", NULL);
        if (!recipient) {
            isds_log_message(context, _("Could not add dmRecipient child to "
                        "dmRecipients element"));
            err = IE_ERROR;
            goto leave;
        }

        if (!copy->dbIDRecipient) {
            isds_log_message(context,
                    _("Message copy is missing recipient box identifier"));
            err = IE_INVAL;
            goto leave;
        }
        INSERT_STRING(recipient, "dbIDRecipient", copy->dbIDRecipient);
        INSERT_STRING(recipient, "dmRecipientOrgUnit",
                copy->dmRecipientOrgUnit);
        INSERT_LONGINT(recipient, "dmRecipientOrgUnitNum",
                copy->dmRecipientOrgUnitNum, string);
        INSERT_STRING(recipient, "dmToHands", copy->dmToHands);
    }

    /* Append envelope and files */
    err = insert_envelope_files(context, outgoing_message, request, 0);
    if (err) goto leave;


    isds_log(ILF_ISDS, ILL_DEBUG,
            _("Sending CreateMultipleMessage request to ISDS\n"));

    /* Sent request */
    err = isds(context, SERVICE_DM_OPERATIONS, request, &response, NULL, NULL);
    if (err) {
        isds_log(ILF_ISDS, ILL_DEBUG,
                _("Processing ISDS response on CreateMultipleMessage "
                    "request failed\n"));
        goto leave;
    }

    /* Check for response status */
    err = isds_response_status(context, SERVICE_DM_OPERATIONS, response,
            &code, &message, NULL);
    if (err) {
        isds_log(ILF_ISDS, ILL_DEBUG,
                _("ISDS response on CreateMultipleMessage request "
                    "is missing status\n"));
        goto leave;
    }

    /* Request processed, but some copies failed */
    if (!xmlStrcmp(code, BAD_CAST "0004")) {
        char *box_id_locale =
            utf82locale((char*)outgoing_message->envelope->dbIDRecipient);
        char *code_locale = utf82locale((char*)code);
        char *message_locale = utf82locale((char*)message);
        isds_log(ILF_ISDS, ILL_DEBUG,
                _("Server did accept message for multiple recipients "
                    "on CreateMultipleMessage request but delivery to "
                    "some of them failed (code=%s, message=%s)\n"),
                box_id_locale, code_locale, message_locale);
        isds_log_message(context, message_locale);
        free(box_id_locale);
        free(code_locale);
        free(message_locale);
        err = IE_PARTIAL_SUCCESS;
    }

    /* Request refused by server as whole */
    else if (xmlStrcmp(code, BAD_CAST "0000")) {
        char *box_id_locale =
            utf82locale((char*)outgoing_message->envelope->dbIDRecipient);
        char *code_locale = utf82locale((char*)code);
        char *message_locale = utf82locale((char*)message);
        isds_log(ILF_ISDS, ILL_DEBUG,
                _("Server did not accept message for multiple recipients "
                    "on CreateMultipleMessage request (code=%s, message=%s)\n"),
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
    result = xmlXPathEvalExpression(
            BAD_CAST "/isds:CreateMultipleMessageResponse"
            "/isds:dmMultipleStatus/isds:dmSingleStatus",
            xpath_ctx);
    if (!result) {
        err = IE_ERROR;
        goto leave;
    }
    if (xmlXPathNodeSetIsEmpty(result->nodesetval)) {
        isds_log_message(context, _("Missing isds:dmSingleStatus element"));
        err = IE_ISDS;
        goto leave;
    }

    /* Extract message ID and delivery status for each copy */ 
    for (item = copies, i = 0; item && i < result->nodesetval->nodeNr;
            item = item->next, i++) {
        copy = (struct isds_message_copy *) item->data;
        xpath_ctx->node = result->nodesetval->nodeTab[i];

        append_err = append_TMStatus(context, copy, xpath_ctx);
        if (append_err) {
            err = append_err;
            goto leave;
        }
    }
    if (item || i < result->nodesetval->nodeNr) {
        isds_printf_message(context, _("ISDS returned unexpected number of "
                    "message copy delivery states: %d"),
                    result->nodesetval->nodeNr);
        err = IE_ISDS;
        goto leave;
    }


leave:
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
                _("CreateMultipleMessageResponse request processed by server "
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
    zfree(context->long_message);
   
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
    err = isds(context, SERVICE_DM_INFO, request, &response, NULL, NULL);
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
 * @raw_response is automatically reallocated bitstream with response body. Use
 * NULL if you don't care
 * @raw_response_length is size of @raw_response in bytes
 * @code is ISDS status code
 * @status_message is ISDS status message
 * @return error coded from lower layer, context message will be set up
 * appropriately. */
static isds_error build_send_check_message_request(struct isds_ctx *context,
        const isds_service service, const xmlChar *service_name,
        const char *message_id,
        xmlDocPtr *response, void **raw_response, size_t *raw_response_length,
        xmlChar **code, xmlChar **status_message) {

    isds_error err = IE_SUCCESS;
    char *service_name_locale = NULL, *message_id_locale = NULL;
    xmlNodePtr request = NULL, node;
    xmlNsPtr isds_ns = NULL;

    if (!context) return IE_INVALID_CONTEXT;
    if (!service_name || !message_id) return IE_INVAL;
    if (!response || !code || !status_message) return IE_INVAL;
    if (!raw_response_length && raw_response) return IE_INVAL;

    /* Free output argument */
    xmlFreeDoc(*response); *response = NULL;
    if (raw_response) zfree(*raw_response);
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
    err = isds(context, service, request, response,
            raw_response, raw_response_length);
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


/* Find dmSignature in ISDS response, extract decoded CMS structure, extract
 * signed data and free ISDS response.
 * @context is session context
 * @message_id is UTF-8 encoded message ID for loging purpose
 * @response is parsed XML document. It will be freed and NULLed in the middle
 * of function run to save memmory. This is not guaranted in case of error.
 * @request_name is name of ISDS request used to construct response root
 * element name and for logging purpose.
 * @raw is reallocated output buffer with DER encoded CMS data
 * @raw_length is size of @raw buffer in bytes
 * @returns standard error codes, in case of error, @raw will be freed and
 * NULLed, @response sometimes. */
static isds_error find_extract_signed_data_free_response(
        struct isds_ctx *context, const xmlChar *message_id,
        xmlDocPtr *response, const xmlChar *request_name,
        void **raw, size_t *raw_length) {

    isds_error err = IE_SUCCESS;
    char *xpath_expression = NULL;
    xmlXPathContextPtr xpath_ctx = NULL;
    xmlXPathObjectPtr result = NULL;
    char *encoded_structure = NULL;

    if (!context) return IE_INVALID_CONTEXT;
    if (!raw) return IE_INVAL;
    zfree(*raw);
    if (!message_id || !response || !*response || !request_name || !raw_length)
        return IE_INVAL;

    /* Build XPath expression */
    xpath_expression = astrcat3("/isds:", (char *) request_name,
            "Response/isds:dmSignature");
    if (!xpath_expression) return IE_NOMEM;
   
    /* Extract data */
    xpath_ctx = xmlXPathNewContext(*response);
    if (!xpath_ctx) {
        err = IE_ERROR;
        goto leave;
    }
    if (register_namespaces(xpath_ctx, MESSAGE_NS_UNSIGNED)) {
        err = IE_ERROR;
        goto leave;
    }
    result = xmlXPathEvalExpression(BAD_CAST xpath_expression, xpath_ctx);
    if (!result) {
        err = IE_ERROR;
        goto leave;
    }
    /* Empty response */
    if (xmlXPathNodeSetIsEmpty(result->nodesetval)) {
        char *message_id_locale = utf82locale((char*) message_id);
        isds_printf_message(context,
                _("Server did not return any signed data for mesage ID `%s' "
                    "on %s request"),
                message_id_locale, request_name);
        free(message_id_locale);
        err = IE_ISDS;
        goto leave;
    }
    /* More reponses */
    if (result->nodesetval->nodeNr > 1) {
        char *message_id_locale = utf82locale((char*) message_id);
        isds_printf_message(context,
                _("Server did return more signed data for message ID `%s' "
                    "on %s request"),
                message_id_locale, request_name);
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

    /* Here we have delivery info as standalone CMS in encoded_structure.
     * We don't need any other data, free them: */
    xmlXPathFreeObject(result); result = NULL;
    xmlXPathFreeContext(xpath_ctx); xpath_ctx = NULL;
    xmlFreeDoc(*response); *response = NULL;


    /* Decode PKCS#7 to DER format */
    *raw_length = b64decode(encoded_structure, raw);
    if (*raw_length == (size_t) -1) {
        isds_log_message(context,
                _("Error while Base64-decoding PKCS#7 structure"));
        err = IE_ERROR;
        goto leave;
    }
  
leave:
    if (err) {
        zfree(*raw);
        raw_length = 0;
    }

    free(encoded_structure);
    xmlXPathFreeObject(result);
    xmlXPathFreeContext(xpath_ctx);
    free(xpath_expression);

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
    zfree(context->long_message);
   
    /* Free former message if any */
    if (!message) return IE_INVAL;
    isds_message_free(message);

    /* Do request and check for success */
    err = build_send_check_message_request(context, SERVICE_DM_INFO,
            BAD_CAST "MessageEnvelopeDownload", message_id,
            &response, NULL, NULL, &code, &status_message);
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


/* Load delivery info of any format from buffer.
 * @context is session context
 * @raw_type advertises format of @buffer content. Only delivery info types
 * are accepted.
 * @buffer is DER encoded PKCS#7 structure with signed delivery info. You can
 * retrieve such data from message->raw after calling
 * isds_get_signed_delivery_info().
 * @length is length of buffer in bytes.
 * @message is automatically reallocated message parsed from @buffer.
 * @strategy selects how buffer will be attached into raw isds_message member.
 * */
isds_error isds_load_delivery_info(struct isds_ctx *context,
        const isds_raw_type raw_type,
        const void *buffer, const size_t length,
        struct isds_message **message, const isds_buffer_strategy strategy) {

    isds_error err = IE_SUCCESS;
    message_ns_type message_ns;
    xmlDocPtr message_doc = NULL;
    xmlXPathContextPtr xpath_ctx = NULL;
    xmlXPathObjectPtr result = NULL;
    void *xml_stream = NULL;
    size_t xml_stream_length = 0;

    if (!context) return IE_INVALID_CONTEXT;
    zfree(context->long_message);
    if (!message) return IE_INVAL;
    isds_message_free(message);
    if (!buffer) return IE_INVAL;


    /* Select buffer format and extract XML from CMS*/
    switch (raw_type) {
        case RAWTYPE_DELIVERYINFO:
            message_ns = MESSAGE_NS_UNSIGNED;
            xml_stream = (void *) buffer;
            xml_stream_length = length;
            break;

        case RAWTYPE_PLAIN_SIGNED_DELIVERYINFO:
            message_ns = MESSAGE_NS_SIGNED_DELIVERY;
            xml_stream = (void *) buffer;
            xml_stream_length = length;
            break;

        case RAWTYPE_CMS_SIGNED_DELIVERYINFO:
            message_ns = MESSAGE_NS_SIGNED_DELIVERY;
            err = extract_cms_data(context, buffer, length,
                    &xml_stream, &xml_stream_length);
            if (err) goto leave;
            break;

        default:
            isds_log_message(context, _("Bad raw delivery representation type"));
            return IE_INVAL;
            break;
    }

    isds_log(ILF_ISDS, ILL_DEBUG,
            _("Delivery info content:\n%.*s\nEnd of delivery info\n"),
            xml_stream_length, xml_stream);

    /* Convert delivery info XML stream into XPath context */
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
    /* XXX: Name spaces mangled for signed delivery info:
     * http://isds.czechpoint.cz/v20/delivery:
     *
     * <q:GetDeliveryInfoResponse xmlns:q="http://isds.czechpoint.cz/v20/delivery">
     *   <q:dmDelivery>
     *     <p:dmDm xmlns:p="http://isds.czechpoint.cz/v20">
     *       <p:dmID>170272</p:dmID>
     *       ...
     *     </p:dmDm>
     *     <q:dmHash algorithm="SHA-1">...</q:dmHash>
     *     ...
     *     </q:dmEvents>...</q:dmEvents>
     *   </q:dmDelivery>
     * </q:GetDeliveryInfoResponse>
     * */
    if (register_namespaces(xpath_ctx, message_ns)) {
        err = IE_ERROR;
        goto leave;
    }
    result = xmlXPathEvalExpression( 
            BAD_CAST "/sisds:GetDeliveryInfoResponse/sisds:dmDelivery",
            xpath_ctx);
    if (!result) {
        err = IE_ERROR;
        goto leave;
    }
    /* Empty delivery info */
    if (xmlXPathNodeSetIsEmpty(result->nodesetval)) {
        isds_printf_message(context,
                _("XML document ss not sisds:dmDelivery document"));
        err = IE_ISDS;
        goto leave;
    }
    /* More delivery infos */
    if (result->nodesetval->nodeNr > 1) {
        isds_printf_message(context,
                _("XML document has more sisds:dmDelivery elements"));
        err = IE_ISDS;
        goto leave;
    }
    /* One delivery info */
    xpath_ctx->node = result->nodesetval->nodeTab[0];

    /* Extract the envelope (= message without documents, hence 0).
     * XXX: extract_TReturnedMessage() can obtain attachments size,
     * but delivery info carries none. It's coded as option elements,
     * so it should work. */
    err = extract_TReturnedMessage(context, 0, message, xpath_ctx);
    if (err) goto leave;

    /* Extract events */
    err = move_xpathctx_to_child(context, BAD_CAST "sisds:dmEvents", xpath_ctx);
    if (err == IE_NOEXIST || err == IE_NOTUNIQ) { err = IE_ISDS; goto leave; }
    if (err) { err = IE_ERROR; goto leave; }
    err = extract_events(context, &(*message)->envelope->events, xpath_ctx);
    if (err) goto leave;

    /* Append raw CMS structure into message */
    (*message)->raw_type = raw_type;
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

    xmlXPathFreeObject(result);
    xmlXPathFreeContext(xpath_ctx);
    xmlFreeDoc(message_doc);
    if (xml_stream != buffer) cms_data_free(xml_stream);

    if (!err)
        isds_log(ILF_ISDS, ILL_DEBUG,
                _("Delivery info loaded successfully.\n"));
    return err;
}


/* Download signed delivery infosheet of given message identified by ID.
 * @context is session context
 * @message_id is message identifier (you can get them from
 * isds_get_list_of_{sent,received}_messages())
 * @message is automatically reallocated message retrieved from ISDS.
 * It will miss documents per se. Use isds_get_signed_received_message(),
 * if you are interrested in documents (content). OTOH, only this function
 * can get list events message has gone through. */
isds_error isds_get_signed_delivery_info(struct isds_ctx *context,
        const char *message_id, struct isds_message **message) {

    isds_error err = IE_SUCCESS;
    xmlDocPtr response = NULL;
    xmlChar *code = NULL, *status_message = NULL;
    void *raw = NULL;
    size_t raw_length = 0;

    if (!context) return IE_INVALID_CONTEXT;
    zfree(context->long_message);
   
    /* Free former message if any */
    if (!message) return IE_INVAL;
    isds_message_free(message);

    /* Do request and check for success */
    err = build_send_check_message_request(context, SERVICE_DM_INFO,
            BAD_CAST "GetSignedDeliveryInfo", message_id,
            &response, NULL, NULL, &code, &status_message);
    if (err) goto leave;

    /* Find signed delivery info, extract it into raw and maybe free
     * response */
    err = find_extract_signed_data_free_response(context,
            (xmlChar *)message_id, &response,
            BAD_CAST "GetSignedDeliveryInfo", &raw, &raw_length);
    if (err) goto leave;
  
    /* Parse delivery info */
    err = isds_load_delivery_info(context,
            RAWTYPE_CMS_SIGNED_DELIVERYINFO, raw, raw_length,
            message, BUFFER_MOVE);
    if (err) goto leave;

    raw = NULL;

leave:
    if (err) {
        isds_message_free(message);
    }

    free(raw);
    free(code);
    free(status_message);
    xmlFreeDoc(response);

    if (!err)
        isds_log(ILF_ISDS, ILL_DEBUG,
                _("GetSignedDeliveryInfo request processed by server "
                        "successfully.\n")
                );
    return err;
}


/* Download delivery infosheet of given message identified by ID.
 * @context is session context
 * @message_id is message identifier (you can get them from
 * isds_get_list_of_{sent,received}_messages())
 * @message is automatically reallocated message retrieved from ISDS.
 * It will miss documents per se. Use isds_get_received_message(), if you are
 * interrested in documents (content). OTOH, only this function can get list
 * events message has gone through. */
isds_error isds_get_delivery_info(struct isds_ctx *context,
        const char *message_id, struct isds_message **message) {

    isds_error err = IE_SUCCESS;
    xmlDocPtr response = NULL;
    xmlChar *code = NULL, *status_message = NULL;
    xmlXPathContextPtr xpath_ctx = NULL;
    xmlXPathObjectPtr result = NULL;
    xmlNodePtr delivery_node = NULL;

    if (!context) return IE_INVALID_CONTEXT;
    zfree(context->long_message);
   
    /* Free former message if any */
    if (!message) return IE_INVAL;
    isds_message_free(message);

    /* Do request and check for success */
    err = build_send_check_message_request(context, SERVICE_DM_INFO,
            BAD_CAST "GetDeliveryInfo", message_id,
            &response, NULL, NULL, &code, &status_message);
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
            BAD_CAST "/isds:GetDeliveryInfoResponse/isds:dmDelivery",
            xpath_ctx);
    if (!result) {
        err = IE_ERROR;
        goto leave;
    }
    /* Empty response */
    if (xmlXPathNodeSetIsEmpty(result->nodesetval)) {
        char *message_id_locale = utf82locale((char*) message_id);
        isds_printf_message(context,
                _("Server did not return any delivery info for ID `%s' "
                    "on GetDeliveryInfo request"), message_id_locale);
        free(message_id_locale);
        err = IE_ISDS;
        goto leave;
    }
    /* More delivery infos */
    if (result->nodesetval->nodeNr > 1) {
        char *message_id_locale = utf82locale((char*) message_id);
        isds_printf_message(context,
                _("Server did return more delivery infos for ID `%s' "
                    "on GetDeliveryInfo request"), message_id_locale);
        free(message_id_locale);
        err = IE_ISDS;
        goto leave;
    }
    /* One delivery info */
    xpath_ctx->node = delivery_node = result->nodesetval->nodeTab[0];

    /* Extract the envelope (= message without documents, hence 0).
     * XXX: extract_TReturnedMessage() can obtain attachments size,
     * but delivery info carries none. It's coded as option elements,
     * so it should work. */
    err = extract_TReturnedMessage(context, 0, message, xpath_ctx);
    if (err) goto leave;

    /* Extract events */
    err = move_xpathctx_to_child(context, BAD_CAST "isds:dmEvents", xpath_ctx);
    if (err == IE_NOEXIST || err == IE_NOTUNIQ) { err = IE_ISDS; goto leave; }
    if (err) { err = IE_ERROR; goto leave; }
    err = extract_events(context, &(*message)->envelope->events, xpath_ctx);
    if (err) goto leave;

     /* Save XML blob */
    err = serialize_subtree(context, delivery_node, &(*message)->raw,
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
                    _("GetDeliveryInfo request processed by server "
                        "successfully.\n")
                );
    return err;
}


/* Load incoming message from buffer.
 * @context is session context
 * @buffer XML stream with unsigned message. You can retrieve such data from
 * message->raw after calling isds_get_received_message().
 * @length is length of buffer in bytes.
 * @message is automatically reallocated message parsed from @buffer.
 * @strategy selects how buffer will be attached into raw isds_message member.
 * */
isds_error isds_load_received_message(struct isds_ctx *context,
        const void *buffer, const size_t length,
        struct isds_message **message, const isds_buffer_strategy strategy) {

    isds_error err = IE_SUCCESS;
    xmlDocPtr message_doc = NULL;
    xmlXPathContextPtr xpath_ctx = NULL;
    xmlXPathObjectPtr result = NULL;

    if (!context) return IE_INVALID_CONTEXT;
    zfree(context->long_message);
    if (!message) return IE_INVAL;
    isds_message_free(message);
    if (!buffer) return IE_INVAL;


    isds_log(ILF_ISDS, ILL_DEBUG, 
            _("Incoming message content:\n%.*s\nEnd of message\n"),
            length, buffer);

    /* Convert extracted messages XML stream into XPath context */
    message_doc = xmlParseMemory(buffer, length);
    if (!message_doc) {
        err = IE_XML;
        goto leave;
    }
    xpath_ctx = xmlXPathNewContext(message_doc);
    if (!xpath_ctx) {
        err = IE_ERROR;
        goto leave;
    }
    /* XXX: Standard name space */
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
    /* Missing dmReturnedMessage */
    if (xmlXPathNodeSetIsEmpty(result->nodesetval)) {
        isds_printf_message(context,
                _("XML document does not contain isds:dmReturnedMessage "
                    "element"));
        err = IE_ISDS;
        goto leave;
    }
    /* More elements. This should never happen. */
    if (result->nodesetval->nodeNr > 1) {
        isds_printf_message(context,
                _("XML document has more isds:dmReturnedMessage elements"));
        err = IE_ISDS;
        goto leave;
    }
    /* One message */
    xpath_ctx->node = result->nodesetval->nodeTab[0];

    /* Extract the message */
    err = extract_TReturnedMessage(context, 1, message, xpath_ctx);
    if (err) goto leave;

    /* Append XML stream into message */
    (*message)->raw_type = RAWTYPE_INCOMING_MESSAGE; 
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
    xmlXPathFreeObject(result);
    xmlXPathFreeContext(xpath_ctx);

    if (!err)
        isds_log(ILF_ISDS, ILL_DEBUG,
                _("Incoming message loaded successfully.\n"));
    return err;
}


/* Download incoming message identified by ID.
 * @context is session context
 * @message_id is message identifier (you can get them from
 * isds_get_list_of_received_messages())
 * @message is automatically reallocated message retrieved from ISDS */
isds_error isds_get_received_message(struct isds_ctx *context,
        const char *message_id, struct isds_message **message) {

    isds_error err = IE_SUCCESS;
    xmlDocPtr response = NULL;
    void *xml_stream = NULL;
    size_t xml_stream_length;
    xmlChar *code = NULL, *status_message = NULL;
    xmlXPathContextPtr xpath_ctx = NULL;
    xmlXPathObjectPtr result = NULL;
    char *phys_path = NULL;
    size_t phys_start, phys_end;

    if (!context) return IE_INVALID_CONTEXT;
    zfree(context->long_message);
   
    /* Free former message if any */
    if (message) isds_message_free(message);

    /* Do request and check for success */
    err = build_send_check_message_request(context, SERVICE_DM_OPERATIONS,
            BAD_CAST "MessageDownload", message_id,
            &response, &xml_stream, &xml_stream_length,
            &code, &status_message);
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

    /* Locate raw XML blob */
    phys_path = strdup(
            SOAP_NS PHYSXML_NS_SEPARATOR "Envelope"
            PHYSXML_ELEMENT_SEPARATOR
            SOAP_NS PHYSXML_NS_SEPARATOR "Body"
            PHYSXML_ELEMENT_SEPARATOR
            ISDS_NS PHYSXML_NS_SEPARATOR "MessageDownloadResponse"
    );
    if (!phys_path) {
        err = IE_NOMEM;
        goto leave;
    }
    err = find_element_boundary(xml_stream, xml_stream_length,
            phys_path, &phys_start, &phys_end);
    zfree(phys_path);
    if (err) {
        isds_log_message(context,
                _("Substring with isds:MessageDownloadResponse element "
                    "could not be located in raw SOAP message"));
        goto leave;
    }
     /* Save XML blob */
    /*err = serialize_subtree(context, xpath_ctx->node, &(*message)->raw,
            &(*message)->raw_length);*/
    /* TODO: Store name space declarations from ancestors */
    /* TODO: Handle non-UTF-8 encoding (XML prologue) */
    (*message)->raw_type = RAWTYPE_INCOMING_MESSAGE;
    (*message)->raw_length = phys_end - phys_start + 1;
    (*message)->raw = malloc((*message)->raw_length);
    if (!(*message)->raw) {
        err = IE_NOMEM;
        goto leave;
    }
    memcpy((*message)->raw, xml_stream + phys_start, (*message)->raw_length);


leave:
    if (err) {
        isds_message_free(message);
    }

    free(phys_path);

    xmlXPathFreeObject(result);
    xmlXPathFreeContext(xpath_ctx);

    free(code);
    free(status_message);
    free(xml_stream);
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
    zfree(context->long_message);
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
                _("XML document embedded into PKCS#7 structure has more "
                    "root sisds:dmReturnedMessage elements"));
        err = IE_ISDS;
        goto leave;
    }
    /* One embedded message */
    xpath_ctx->node = result->nodesetval->nodeTab[0];

    /* Extract the message */
    err = extract_TReturnedMessage(context, 1, message, xpath_ctx);
    if (err) goto leave;

    /* Append raw CMS structure into message */
    (*message)->raw_type = (outgoing) ?  RAWTYPE_CMS_SIGNED_OUTGOING_MESSAGE :
        RAWTYPE_CMS_SIGNED_INCOMING_MESSAGE;
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


/* Load message of any type from buffer.
 * @context is session context
 * @raw_type defines content type of @buffer. Only message types are allowed.
 * @buffer is message raw representation. Format (CMS, plain signed,
 * message direction) is defined in @raw_type. You can retrieve such data
 * from message->raw after calling isds_get_[signed]{received,sent}_message().
 * @length is length of buffer in bytes.
 * @message is automatically reallocated message parsed from @buffer.
 * @strategy selects how buffer will be attached into raw isds_message member.
 * */
isds_error isds_load_message(struct isds_ctx *context,
        const isds_raw_type raw_type, const void *buffer, const size_t length,
        struct isds_message **message, const isds_buffer_strategy strategy) {

    isds_error err = IE_SUCCESS;
    void *xml_stream = NULL;
    size_t xml_stream_length = 0;
    message_ns_type message_ns;
    xmlDocPtr message_doc = NULL;
    xmlXPathContextPtr xpath_ctx = NULL;
    xmlXPathObjectPtr result = NULL;

    if (!context) return IE_INVALID_CONTEXT;
    zfree(context->long_message);
    if (!message) return IE_INVAL;
    isds_message_free(message);
    if (!buffer) return IE_INVAL;


    /* Select buffer format and extract XML from CMS*/
    switch (raw_type) {
        case RAWTYPE_INCOMING_MESSAGE:
            message_ns = MESSAGE_NS_UNSIGNED;
            xml_stream = (void *) buffer;
            xml_stream_length = length;
            break;

        case RAWTYPE_PLAIN_SIGNED_INCOMING_MESSAGE:
            message_ns = MESSAGE_NS_SIGNED_INCOMING;
            xml_stream = (void *) buffer;
            xml_stream_length = length;
            break;

        case RAWTYPE_CMS_SIGNED_INCOMING_MESSAGE:
            message_ns = MESSAGE_NS_SIGNED_INCOMING;
            err = extract_cms_data(context, buffer, length,
                    &xml_stream, &xml_stream_length);
            if (err) goto leave;
            break;

        case RAWTYPE_PLAIN_SIGNED_OUTGOING_MESSAGE:
            message_ns = MESSAGE_NS_SIGNED_OUTGOING;
            xml_stream = (void *) buffer;
            xml_stream_length = length;
            break;

        case RAWTYPE_CMS_SIGNED_OUTGOING_MESSAGE:
            message_ns = MESSAGE_NS_SIGNED_OUTGOING;
            err = extract_cms_data(context, buffer, length,
                    &xml_stream, &xml_stream_length);
            if (err) goto leave;
            break;

        default:
            isds_log_message(context, _("Bad raw message representation type"));
            return IE_INVAL;
            break;
    }

    isds_log(ILF_ISDS, ILL_DEBUG,
        _("Loading message:\n%.*s\nEnd of message\n"),
        xml_stream_length, xml_stream);

    /* Convert messages XML stream into XPath context */
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
    /* XXX: Standard name space for unsigned incoming direction:
     * http://isds.czechpoint.cz/v20/
     *
     * XXX: Name spaces mangled for signed outgoing direction:
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
     * XXX: Name spaces mangled for signed incoming direction:
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
    if (register_namespaces(xpath_ctx, message_ns)) {
        err = IE_ERROR;
        goto leave;
    }
    result = xmlXPathEvalExpression( 
            BAD_CAST "/sisds:MessageDownloadResponse/sisds:dmReturnedMessage",
            xpath_ctx);
    if (!result) {
        err = IE_ERROR;
        goto leave;
    }
    /* Empty message */
    if (xmlXPathNodeSetIsEmpty(result->nodesetval)) {
        isds_printf_message(context,
                _("XML document does not contain "
                    "sisds:dmReturnedMessage element"));
        err = IE_ISDS;
        goto leave;
    }
    /* More messages */
    if (result->nodesetval->nodeNr > 1) {
        isds_printf_message(context,
                _("XML document has more sisds:dmReturnedMessage elements"));
        err = IE_ISDS;
        goto leave;
    }
    /* One message */
    xpath_ctx->node = result->nodesetval->nodeTab[0];

    /* Extract the message */
    err = extract_TReturnedMessage(context, 1, message, xpath_ctx);
    if (err) goto leave;

    /* Append raw buffer into message */
    (*message)->raw_type = raw_type;
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

    if (xml_stream != buffer) cms_data_free(xml_stream);
    xmlXPathFreeObject(result);
    xmlXPathFreeContext(xpath_ctx);
    xmlFreeDoc(message_doc);

    if (!err)
        isds_log(ILF_ISDS, ILL_DEBUG, _("Message loaded successfully.\n"));
    return err;
}


/* Determine type of raw message or delivery info according some heuristics.
 * It does not validate the raw blob.
 * @context is session context
 * @raw_type returns content type of @buffer. Valid only if exit code of this
 * function is IE_SUCCESS. The pointer must be valid. This is no automatically
 * reallocted memory.
 * @buffer is message raw representation.
 * @length is length of buffer in bytes. */
isds_error isds_guess_raw_type(struct isds_ctx *context,
        isds_raw_type *raw_type, const void *buffer, const size_t length) {
    isds_error err;
    void *xml_stream = NULL;
    size_t xml_stream_length = 0;
    xmlDocPtr document = NULL;
    xmlNodePtr root = NULL;
    
    if (!context) return IE_INVALID_CONTEXT;
    zfree(context->long_message);
    if (length == 0 || !buffer) return IE_INVAL;
    if (!raw_type) return IE_INVAL;

    /* Try CMS */
    err = extract_cms_data(context, buffer, length,
            &xml_stream, &xml_stream_length);
    if (err) {
        xml_stream = (void *) buffer;
        xml_stream_length = (size_t) length;
        err = IE_SUCCESS;
    }

    /* Try XML */
    document = xmlParseMemory(xml_stream, xml_stream_length);
    if (!document) {
        isds_printf_message(context,
                _("Could not parse data as XML document"));
        err = IE_NOTSUP;
        goto leave;
    }

    /* Get root element */
    root = xmlDocGetRootElement(document);
    if (!root) {
        isds_printf_message(context,
                _("XML document is missing root element"));
        err = IE_XML;
        goto leave;
    }

    if (!root->ns || !root->ns->href) {
        isds_printf_message(context,
                _("Root element does not belong to any name space"));
        err = IE_NOTSUP;
        goto leave;
    }

    /* Test name space */
    if (!xmlStrcmp(root->ns->href, BAD_CAST SISDS_INCOMING_NS)) {
        if (xml_stream == buffer) 
            *raw_type = RAWTYPE_PLAIN_SIGNED_INCOMING_MESSAGE;
        else
            *raw_type = RAWTYPE_CMS_SIGNED_INCOMING_MESSAGE;
    } else if (!xmlStrcmp(root->ns->href, BAD_CAST SISDS_OUTGOING_NS)) {
        if (xml_stream == buffer) 
            *raw_type = RAWTYPE_PLAIN_SIGNED_OUTGOING_MESSAGE;
        else
            *raw_type = RAWTYPE_CMS_SIGNED_OUTGOING_MESSAGE;
    } else if (!xmlStrcmp(root->ns->href, BAD_CAST SISDS_DELIVERY_NS)) {
        if (xml_stream == buffer) 
            *raw_type = RAWTYPE_PLAIN_SIGNED_DELIVERYINFO;
        else
            *raw_type = RAWTYPE_CMS_SIGNED_DELIVERYINFO;
    } else if (!xmlStrcmp(root->ns->href, BAD_CAST ISDS_NS)) {
        if (xml_stream != buffer) {
            isds_printf_message(context,
                    _("Document in ISDS name space is encapsulated into CMS" ));
            err = IE_NOTSUP;
        } else if (!xmlStrcmp(root->name, BAD_CAST "MessageDownloadResponse"))
            *raw_type = RAWTYPE_INCOMING_MESSAGE;
        else if (!xmlStrcmp(root->name, BAD_CAST "GetDeliveryInfoResponse"))
            *raw_type = RAWTYPE_DELIVERYINFO;
        else {
            isds_printf_message(context,
                    _("Unknown root element in ISDS name space"));
            err = IE_NOTSUP;
        }
    } else {
        isds_printf_message(context,
                _("Uknown namespace"));
        err = IE_NOTSUP;
    }

leave:
    if (xml_stream != buffer) cms_data_free(xml_stream);
    xmlFreeDoc(document);
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
    zfree(context->long_message);
    if (!message) return IE_INVAL;
    isds_message_free(message);

    /* Do request and check for success */
    err = build_send_check_message_request(context, SERVICE_DM_OPERATIONS,
            (outgoing) ? BAD_CAST "SignedSentMessageDownload" :
                BAD_CAST "SignedMessageDownload",
            message_id, &response, NULL, NULL, &code, &status_message);
    if (err) goto leave;

    /* Find signed message, extract it into raw and maybe free
     * response */
    err = find_extract_signed_data_free_response(context,
            (xmlChar *)message_id, &response,
            (outgoing) ? BAD_CAST "SignedSentMessageDownload" :
                BAD_CAST "SignedMessageDownload",
            &raw, &raw_length);
    if (err) goto leave;
  
    /* Parse message */
    err = isds_load_message(context,
            (outgoing) ? RAWTYPE_CMS_SIGNED_OUTGOING_MESSAGE :
                RAWTYPE_CMS_SIGNED_INCOMING_MESSAGE,
            raw, raw_length, message, BUFFER_MOVE);
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
    zfree(context->long_message);
   
    isds_hash_free(hash);

    err = build_send_check_message_request(context, SERVICE_DM_INFO,
            BAD_CAST "VerifyMessage", message_id,
            &response, NULL, NULL, &code, &status_message);
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
    zfree(context->long_message);
   
    /* Do request and check for success */
    err = build_send_check_message_request(context, SERVICE_DM_INFO,
            BAD_CAST "MarkMessageAsDownloaded", message_id,
            &response, NULL, NULL, &code, &status_message);

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

/* Mark message as received by recipient. This is applicable only to
 * commercial message. There is no specified way how to distinguishe
 * commercial message from government message yet. Government message is
 * received automatically (by law), commenrcial message on recipient request.
 * @context is session context
 * @message_id is message identifier. */
isds_error isds_mark_message_received(struct isds_ctx *context,
        const char *message_id) {

    isds_error err = IE_SUCCESS;
    xmlDocPtr response = NULL;
    xmlChar *code = NULL, *status_message = NULL;

    if (!context) return IE_INVALID_CONTEXT;
    zfree(context->long_message);
   
    /* Do request and check for success */
    err = build_send_check_message_request(context, SERVICE_DM_INFO,
            BAD_CAST "ConfirmDelivery", message_id,
            &response, NULL, NULL, &code, &status_message);

    free(code);
    free(status_message);
    xmlFreeDoc(response);

    if (!err)
        isds_log(ILF_ISDS, ILL_DEBUG,
                    _("ConfirmDelivery request processed by server "
                        "successfully.\n")
                );
    return err;
}


/* Send document for authorize conversion into Czech POINT system.
 * This is public anonymous service, no login necessary. Special context is
 * used to reuse keep-a-live HTTPS connection.
 * @context is Czech POINT session context. DO NOT use context connected to
 * ISDS server. Use new context or context used by this function previously.
 * @document is document to convert. Only data, data_length and dmFileDescr
 * memebers are signifact. Be ware that not all document formats can be
 * converted (signed PDF 1.3 and higher only (2010-02 state)).
 * @id is reallocated identifier assigned by Czech POINT system to
 * your document on submit. Use is to tell it to Czech POINT officer.
 * @date is reallocated document submit date (submitted documents
 * expires after some period). Only tm_year, tm_mon and tm_mday carry sane
 * value. */
isds_error czp_convert_document(struct isds_ctx *context,
        const struct isds_document *document,
        char **id, struct tm **date) {
    isds_error err = IE_SUCCESS;
    xmlNsPtr deposit_ns = NULL, empty_ns = NULL;
    xmlNodePtr request = NULL, node;
    xmlDocPtr response = NULL;
    xmlChar *base64data = NULL;

    xmlXPathContextPtr xpath_ctx = NULL;
    xmlXPathObjectPtr result = NULL;
    long int status = -1;
    long int *status_ptr = &status;
    char *string = NULL;


    if (!context) return IE_INVALID_CONTEXT;
    zfree(context->long_message);
    if (!document || !id || !date) return IE_INVAL;

    /* Free output arguments */
    zfree(*id);
    zfree(*date);

    /* Store configuration */
    context->type = CTX_TYPE_CZP;
    free(context->url);
    context->url = strdup("https://www.czechpoint.cz/uschovna/services.php");
    if (!(context->url))
        return IE_NOMEM;

    /* Prepare CURL handle if not yet connected */
    if (!context->curl) {
        context->curl = curl_easy_init();
        if (!(context->curl))
            return IE_ERROR;
    }

    /* Build conversion request */
    request = xmlNewNode(NULL, BAD_CAST "saveDocument");
    if (!request) {
        isds_log_message(context,
                _("Could not build Czech POINT conversion request"));
        return IE_ERROR;
    }
    deposit_ns = xmlNewNs(request, BAD_CAST DEPOSIT_NS, BAD_CAST "dep");
    if(!deposit_ns) {
        isds_log_message(context,
                _("Could not create Czech POINT deposit name space"));
        xmlFreeNode(request);
        return IE_ERROR;
    }
    xmlSetNs(request, deposit_ns);

    /* Insert childern. They are in empty namespace! */
    empty_ns = xmlNewNs(request, BAD_CAST "", NULL);
    if(!empty_ns) {
        isds_log_message(context, _("Could not create empty name space"));
        err = IE_ERROR;
        goto leave;
    } 
    INSERT_STRING_WITH_NS(request, empty_ns, "conversionID", "0");
    INSERT_STRING_WITH_NS(request, empty_ns, "fileName",
            document->dmFileDescr);

    /* Document encoded in Base64 */
    base64data = (xmlChar *) b64encode(document->data, document->data_length);
    if (!base64data) {
        isds_printf_message(context,
                ngettext("Not enought memory to encode %zd bytes into Base64",
                    "Not enought memory to encode %zd bytes into Base64",
                    document->data_length),
                document->data_length);
        err = IE_NOMEM;
        goto leave;
    }
    INSERT_STRING_WITH_NS(request, empty_ns, "document", base64data);
    zfree(base64data);

    isds_log(ILF_ISDS, ILL_DEBUG,
            _("Submitting document for conversion into Czech POINT deposit"));

    /* Send conversion request */
    err = czpdeposit(context, request, &response);
    xmlFreeNode(request); request = NULL;

    if (err) {
        czp_do_close_connection(context);
        goto leave;
    }


    /* Extract response */
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
            BAD_CAST "/deposit:saveDocumentResponse/return",
            xpath_ctx);
    if (!result) {
        err = IE_ERROR;
        goto leave;
    }
    /* Empty response */
    if (xmlXPathNodeSetIsEmpty(result->nodesetval)) {
        isds_printf_message(context,
                _("Missing `return' element in Czech POINT deposit response"));
        err = IE_ISDS;
        goto leave;
    }
    /* More responses */
    if (result->nodesetval->nodeNr > 1) {
        isds_printf_message(context,
                _("Multiple `return' element in Czech POINT deposit response"));
        err = IE_ISDS;
        goto leave;
    }
    /* One response */
    xpath_ctx->node = result->nodesetval->nodeTab[0];

    /* Get status */
    EXTRACT_LONGINT("status", status_ptr, 1);
    if (status) {
        EXTRACT_STRING("statusMsg", string);
        char *string_locale = utf82locale(string);
        isds_printf_message(context,
                _("Czech POINT deposit refused document for conversion "
                    "(code=%ld, message=%s)"),
                status, string_locale);
        free(string_locale);
        err = IE_ISDS;
        goto leave;
    }

    /* Get docuement ID */
    EXTRACT_STRING("documentID", *id);

    /* Get submit date */
    EXTRACT_STRING("dateInserted", string);
    if (string) {
        *date = calloc(1, sizeof(**date));
        if (!*date) {
            err = IE_NOMEM;
            goto leave;
        }
        err = datestring2tm((xmlChar *)string, *date);
        if (err) {
            if (err == IE_NOTSUP) {
                err = IE_ISDS;
                char *string_locale = utf82locale(string);
                isds_printf_message(context,
                        _("Invalid dateInserted value: %s"), string_locale);
                free(string_locale);
            }
            goto leave;
        }
    }

leave:
    free(string);
    xmlXPathFreeObject(result);
    xmlXPathFreeContext(xpath_ctx);

    xmlFreeDoc(response);
    free(base64data);
    xmlFreeNode(request);

    if (!err) {
        char *id_locale = utf82locale((char *) *id); 
        isds_log(ILF_ISDS, ILL_DEBUG,
                _("Document %s has been submitted for conversion "
                    "to server successfully\n"), id_locale);
        free(id_locale);
    }
    return err;
}


/* Close possibly opened connection to Czech POINT document deposit.
 * @context is Czech POINT session context. */
isds_error czp_close_connection(struct isds_ctx *context) {
    if (!context) return IE_INVALID_CONTEXT;
    zfree(context->long_message);
    return czp_do_close_connection(context);
}


/* Send request for new box creation in testing ISDS instance.
 * It's not possible to requst for a production box currently, as it
 * communicates via e-mail.
 * XXX: This function does not work either. Server complains about invalid
 * e-mail address.
 * XXX: Remove context->type hacks in isds.c and validator.c when removing
 * this function
 * @context is special session context for box creation request. DO NOT use
 * standard context as it could reveal your password. Use fresh new context or
 * context previously used by this function.
 * @box is box description to create including single primary user (in case of
 * FO box type). It outputs box ID assigned by ISDS in dbID element.
 * @users is list of struct isds_DbUserInfo (primary users in case of non-FO
 * box, or contact address of PFO box owner). The email member is mandatory as
 * it will be used to deliver credentials.
 * @former_names is optional undocumented string. Pass NULL if you don't care.
 * @approval is optional external approval of box manipulation
 * @refnumber is reallocated serial number of request assigned by ISDS. Use
 * NULL, if you don't care.*/
isds_error isds_request_new_testing_box(struct isds_ctx *context,
        struct isds_DbOwnerInfo *box, const struct isds_list *users,
        const char *former_names, const struct isds_approval *approval,
        char **refnumber) {
    isds_error err = IE_SUCCESS;
    xmlNodePtr request = NULL;
    xmlDocPtr response = NULL;
    xmlXPathContextPtr xpath_ctx = NULL;
    xmlXPathObjectPtr result = NULL;


    if (!context) return IE_INVALID_CONTEXT;
    zfree(context->long_message);
    if (!box) return IE_INVAL;

    if (!box->email || box->email[0] == '\0') {
        isds_log_message(context, _("E-mail field is mandatory"));
        return IE_INVAL;
    }

    /* Scratch box ID */
    zfree(box->dbID);

    /* Store configuration */
    context->type = CTX_TYPE_TESTING_REQUEST_COLLECTOR;
    free(context->url);
    context->url = strdup("http://78.102.19.203/testbox/request_box.php");
    if (!(context->url))
        return IE_NOMEM;

    /* Prepare CURL handle if not yet connected */
    if (!context->curl) {
        context->curl = curl_easy_init();
        if (!(context->curl))
            return IE_ERROR;
    }

    /* Build CreateDataBox request */
    err = build_CreateDBInput_request(context,
            &request, BAD_CAST "CreateDataBox",
            box, users, (xmlChar *) former_names, NULL, NULL, approval);
    if (err) goto leave;

    /* Send it to server and process response */
    err = send_destroy_request_check_response(context,
            SERVICE_DB_MANIPULATION, BAD_CAST "CreateDataBox", &request,
            &response, (xmlChar **) refnumber);

    /* Extract box ID */
    xpath_ctx = xmlXPathNewContext(response);
    if (!xpath_ctx) {
        err = IE_ERROR;
        goto leave;
    }
    if (register_namespaces(xpath_ctx, MESSAGE_NS_UNSIGNED)) {
        err = IE_ERROR;
        goto leave;
    }
    EXTRACT_STRING("/isds:CreateDataBoxResponse/dbID", box->dbID);

leave:
    xmlXPathFreeObject(result);
    xmlXPathFreeContext(xpath_ctx);
    xmlFreeDoc(response);
    xmlFreeNode(request);

    if (!err) {
        isds_log(ILF_ISDS, ILL_DEBUG,
                _("CreateDataBox request processed by server successfully.\n"));
    }

    return err;
}

#undef INSERT_ELEMENT
#undef CHECK_FOR_STRING_LENGTH
#undef INSERT_STRING_ATTRIBUTE
#undef INSERT_ULONGINTNOPTR
#undef INSERT_ULONGINT
#undef INSERT_LONGINT
#undef INSERT_BOOLEAN
#undef INSERT_SCALAR_BOOLEAN
#undef INSERT_STRING
#undef INSERT_STRING_WITH_NS
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
    const char *nsuri;
    void *xml_stream = NULL;
    size_t xml_stream_length;
    size_t phys_start, phys_end;
    char *phys_path = NULL;
    struct isds_hash *new_hash = NULL;
    

    if (!context) return IE_INVALID_CONTEXT;
    zfree(context->long_message);
    if (!message) return IE_INVAL;
   
    if (!message->raw) {
        isds_log_message(context,
                _("Message does not carry raw representation"));
        return IE_INVAL;
    }

    switch (message->raw_type) {
        case RAWTYPE_INCOMING_MESSAGE:
            nsuri = ISDS_NS;
            xml_stream = message->raw;
            xml_stream_length = message->raw_length;
            break;

        case RAWTYPE_PLAIN_SIGNED_INCOMING_MESSAGE:
            nsuri = SISDS_INCOMING_NS;
            xml_stream = message->raw;
            xml_stream_length = message->raw_length;
            break;

        case RAWTYPE_CMS_SIGNED_INCOMING_MESSAGE:
            nsuri = SISDS_INCOMING_NS;
            err = extract_cms_data(context, message->raw, message->raw_length,
                    &xml_stream, &xml_stream_length);
            if (err) goto leave;
            break;

        case RAWTYPE_PLAIN_SIGNED_OUTGOING_MESSAGE:
            nsuri = SISDS_OUTGOING_NS;
            xml_stream = message->raw;
            xml_stream_length = message->raw_length;
            break;

        case RAWTYPE_CMS_SIGNED_OUTGOING_MESSAGE:
            nsuri = SISDS_OUTGOING_NS;
            err = extract_cms_data(context, message->raw, message->raw_length,
                    &xml_stream, &xml_stream_length);
            if (err) goto leave;
            break;

        default:
            isds_log_message(context, _("Bad raw representation type"));
            return IE_INVAL;
            break;
    }


    /* XXX: Hash is computed from original string represinting isds:dmDm
     * subtree. That means no encoding, white space, xmlns attributes changes.
     * In other words, input for hash can be invalid XML stream. */
    if (-1 == isds_asprintf(&phys_path, "%s%s%s%s",
            nsuri, PHYSXML_NS_SEPARATOR "MessageDownloadResponse"
                PHYSXML_ELEMENT_SEPARATOR,
            nsuri, PHYSXML_NS_SEPARATOR "dmReturnedMessage"
                PHYSXML_ELEMENT_SEPARATOR
                ISDS_NS PHYSXML_NS_SEPARATOR "dmDm")) {
        err = IE_NOMEM;
        goto leave;
    }
    err = find_element_boundary(xml_stream, xml_stream_length,
            phys_path, &phys_start, &phys_end);
    zfree(phys_path);
    if (err) {
        isds_log_message(context,
                _("Substring with isds:dmDM element could not be located "
                    "in raw message"));
        goto leave;
    }


    /* Compute hash */
    new_hash = calloc(1, sizeof(*new_hash));
    if (!new_hash) {
        err = IE_NOMEM;
        goto leave;
    }
    new_hash->algorithm = algorithm;
    err = compute_hash(xml_stream + phys_start, phys_end - phys_start + 1,
            new_hash);
    if (err) {
        isds_log_message(context, _("Could not compute message hash"));
        goto leave;
    }

    /* Save computed hash */
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

    free(phys_path);
    if (xml_stream != message->raw) free(xml_stream);
    return err;
}


/* Compare two hashes.
 * @h1 is first hash
 * @h2 is another hash
 * @return
 *  IE_SUCCESS  if hashes equal
 *  IE_NOTUNIQ  if hashes are comparable, but they don't equal
 *  IE_ENUM     if not comparable, but both structures defined
 *  IE_INVAL    if some of the structures are undefined (NULL)
 *  IE_ERROR    if internal error occurs */
isds_error isds_hash_cmp(const struct isds_hash *h1, const struct isds_hash *h2) {
    if (h1 == NULL || h2 == NULL) return IE_INVAL;
    if (h1->algorithm != h2->algorithm) return IE_ENUM;
    if (h1->length != h2->length) return IE_ERROR;
    if (h1->length > 0 && !h1->value) return IE_ERROR;
    if (h2->length > 0 && !h2->value) return IE_ERROR;

    for (int i = 0; i < h1->length; i++) {
        if (((uint8_t *) (h1->value))[i] != ((uint8_t *) (h2->value))[i])
            return IE_NOTEQUAL;
    }
    return IE_SUCCESS;
}


/* Check message has gone through ISDS by comparing message hash stored in
 * ISDS and locally computed hash. You must provide message with valid raw
 * member (do not use isds_load_message(..., BUFFER_DONT_STORE)).
 * This is convenient wrapper for isds_download_message_hash(),
 * isds_compute_message_hash(), and isds_hash_cmp() sequence.
 * @context is session context
 * @message is message with valid raw and envelope member; envelope->hash
 * member will be changed during funcion run. Use envelope on heap only.
 * @return
 *  IE_SUCCESS  if message originates in ISDS
 *  IE_NOTEQUAL if message is unknown to ISDS
 *  other code  for other errors */
isds_error isds_verify_message_hash(struct isds_ctx *context,
        struct isds_message *message) {
    isds_error err = IE_SUCCESS;
    struct isds_hash *downloaded_hash = NULL;

    if (!context) return IE_INVALID_CONTEXT;
    zfree(context->long_message);
    if (!message) return IE_INVAL;

    if (!message->envelope) {
        isds_log_message(context,
                _("Given message structure is missing envelope"));
        return IE_INVAL;
    }
    if (!message->raw) {
        isds_log_message(context,
                _("Given message structure is missing raw representation"));
        return IE_INVAL;
    }

    err = isds_download_message_hash(context, message->envelope->dmID,
            &downloaded_hash);
    if (err) goto leave;

    err = isds_compute_message_hash(context, message,
            downloaded_hash->algorithm);
    if (err) goto leave;

    err = isds_hash_cmp(downloaded_hash, message->envelope->hash);

leave:
    isds_hash_free(&downloaded_hash);
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
        case MESSAGE_NS_1:
            message_namespace = BAD_CAST ISDS1_NS; break;
        case MESSAGE_NS_UNSIGNED:
            message_namespace = BAD_CAST ISDS_NS; break;
        case MESSAGE_NS_SIGNED_INCOMING:
            message_namespace = BAD_CAST SISDS_INCOMING_NS; break;
        case MESSAGE_NS_SIGNED_OUTGOING:
            message_namespace = BAD_CAST SISDS_OUTGOING_NS; break;
        case MESSAGE_NS_SIGNED_DELIVERY:
            message_namespace = BAD_CAST SISDS_DELIVERY_NS; break;
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
    if (xmlXPathRegisterNs(xpath_ctx, BAD_CAST "deposit", BAD_CAST DEPOSIT_NS))
        return IE_ERROR;
    return IE_SUCCESS;
}

