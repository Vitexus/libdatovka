#include "isds_priv.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdarg.h>
#include <ctype.h>
#include <stdint.h>     /* For uint8_t and intmax_t */
#include <limits.h>     /* Because of LONG_{MIN,MAX} constants */
#include <inttypes.h>   /* For PRIdMAX formatting macro */
#include "utils.h"
#if HAVE_LIBCURL
    #include "soap.h"
#endif
#include "validator.h"
#include "crypto.h"
#include "physxml.h"
#include "system.h"

/* Global variables.
 * Allocated in isds_init() and deallocated in isds_cleanup(). */
unsigned int log_facilities;
isds_log_level log_level;
isds_log_callback log_callback;
void *log_callback_data;
const char *version_gpgme = N_("n/a");
const char *version_gcrypt = N_("n/a");
const char *version_openssl = N_("n/a");
const char *version_expat = N_("n/a");

/* Locators */
/* Base URL of production ISDS instance */
const char isds_locator[] = "https://ws1.mojedatovaschranka.cz/";
const char isds_cert_locator[] = "https://ws1c.mojedatovaschranka.cz/";
const char isds_otp_locator[] = "https://www.mojedatovaschranka.cz/";
const char isds_mep_locator[] = "https://www.mojedatovaschranka.cz/";

/* Base URL of production ISDS instance */
const char isds_testing_locator[] = "https://ws1.czebox.cz/";
const char isds_cert_testing_locator[] = "https://ws1c.czebox.cz/";
const char isds_otp_testing_locator[] = "https://www.czebox.cz/";
const char isds_mep_testing_locator[] = "https://www.czebox.cz/";

/* Extension to MIME type map */
static const xmlChar *extension_map_mime[] = {
    BAD_CAST "cer", BAD_CAST "application/x-x509-ca-cert",
    BAD_CAST "crt", BAD_CAST "application/x-x509-ca-cert",
    BAD_CAST "der", BAD_CAST "application/x-x509-ca-cert",
    BAD_CAST "doc", BAD_CAST "application/msword",
    BAD_CAST "docx", BAD_CAST "application/vnd.openxmlformats-officedocument."
        "wordprocessingml.document",
    BAD_CAST "dbf", BAD_CAST "application/octet-stream",
    BAD_CAST "prj", BAD_CAST "application/octet-stream",
    BAD_CAST "qix", BAD_CAST "application/octet-stream",
    BAD_CAST "sbn", BAD_CAST "application/octet-stream",
    BAD_CAST "sbx", BAD_CAST "application/octet-stream",
    BAD_CAST "shp", BAD_CAST "application/octet-stream",
    BAD_CAST "shx", BAD_CAST "application/octet-stream",
    BAD_CAST "dgn", BAD_CAST "application/octet-stream",
    BAD_CAST "dwg", BAD_CAST "image/vnd.dwg",
    BAD_CAST "edi", BAD_CAST "application/edifact",
    BAD_CAST "fo", BAD_CAST "application/vnd.software602.filler.form+xml",
    BAD_CAST "gfs", BAD_CAST "application/xml",
    BAD_CAST "gml", BAD_CAST "application/xml",
    BAD_CAST "gif", BAD_CAST "image/gif",
    BAD_CAST "htm", BAD_CAST "text/html",
    BAD_CAST "html", BAD_CAST "text/html",
    BAD_CAST "isdoc", BAD_CAST "text/isdoc",
    BAD_CAST "isdocx", BAD_CAST "text/isdocx",
    BAD_CAST "jfif", BAD_CAST "image/jpeg",
    BAD_CAST "jpg", BAD_CAST "image/jpeg",
    BAD_CAST "jpeg", BAD_CAST "image/jpeg",
    BAD_CAST "mpeg", BAD_CAST "video/mpeg",
    BAD_CAST "mpeg1", BAD_CAST "video/mpeg",
    BAD_CAST "mpeg2", BAD_CAST "video/mpeg",
    BAD_CAST "mpg", BAD_CAST "video/mpeg",
    BAD_CAST "mp2", BAD_CAST "audio/mpeg",
    BAD_CAST "mp3", BAD_CAST "audio/mpeg",
    BAD_CAST "odp", BAD_CAST "application/vnd.oasis.opendocument.presentation",
    BAD_CAST "ods", BAD_CAST "application/vnd.oasis.opendocument.spreadsheet",
    BAD_CAST "odt", BAD_CAST "application/vnd.oasis.opendocument.text",
    BAD_CAST "pdf", BAD_CAST "application/pdf",
    BAD_CAST "p7b", BAD_CAST "application/pkcs7-certificates",
    BAD_CAST "p7c", BAD_CAST "application/pkcs7-mime",
    BAD_CAST "p7m", BAD_CAST "application/pkcs7-mime",
    BAD_CAST "p7f", BAD_CAST "application/pkcs7-signature",
    BAD_CAST "p7s", BAD_CAST "application/pkcs7-signature",
    BAD_CAST "pk7", BAD_CAST "application/pkcs7-mime",
    BAD_CAST "png", BAD_CAST "image/png",
    BAD_CAST "ppt", BAD_CAST "application/vnd.ms-powerpoint",
    BAD_CAST "pptx", BAD_CAST "application/vnd.openxmlformats-officedocument."
        "presentationml.presentation",
    BAD_CAST "rtf", BAD_CAST "application/rtf",
    BAD_CAST "tif", BAD_CAST "image/tiff",
    BAD_CAST "tiff", BAD_CAST "image/tiff",
    BAD_CAST "tsr", BAD_CAST "application/timestamp-reply",
    BAD_CAST "tst", BAD_CAST "application/timestamp-reply",
    BAD_CAST "txt", BAD_CAST "text/plain",
    BAD_CAST "wav", BAD_CAST "audio/wav",
    BAD_CAST "xls", BAD_CAST "application/vnd.ms-excel",
    BAD_CAST "xlsx", BAD_CAST "application/vnd.openxmlformats-officedocument."
        "spreadsheetml.sheet",
    BAD_CAST "xml", BAD_CAST "application/xml",
    BAD_CAST "xsd", BAD_CAST "application/xml",
    BAD_CAST "zfo", BAD_CAST "application/vnd.software602.filler.form-xml-zip"
};

/* Structure type to hold conversion table from status code to isds_error and
 * long message */
struct code_map_isds_error {
    const xmlChar **codes;     /* NULL terminated array of status codes */
    const char **meanings;     /* Mapping to non-localized long messages */
    const isds_error *errors;  /* Mapping to isds_error code */
};

/* Deallocate structure isds_pki_credentials and NULL it.
 * Pass-phrase is discarded.
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
        if (item->destructor) (item->destructor)(&(item->data));
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
void isds_PersonName_free(struct isds_PersonName **person_name) {
    if (!person_name || !*person_name) return;

    free((*person_name)->pnFirstName);
    free((*person_name)->pnMiddleName);
    free((*person_name)->pnLastName);
    free((*person_name)->pnLastNameAtBirth);
   
    free(*person_name);
    *person_name = NULL;
}


/* Deallocate structure isds_BirthInfo recursively and NULL it */
void isds_BirthInfo_free(struct isds_BirthInfo **birth_info) {
    if (!birth_info || !*birth_info) return;

    free((*birth_info)->biDate);
    free((*birth_info)->biCity);
    free((*birth_info)->biCounty);
    free((*birth_info)->biState);
    
    free(*birth_info);
    *birth_info = NULL;
}


/* Deallocate structure isds_Address recursively and NULL it */
void isds_Address_free(struct isds_Address **address) {
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
    free((*db_owner_info)->dbOpenAddressing);
    
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
    free((*db_user_info)->caState);
    
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

    free((*envelope)->dmOrdinal);
    free((*envelope)->dmMessageStatus);
    free((*envelope)->dmAttachmentSize);
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
    free((*envelope)->dmType);

    free((*envelope)->dmOVM);
    free((*envelope)->dmPublishOwnID);

    free(*envelope);
    *envelope = NULL;
}


/* Deallocate struct isds_message recursively and NULL it */
void isds_message_free(struct isds_message **message) {
    if (!message || !*message) return;

    free((*message)->raw);
    isds_envelope_free(&((*message)->envelope));
    isds_list_free(&((*message)->documents));
    xmlFreeDoc((*message)->xml); (*message)->xml = NULL;

    free(*message);
    *message = NULL;
}


/* Deallocate struct isds_document recursively and NULL it */
void isds_document_free(struct isds_document **document) {
    if (!document || !*document) return;
    
    if (!(*document)->is_xml) {
        free((*document)->data);
    }
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


/* Deallocate struct isds_message_status_change recursively and NULL it */
void isds_message_status_change_free(
        struct isds_message_status_change **message_status_change) {
    if (!message_status_change || !*message_status_change) return;

    free((*message_status_change)->dmID);
    free((*message_status_change)->time);
    free((*message_status_change)->dmMessageStatus);

    zfree(*message_status_change);
}


/* Deallocate struct isds_approval recursively and NULL it */
void isds_approval_free(struct isds_approval **approval) {
    if (!approval || !*approval) return;

    free((*approval)->refference);

    zfree(*approval);
}


/* Deallocate struct isds_credentials_delivery recursively and NULL it.
 * The email string is deallocated too. */
void isds_credentials_delivery_free(
        struct isds_credentials_delivery **credentials_delivery) {
    if (!credentials_delivery || !*credentials_delivery) return;

    free((*credentials_delivery)->email);
    free((*credentials_delivery)->token);
    free((*credentials_delivery)->new_user_name);

    zfree(*credentials_delivery);
}


/* Deallocate struct isds_commercial_permission recursively and NULL it */
void isds_commercial_permission_free(
        struct isds_commercial_permission **permission) {
    if (NULL == permission || NULL == *permission) return;

    free((*permission)->recipient);
    free((*permission)->payer);
    free((*permission)->expiration);
    free((*permission)->count);
    free((*permission)->reply_identifier);

    zfree(*permission);
}


/* Deallocate struct isds_credit_event recursively and NULL it */
void isds_credit_event_free(struct isds_credit_event **event) {
    if (NULL == event || NULL == *event) return;

    free((*event)->time);
    switch ((*event)->type) {
        case ISDS_CREDIT_CHARGED:
            free((*event)->details.charged.transaction);
            break;
        case ISDS_CREDIT_DISCHARGED:
            free((*event)->details.discharged.transaction);
            break;
        case ISDS_CREDIT_MESSAGE_SENT:
            free((*event)->details.message_sent.recipient);
            free((*event)->details.message_sent.message_id);
            break;
        case ISDS_CREDIT_STORAGE_SET:
            free((*event)->details.storage_set.new_valid_from);
            free((*event)->details.storage_set.new_valid_to);
            free((*event)->details.storage_set.old_capacity);
            free((*event)->details.storage_set.old_valid_from);
            free((*event)->details.storage_set.old_valid_to);
            free((*event)->details.storage_set.initiator);
            break;
        case ISDS_CREDIT_EXPIRED:
            break;
    }

    zfree(*event);
}


/* Deallocate struct isds_fulltext_result recursively and NULL it */
void isds_fulltext_result_free(
        struct isds_fulltext_result **result) {
    if (NULL == result || NULL == *result) return;

    free((*result)->dbID);
    free((*result)->name);
    isds_list_free(&((*result)->name_match_start));
    isds_list_free(&((*result)->name_match_end));
    free((*result)->address);
    isds_list_free(&((*result)->address_match_start));
    isds_list_free(&((*result)->address_match_end));
    free((*result)->ic);
    free((*result)->biDate);

    zfree(*result);
}


/* Deallocate struct isds_box_state_period recursively and NULL it */
void isds_box_state_period_free(struct isds_box_state_period **period) {
    if (NULL == period || NULL == *period) return;
    zfree(*period);
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
        const struct isds_PersonName *src) {
    struct isds_PersonName *new = NULL;

    if (!src) return NULL;

    new = calloc(1, sizeof(*new));
    if (!new) return NULL;

    STRDUP_OR_ERROR(new->pnFirstName, src->pnFirstName);
    STRDUP_OR_ERROR(new->pnMiddleName, src->pnMiddleName);
    STRDUP_OR_ERROR(new->pnLastName, src->pnLastName);
    STRDUP_OR_ERROR(new->pnLastNameAtBirth, src->pnLastNameAtBirth);

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
        const struct isds_Address *src) {
    struct isds_Address *new = NULL;

    if (!src) return NULL;

    new = calloc(1, sizeof(*new));
    if (!new) return NULL;

    STRDUP_OR_ERROR(new->adCity, src->adCity);
    STRDUP_OR_ERROR(new->adStreet, src->adStreet);
    STRDUP_OR_ERROR(new->adNumberInStreet, src->adNumberInStreet);
    STRDUP_OR_ERROR(new->adNumberInMunicipality,
            src->adNumberInMunicipality);
    STRDUP_OR_ERROR(new->adZipCode, src->adZipCode);
    STRDUP_OR_ERROR(new->adState, src->adState);
    
    return new;
    
error:
    isds_Address_free(&new);
    return NULL;
}


/* Copy structure isds_DbOwnerInfo recursively */
struct isds_DbOwnerInfo *isds_DbOwnerInfo_duplicate(
        const struct isds_DbOwnerInfo *src) {
    struct isds_DbOwnerInfo *new = NULL;
    if (!src) return NULL;

    new = calloc(1, sizeof(*new));
    if (!new) return NULL;

    STRDUP_OR_ERROR(new->dbID, src->dbID);
    FLATDUP_OR_ERROR(new->dbType, src->dbType);
    STRDUP_OR_ERROR(new->ic, src->ic);

    if (src->personName) {
        if (!(new->personName =
                    isds_PersonName_duplicate(src->personName)))
            goto error;
    }

    STRDUP_OR_ERROR(new->firmName, src->firmName);

    if (src->birthInfo) {
        if (!(new->birthInfo =
                    isds_BirthInfo_duplicate(src->birthInfo)))
            goto error;
    }

    if (src->address) {
        if (!(new->address = isds_Address_duplicate(src->address)))
            goto error;
    }
    
    STRDUP_OR_ERROR(new->nationality, src->nationality);
    STRDUP_OR_ERROR(new->email, src->email);
    STRDUP_OR_ERROR(new->telNumber, src->telNumber);
    STRDUP_OR_ERROR(new->identifier, src->identifier);
    STRDUP_OR_ERROR(new->registryCode, src->registryCode);
    FLATDUP_OR_ERROR(new->dbState, src->dbState);
    FLATDUP_OR_ERROR(new->dbEffectiveOVM, src->dbEffectiveOVM);
    FLATDUP_OR_ERROR(new->dbOpenAddressing, src->dbOpenAddressing);

    return new;
    
error:
    isds_DbOwnerInfo_free(&new);
    return NULL;
}


/* Copy structure isds_DbUserInfo recursively */
struct isds_DbUserInfo *isds_DbUserInfo_duplicate(
        const struct isds_DbUserInfo *src) {
    struct isds_DbUserInfo *new = NULL;
    if (!src) return NULL;

    new = calloc(1, sizeof(*new));
    if (!new) return NULL;

    STRDUP_OR_ERROR(new->userID, src->userID);
    FLATDUP_OR_ERROR(new->userType, src->userType);
    FLATDUP_OR_ERROR(new->userPrivils, src->userPrivils);

    if (src->personName) {
        if (!(new->personName =
                    isds_PersonName_duplicate(src->personName)))
            goto error;
    }

    if (src->address) {
        if (!(new->address = isds_Address_duplicate(src->address)))
            goto error;
    }
    
    FLATDUP_OR_ERROR(new->biDate, src->biDate);
    STRDUP_OR_ERROR(new->ic, src->ic);
    STRDUP_OR_ERROR(new->firmName, src->firmName);
    STRDUP_OR_ERROR(new->caStreet, src->caStreet);
    STRDUP_OR_ERROR(new->caCity, src->caCity);
    STRDUP_OR_ERROR(new->caZipCode, src->caZipCode);
    STRDUP_OR_ERROR(new->caState, src->caState);

    return new;
    
error:
    isds_DbUserInfo_free(&new);
    return NULL;
}


/* Copy structure isds_box_state_period recursively */
struct isds_box_state_period *isds_box_state_period_duplicate(
        const struct isds_box_state_period *src) {
    struct isds_box_state_period *new = NULL;
    if (!src) return NULL;

    new = calloc(1, sizeof(*new));
    if (!new) return NULL;

    memcpy(&new->from, &src->from, sizeof(src->from));
    memcpy(&new->to, &src->to, sizeof(src->to));
    new->dbState = src->dbState;

    return new;
}

#undef FLATDUP_OR_ERROR
#undef STRDUP_OR_ERROR 


/* Logs libxml2 errors. Should be registered to libxml2 library.
 * @ctx is unused currently
 * @msg is printf-like formated message from libxml2 (UTF-8?)
 * @... are variadic arguments for @msg */
static void log_xml(void *ctx, const char *msg, ...) {
    va_list ap;
    char *text = NULL;

    /* Silent warning for unused function argument.
     * The prototype is an API of libxml2's xmlSetGenericErrorFunc(). */
    (void)ctx;

    if (!msg) return;

    va_start(ap, msg);
    isds_vasprintf(&text, msg, ap);
    va_end(ap);

    if (text)
        isds_log(ILF_XML, ILL_ERR, "%s", text);
    free(text);
}


/* Initialize ISDS library.
 * Global function, must be called before other functions.
 * If it fails you can not use ISDS library and must call isds_cleanup() to
 * free partially initialized global variables. */
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

#if HAVE_LIBCURL
    /* Initialize CURL */
    if (curl_global_init(CURL_GLOBAL_ALL)) {
        isds_log(ILF_ISDS, ILL_CRIT, _("CURL library initialization failed\n"));
        return IE_ERROR;
    }
#endif /* HAVE_LIBCURL */

    /* Initialise cryptographic back-ends. */
    if (IE_SUCCESS != _isds_init_crypto()) {
        isds_log(ILF_ISDS, ILL_CRIT,
                _("Initialization of cryptographic back-end failed\n"));
        return IE_ERROR;
    }

    /* This can _exit() current program. Find not so assertive check. */
    LIBXML_TEST_VERSION;
    xmlSetGenericErrorFunc(NULL, log_xml);

    /* Check expat */
    if (_isds_init_expat(&version_expat)) {
        isds_log(ILF_ISDS, ILL_CRIT,
                _("expat library initialization failed\n"));
        return IE_ERROR;
    }

    /* Allocate global variables */


    return IE_SUCCESS;
}


/* Deinitialize ISDS library.
 * Global function, must be called as last library function. */
isds_error isds_cleanup(void) {
    /* XML */
    xmlCleanupParser();
    
#if HAVE_LIBCURL
    /* Curl */
    curl_global_cleanup();
#endif

    return IE_SUCCESS;
}


/* Return version string of this library. Version of dependencies can be
 * embedded. Do no try to parse it. You must free it. */
char *isds_version(void) {
    char *buffer = NULL;

    isds_asprintf(&buffer,
#if HAVE_LIBCURL
#  ifndef USE_OPENSSL_BACKEND
            _("%s (%s, GPGME %s, gcrypt %s, %s, libxml2 %s)"),
#  else
            _("%s (%s, %s, %s, libxml2 %s)"),
#  endif
#else
#  ifndef USE_OPENSSL_BACKEND
            _("%s (GPGME %s, gcrypt %s, %s, libxml2 %s)"),
#  else
            _("%s (%s, %s, libxml2 %s)"),
#  endif
#endif
            PACKAGE_VERSION,
#if HAVE_LIBCURL
            curl_version(),
#endif
#ifndef USE_OPENSSL_BACKEND
            version_gpgme, version_gcrypt,
#else
            version_openssl,
#endif
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
            return(_("Values not equal")); break;
        case IE_PARTIAL_SUCCESS:
            return(_("Some suboperations failed")); break;
        case IE_ABORTED:
            return(_("Operation aborted")); break;
        case IE_SECURITY:
            return(_("Security problem")); break;
        default:
            return(_("Unknown error"));
    }
}


/* Create ISDS context.
 * Each context can be used for different sessions to (possibly) different
 * ISDS server with different credentials. */
struct isds_ctx *isds_ctx_create(void) {
    struct isds_ctx *context;
    context = malloc(sizeof(*context));
    if (context) memset(context, 0, sizeof(*context));
    return context;
};

#if HAVE_LIBCURL
/* Close possibly opened connection to Czech POINT document deposit without
 * resetting long_message buffer.
 * XXX: Do not use czp_close_connection() if you do not want to destroy log
 * message.
 * @context is Czech POINT session context. */
static isds_error czp_do_close_connection(struct isds_ctx *context) {
    if (!context) return IE_INVALID_CONTEXT;
    _isds_close_connection(context);
    return IE_SUCCESS;
}


/* Discard credentials.
 * @context is ISDS context
 * @discard_saved_username is true for removing saved username, false for
 * keeping it.
 * Only that. It does not cause log out, connection close or similar. */
_hidden isds_error _isds_discard_credentials(struct isds_ctx *context,
        _Bool discard_saved_username) {
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
    if (discard_saved_username && context->saved_username) {
        memset(context->saved_username, 0, strlen(context->saved_username));
        zfree(context->saved_username);
    }

    return IE_SUCCESS;
}
#endif /* HAVE_LIBCURL */


/* Destroy ISDS context and free memory.
 * @context will be NULLed on success. */
isds_error isds_ctx_free(struct isds_ctx **context) {
    if (!context || !*context) {
        return IE_INVALID_CONTEXT;
    }
  
#if HAVE_LIBCURL
    /* Discard credentials and close connection */
    switch ((*context)->type) {
        case CTX_TYPE_NONE: break;
        case CTX_TYPE_ISDS: isds_logout(*context); break;
        case CTX_TYPE_CZP:
        case CTX_TYPE_TESTING_REQUEST_COLLECTOR:
                            czp_do_close_connection(*context); break;
    }

    /* For sure */
    _isds_discard_credentials(*context, 1);

    /* Free other structures */
    free((*context)->url);
    free((*context)->tls_verify_server);
    free((*context)->tls_ca_file);
    free((*context)->tls_ca_dir);
    free((*context)->tls_crl_file);
#endif /* HAVE_LIBCURL */
    free((*context)->long_message);

    free(*context);
    *context = NULL;
    return IE_SUCCESS;
}


/* Return long message text produced by library function, e.g. detailed error
 * message. Returned pointer is only valid until new library function is
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


/* Stores formatted message into context' long_message buffer.
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
 * @facilities is bit mask of isds_log_facility values,
 * @level is verbosity level. */
void isds_set_logging(const unsigned int facilities,
        const isds_log_level level) {
    log_facilities = facilities;
    log_level = level;
}


/* Register callback function libisds calls when new global log message is
 * produced by library. Library logs to stderr by default.
 * @callback is function provided by application libisds will call. See type
 * definition for @callback argument explanation. Pass NULL to revert logging to
 * default behaviour.
 * @data is application specific data @callback gets as last argument */
void isds_set_log_callback(isds_log_callback callback, void *data) {
    log_callback = callback;
    log_callback_data = data;
}


/* Log @message in class @facility with log @level into global log. @message
 * is printf(3) formatting string, variadic arguments may be necessary.
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


/* Set timeout in milliseconds for each network job like connecting to server
 * or sending message. Use 0 to disable timeout limits. */
isds_error isds_set_timeout(struct isds_ctx *context,
        const unsigned int timeout) {
    if (!context) return IE_INVALID_CONTEXT;
    zfree(context->long_message);

#if HAVE_LIBCURL
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
#else /* not HAVE_LIBCURL */
    return IE_NOTSUP;
#endif
}


/* Register callback function libisds calls periodically during HTTP data
 * transfer.
 * @context is session context
 * @callback is function provided by application libisds will call. See type
 * definition for @callback argument explanation.
 * @data is application specific data @callback gets as last argument */
isds_error isds_set_progress_callback(struct isds_ctx *context,
        isds_progress_callback callback, void *data) {
    if (!context) return IE_INVALID_CONTEXT;
    zfree(context->long_message);

#if HAVE_LIBCURL
    context->progress_callback = callback;
    context->progress_callback_data = data;

    return IE_SUCCESS;
#else /* not HAVE_LIBCURL */
    return IE_NOTSUP;
#endif
}


/* Change context settings.
 * @context is context which setting will be applied to
 * @option is name of option. It determines the type of last argument. See
 * isds_option definition for more info.
 * @... is value of new setting. Type is determined by @option
 * */
isds_error isds_set_opt(struct isds_ctx *context, const isds_option option,
        ...) {
    isds_error err = IE_SUCCESS;
    va_list ap;
#if HAVE_LIBCURL
    char *pointer, *string;
#endif

    if (!context) return IE_INVALID_CONTEXT;
    zfree(context->long_message);

    va_start(ap, option);

#define REPLACE_VA_BOOLEAN(destination) { \
    if (!(destination)) { \
        (destination) = malloc(sizeof(*(destination))); \
        if (!(destination)) { \
            err = IE_NOMEM; goto leave; \
        } \
    } \
    *(destination) = (_Bool) !!va_arg(ap, int); \
}

#define REPLACE_VA_STRING(destination) { \
    string = va_arg(ap, char *); \
    if (string) { \
        pointer = realloc((destination), 1 + strlen(string)); \
        if (!pointer) { err = IE_NOMEM; goto leave; } \
        strcpy(pointer, string); \
        (destination) = pointer; \
    } else { \
        free(destination); \
        (destination) = NULL; \
    } \
}

    switch (option) {
        case IOPT_TLS_VERIFY_SERVER:
#if HAVE_LIBCURL
            REPLACE_VA_BOOLEAN(context->tls_verify_server);
#else
            err = IE_NOTSUP; goto leave;
#endif
            break;
        case IOPT_TLS_CA_FILE:
#if HAVE_LIBCURL
            REPLACE_VA_STRING(context->tls_ca_file);
#else
            err = IE_NOTSUP; goto leave;
#endif
            break;
        case IOPT_TLS_CA_DIRECTORY:
#if HAVE_LIBCURL
            REPLACE_VA_STRING(context->tls_ca_dir);
#else
            err = IE_NOTSUP; goto leave;
#endif
            break;
        case IOPT_TLS_CRL_FILE:
#if HAVE_LIBCURL
#if HAVE_DECL_CURLOPT_CRLFILE /* Since curl-7.19.0 */
            REPLACE_VA_STRING(context->tls_crl_file);
#else
            isds_log_message(context,
                    _("Curl library does not support CRL definition"));
            err = IE_NOTSUP;
#endif  /* not HAVE_DECL_CURLOPT_CRLFILE */
#else
            err = IE_NOTSUP; goto leave;
#endif  /* not HAVE_LIBCURL */
            break;
        case IOPT_NORMALIZE_MIME_TYPE:
            context->normalize_mime_type = (_Bool) !!va_arg(ap, int);
            break;

        default:
            err = IE_ENUM; goto leave;
    }

#undef REPLACE_VA_STRING
#undef REPLACE_VA_BOOLEAN

leave:
    va_end(ap);
    return err;
}


#if HAVE_LIBCURL
/* Copy credentials into context. Any non-NULL argument will be duplicated.
 * Destination for NULL argument will not be touched.
 * Destination pointers must be freed before calling this function.
 * If @username is @context->saved_username, the saved_username will not be
 * replaced. The saved_username is clobbered only if context has set otp
 * member.
 * Return IE_SUCCESS on success. */
static isds_error _isds_store_credentials(struct isds_ctx *context,
        const char *username, const char *password,
        const struct isds_pki_credentials *pki_credentials) {
    if (NULL == context) return IE_INVALID_CONTEXT;

    /* FIXME: mlock password
     * (I have a library) */

    if (username) {
        context->username = strdup(username);
        if (context->otp && context->saved_username != username)
            context->saved_username = strdup(username);
    }
    if (password) {
        if (NULL == context->otp_credentials)
            context->password = strdup(password);
        else
            context->password = _isds_astrcat(password,
                    context->otp_credentials->otp_code);
    }
    context->pki_credentials = isds_pki_credentials_duplicate(pki_credentials);

    if ((NULL != username && NULL == context->username) ||
            (NULL != password && NULL == context->password) ||
            (NULL != pki_credentials && NULL == context->pki_credentials) ||
            (context->otp && NULL != context->username &&
             NULL == context->saved_username)) {
        return IE_NOMEM;
    }

    return IE_SUCCESS;
}
#endif


/* Connect and log into ISDS server.
 * All required arguments will be copied, you do not have to keep them after
 * that.
 * ISDS supports six different authentication methods. Exact method is
 * selected on @username, @password, @pki_credentials, and @otp arguments:
 *   - If @pki_credentials == NULL, @username and @password must be supplied
 *     and then
 *      - If @otp == NULL, simple authentication by username and password will
 *        be proceeded.
 *      - If @otp != NULL, authentication by username and password and OTP
 *        will be used.
 *   - If @pki_credentials != NULL, then
 *      - If @username == NULL, only certificate will be used
 *      - If @username != NULL, then
 *          - If @password == NULL, then certificate will be used and
 *            @username shifts meaning to box ID. This is used for hosted
 *            services.
 *          - Otherwise all three arguments will be used.
 *      Please note, that different cases require different certificate type
 *      (system qualified one or commercial non qualified one). This library
 *      does not check such political issues. Please see ISDS Specification
 *      for more details.
 * @url is base address of ISDS web service. Pass extern isds_locator
 * variable to use production ISDS instance without client certificate
 * authentication (or extern isds_cert_locator with client certificate
 * authentication or extern isds_otp_locators with OTP authentication).
 * Passing NULL has the same effect, autoselection between isds_locator,
 * isds_cert_locator, and isds_otp_locator is performed in addition. You can
 * pass extern isds_testing_locator (or isds_cert_testing_locator or
 * isds_otp_testing_locator) variable to select testing instance. 
 * @username is user name of ISDS user or box ID
 * @password is user's secret password
 * @pki_credentials defines public key cryptographic material to use in client
 * authentication.
 * @otp selects one-time password authentication method to use, defines OTP
 * code (if known) and returns fine grade resolution of OTP procedure.
 * @return:
 *  IE_SUCCESS if authentication succeeds
 *  IE_NOT_LOGGED_IN if authentication fails. If OTP authentication has been
 *  requested, fine grade reason will be set into @otp->resolution. Error
 *  message from server can be obtained by isds_long_message() call. 
 *  IE_PARTIAL_SUCCESS if time-based OTP authentication has been requested and
 *  server has sent OTP code through side channel. Application is expected to
 *  fill the code into @otp->otp_code, keep other arguments unchanged, and retry
 *  this call to complete second phase of TOTP authentication;
 *  or other appropriate error. */
isds_error isds_login(struct isds_ctx *context, const char *url,
        const char *username, const char *password,
        const struct isds_pki_credentials *pki_credentials,
        struct isds_otp *otp) {
#if HAVE_LIBCURL
    isds_error err = IE_NOT_LOGGED_IN;
    isds_error soap_err;
    xmlNsPtr isds_ns = NULL;
    xmlNodePtr request = NULL;
#endif /* HAVE_LIBCURL */

    if (!context) return IE_INVALID_CONTEXT;
    zfree(context->long_message);

#if HAVE_LIBCURL
    /* Close connection if already logged in */
    if (context->curl) {
        _isds_close_connection(context);
    }

    /* Store configuration */
    context->type = CTX_TYPE_ISDS;
    zfree(context->url);

    /* Mangle base URI according to requested authentication method */
    if (NULL == pki_credentials) {
        isds_log(ILF_SEC, ILL_INFO,
                _("Selected authentication method: no certificate, "
                    "username and password\n"));
        if (!username || !password) {
            isds_log_message(context,
                    _("Both username and password must be supplied"));
            return IE_INVAL;
        }
        context->otp_credentials = otp;
        context->otp = (NULL != context->otp_credentials);
        
        if (!context->otp) {
            /* Default locator is official system (without certificate or
             * OTP) */
            context->url = strdup((NULL != url) ? url : isds_locator);
        } else {
            const char *authenticator_uri = NULL;
            if (!url) url = isds_otp_locator;
            otp->resolution = OTP_RESOLUTION_UNKNOWN;
            switch (context->otp_credentials->method) {
                case OTP_HMAC: 
                    isds_log(ILF_SEC, ILL_INFO,
                            _("Selected authentication method: "
                                "HMAC-based one-time password\n"));
                    authenticator_uri =
                        "%sas/processLogin?type=hotp&uri=%sapps/";
                    break;
                case OTP_TIME: 
                    isds_log(ILF_SEC, ILL_INFO,
                            _("Selected authentication method: "
                                "Time-based one-time password\n"));
                    if (context->otp_credentials->otp_code == NULL) {
                        isds_log(ILF_SEC, ILL_INFO,
                                _("OTP code has not been provided by "
                                    "application, requesting server for "
                                    "new one.\n"));
                        authenticator_uri =
                            "%sas/processLogin?type=totp&sendSms=true&"
                            "uri=%sapps/";
                    } else {
                        isds_log(ILF_SEC, ILL_INFO,
                                _("OTP code has been provided by "
                                    "application, not requesting server "
                                    "for new one.\n"));
                        authenticator_uri =
                            "%sas/processLogin?type=totp&"
                            "uri=%sapps/";
                    }
                    break;
                default:
                    isds_log_message(context,
                            _("Unknown one-time password authentication "
                                "method requested by application"));
                    return IE_ENUM;
            }
            if (-1 == isds_asprintf(&context->url, authenticator_uri, url, url))
                return IE_NOMEM; 
        }
    } else {
        /* Default locator is official system (with client certificate) */
        context->otp = 0;
        context->otp_credentials = NULL;
        if (!url) url = isds_cert_locator;

        if (!username) {
            isds_log(ILF_SEC, ILL_INFO,
                    _("Selected authentication method: system certificate, "
                        "no username and no password\n"));
            password = NULL;
            context->url = _isds_astrcat(url, "cert/");
        } else {
            if (!password) {
                isds_log(ILF_SEC, ILL_INFO,
                        _("Selected authentication method: system certificate, "
                            "box ID and no password\n"));
                context->url = _isds_astrcat(url, "hspis/");
            } else {
                isds_log(ILF_SEC, ILL_INFO,
                        _("Selected authentication method: commercial "
                            "certificate, username and password\n"));
                context->url = _isds_astrcat(url, "certds/");
            }
        }
    }
    if (!(context->url))
        return IE_NOMEM;

    /* Prepare CURL handle */
    context->curl = curl_easy_init();
    if (!(context->curl))
        return IE_ERROR;

    /* Build log-in request */
    request = xmlNewNode(NULL, BAD_CAST "DummyOperation");
    if (!request) {
        isds_log_message(context, _("Could not build ISDS log-in request"));
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
    _isds_discard_credentials(context, 1);
    if (_isds_store_credentials(context, username, password, pki_credentials)) {
        _isds_discard_credentials(context, 1);
        xmlFreeNode(request);
        return IE_NOMEM;
    }

    isds_log(ILF_ISDS, ILL_DEBUG, _("Logging user %s into server %s\n"),
            username, url);

    /* XXX: ISDS documentation does not specify response body for
     * DummyOperation request.  However real server sends back
     * DummyOperationResponse.  Therefore we cannot check for the SOAP body
     * content and we call _isds_soap() instead of _isds().  _isds() checks for
     * SOAP body content, e.g. the dmStatus element. */

    /* Send log-in request */
    soap_err = _isds_soap(context, "DS/dz", request, NULL, NULL, NULL, NULL);
   
    if (context->otp) {
        /* Revert context URL from OTP authentication service URL to OTP web
         * service base URL for subsequent calls. Potential isds_login() retry
         * will re-set context URL again. */
        zfree(context->url);
        context->url = _isds_astrcat(url, "apps/");
        if (context->url == NULL) {
            soap_err = IE_NOMEM;
        }
        /* Detach pointer to OTP credentials from context */
        context->otp_credentials = NULL;
    }

    /* Remove credentials */
    _isds_discard_credentials(context, 0);

    /* Destroy log-in request */
    xmlFreeNode(request);

    if (soap_err) {
        _isds_close_connection(context);
        return soap_err;
    }

    /* XXX: Until we don't propagate HTTP code 500 or 4xx, we can be sure
     * authentication succeeded if soap_err == IE_SUCCESS */
    err = IE_SUCCESS;

    if (!err)
        isds_log(ILF_ISDS, ILL_DEBUG,
                _("User %s has been logged into server %s successfully\n"),
            username, url);
    return err;
#else /* not HAVE_LIBCURL */
    return IE_NOTSUP;
#endif
}

isds_error isds_login_mep(struct isds_ctx *context, const char *url,
        const char *username, const char *code, struct isds_mep *mep) {
#if HAVE_LIBCURL
    isds_error err = IE_NOT_LOGGED_IN;
    isds_error soap_err;
    xmlNsPtr isds_ns = NULL;
    xmlNodePtr request = NULL;
#endif /* HAVE_LIBCURL */

    if (NULL == context) {
        return IE_INVALID_CONTEXT;
    }
    zfree(context->long_message);

#if HAVE_LIBCURL

    context->type = CTX_TYPE_ISDS;

    if ((NULL != username) && (NULL != code) && (NULL != mep)) {
        isds_log(ILF_SEC, ILL_INFO,
                _("Selected authentication method: username and mobile key\n"));
    } else {
        isds_log_message(context,
                "Username, communication code and mep context must be supplied.\n");
        return IE_INVAL;
    }
    /* Close connection if already logged in, but don't close the connection
     * if continuing to negotiate MEP authentication.*/
    if ((NULL != context->curl) && (NULL == mep->intermediate_uri)) {
        _isds_close_connection(context);
    }

    context->mep_credentials = mep;
    context->mep = (NULL != context->mep_credentials);

    if (context->mep) {
        if (NULL == url) {
            url = isds_mep_locator;
        }
        mep->resolution = MEP_RESOLUTION_UNKNOWN;
        const char *authenticator_uri =
            "%sas/processLogin?type=mep-ws&applicationName=%s&"
            "uri=%sapps/";
        const char *app_name = context->mep_credentials->app_name;
        if (NULL == app_name) {
            app_name = "";
        }
        char *escaped_app_name = curl_easy_escape(context->curl, app_name, 0);
        if (-1 == isds_asprintf(&context->url, authenticator_uri, url,
                    escaped_app_name, url)) {
            curl_free(escaped_app_name);
            return IE_NOMEM;
        }
        curl_free(escaped_app_name);
    }
    if (NULL == context->url) {
        return IE_NOMEM;
    }

    /* Prepare CURL handle */
    if (NULL == context->curl) {
        context->curl = curl_easy_init();
    }
    if (NULL == context->curl) {
        return IE_ERROR;
    }

    /* Build log-in request */
    request = xmlNewNode(NULL, BAD_CAST "DummyOperation");
    if (NULL == request) {
        isds_log_message(context, _("Could not build ISDS log-in request"));
        return IE_ERROR;
    }
    isds_ns = xmlNewNs(request, BAD_CAST ISDS_NS, NULL);
    if(NULL == isds_ns) {
        isds_log_message(context, _("Could not create ISDS name space"));
        xmlFreeNode(request);
        return IE_ERROR;
    }
    xmlSetNs(request, isds_ns);

    /* Store credentials, use  mobile key code for password. */
    _isds_discard_credentials(context, 1);
    if (IE_SUCCESS != _isds_store_credentials(context, username, code, NULL)) {
        _isds_discard_credentials(context, 1);
        xmlFreeNode(request);
        return IE_NOMEM;
    }

    isds_log(ILF_ISDS, ILL_DEBUG, _("Logging user %s into server %s\n"),
            username, url);

    /* XXX: ISDS documentation does not specify response body for
     * DummyOperation request.  However real server sends back
     * DummyOperationResponse.  Therefore we cannot check for the SOAP body
     * content and we call _isds_soap() instead of _isds().  _isds() checks for
     * SOAP body content, e.g. the dmStatus element. */

    /* Send log-in request */
    soap_err = _isds_soap(context, "DS/dz", request, NULL, NULL, NULL, NULL);

    if (context->mep) {
        /* Revert context URL from mobile key authentication service to web
         * service base URL for subsequent calls. */
        zfree(context->url);
        context->url = _isds_astrcat(url, "apps/");
        if (context->url == NULL) {
            soap_err = IE_NOMEM;
        }
        /* Detach credentials pointer from context. */
        context->mep_credentials = NULL;
    }

    /* Remove credentials */
    _isds_discard_credentials(context, 0);
   
    /* Destroy log-in request */
    xmlFreeNode(request);

    if ((IE_SUCCESS != soap_err) && (context->mep && (IE_PARTIAL_SUCCESS != soap_err))) {
        /* Don't close connection when using MEP authentication. */
        _isds_close_connection(context);
        return soap_err;
    }

    /* XXX: Until we don't propagate HTTP code 500 or 4xx, we can be sure
     * authentication succeeded if soap_err == IE_SUCCESS */
    err = soap_err;

    if (IE_SUCCESS == err) {
        isds_log(ILF_ISDS, ILL_DEBUG,
                _("User %s has been logged into server %s successfully\n"),
            username, url);
    }
    return err;
#else /* not HAVE_LIBCURL */
    return IE_NOTSUP;
#endif
}


/* Log out from ISDS server discards credentials and connection configuration. */
isds_error isds_logout(struct isds_ctx *context) {
    if (!context) return IE_INVALID_CONTEXT;
    zfree(context->long_message);

#if HAVE_LIBCURL
    if (context->curl) {
        if (context->otp || context->mep) {
            isds_error err = _isds_invalidate_otp_cookie(context);
            if (err) return err;
        }

        /* Close connection */
        _isds_close_connection(context);

        /* Discard credentials for sure. They should not survive isds_login(),
         * even successful .*/
        _isds_discard_credentials(context, 1);

        isds_log(ILF_ISDS, ILL_DEBUG, _("Logged out from ISDS server\n"));
    } else {
        _isds_discard_credentials(context, 1);
    }
    zfree(context->url);
    return IE_SUCCESS;
#else /* not HAVE_LIBCURL */
    return IE_NOTSUP;
#endif
}


/* Verify connection to ISDS is alive and server is responding.
 * Send dummy request to ISDS and expect dummy response. */
isds_error isds_ping(struct isds_ctx *context) {
#if HAVE_LIBCURL
    isds_error soap_err;
    xmlNsPtr isds_ns = NULL;
    xmlNodePtr request = NULL;
#endif /* HAVE_LIBCURL */

    if (!context) return IE_INVALID_CONTEXT;
    zfree(context->long_message);

#if HAVE_LIBCURL
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

    /* XXX: ISDS documentation does not specify response body for
     * DummyOperation request.  However real server sends back
     * DummyOperationResponse.  Therefore we cannot check for the SOAP body
     * content and we call _isds_soap() instead of _isds().  _isds() checks for
     * SOAP body content, e.g. the dmStatus element. */

    /* Send dummy request */
    soap_err = _isds_soap(context, "DS/dz", request, NULL, NULL, NULL, NULL);
   
    /* Destroy log-in request */
    xmlFreeNode(request);

    if (soap_err) {
        isds_log(ILF_ISDS, ILL_DEBUG,
                _("ISDS server could not be contacted\n"));
        return soap_err;
    }

    /* XXX: Until we don't propagate HTTP code 500 or 4xx, we can be sure
     * authentication succeeded if soap_err == IE_SUCCESS */


    isds_log(ILF_ISDS, ILL_DEBUG, _("ISDS server alive\n"));

    return IE_SUCCESS;
#else /* not HAVE_LIBCURL */
    return IE_NOTSUP;
#endif
}


/* Send bogus request to ISDS.
 * Just for test purposes */
isds_error isds_bogus_request(struct isds_ctx *context) {
#if HAVE_LIBCURL
    isds_error err;
    xmlNsPtr isds_ns = NULL;
    xmlNodePtr request = NULL;
    xmlDocPtr response = NULL;
    xmlChar *code = NULL, *message = NULL;
#endif

    if (!context) return IE_INVALID_CONTEXT;
    zfree(context->long_message);

#if HAVE_LIBCURL
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
    err = _isds(context, SERVICE_DM_OPERATIONS, request, &response, NULL, NULL);
   
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
        char *code_locale = _isds_utf82locale((char*)code);
        char *message_locale = _isds_utf82locale((char*)message);
        isds_log(ILF_ISDS, ILL_DEBUG,
                _("Server refused bogus request (code=%s, message=%s)\n"),
                code_locale, message_locale);
        /* XXX: Literal error messages from ISDS are Czech messages
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
#else /* not HAVE_LIBCURL */
    return IE_NOTSUP;
#endif
}


#if HAVE_LIBCURL
/* Serialize XML subtree to buffer preserving XML indentation.
 * @context is session context
 * @subtree is XML element to be serialized (with children)
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
     * meaningful value yet */
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
#endif /* HAVE_LIBCURL */


#if 0
/* Dump XML subtree to buffer as literal string, not valid XML possibly.
 * @context is session context
 * @document is original document where @nodeset points to
 * @nodeset is XPath node set to dump (recursively)
 * @buffer is automatically reallocated buffer where serialize to
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

    /* Resulting the document into buffer */
    xml_buffer = xmlBufferCreate();
    if (!xml_buffer) {
        isds_log_message(context, _("Could not create xmlBuffer"));
        err = IE_ERROR;
        goto leave;
    }
   
    /* Iterate over all nodes */
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
/* Dump XML subtree to buffer as literal string, not valid XML possibly.
 * @context is session context
 * @document is original document where @nodeset points to
 * @nodeset is XPath node set to dump (recursively)
 * @buffer is automatically reallocated buffer where serialize to
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

    /* Resulting the document into buffer */
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

   
    /* Iterate over all nodes */
    for (int i = 0; i < nodeset->nodeNr; i++) {
        /* Serialize node.
         * XXX: xmlNodeDump() appends to xml_buffer. */
        /*if (-1 ==
                xmlNodeDump(xml_buffer, document, nodeset->nodeTab[i], 0, 0)) {
                */
        /* XXX: According LibXML documentation, this function does not return
         * meaningful value yet */
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


#if HAVE_LIBCURL
/* Convert UTF-8 @string representation of ISDS dbType to enum @type */
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
    else if (!xmlStrcmp(string, BAD_CAST "PFO_AUDITOR"))
        *type = DBTYPE_PFO_AUDITOR;
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
    else if (!xmlStrcmp(string, BAD_CAST "OVM_FO"))
        *type = DBTYPE_OVM_FO;
    else if (!xmlStrcmp(string, BAD_CAST "OVM_PFO"))
        *type = DBTYPE_OVM_PFO;
    else if (!xmlStrcmp(string, BAD_CAST "OVM_PO"))
        *type = DBTYPE_OVM_PO;
    else
        return IE_ENUM;
    return IE_SUCCESS;
}


/* Convert ISDS dbType enum @type to UTF-8 string.
 * @Return pointer to static string, or NULL if unknown enum value */
static const xmlChar *isds_DbType2string(const isds_DbType type) {
     switch(type) {
            /* DBTYPE_SYSTEM and DBTYPE_OVM_MAIN are invalid values from point
             * of view of generic public SOAP interface. */
            case DBTYPE_FO: return(BAD_CAST "FO"); break;
            case DBTYPE_PFO: return(BAD_CAST "PFO"); break;
            case DBTYPE_PFO_ADVOK: return(BAD_CAST "PFO_ADVOK"); break;
            case DBTYPE_PFO_DANPOR: return(BAD_CAST "PFO_DANPOR"); break;
            case DBTYPE_PFO_INSSPR: return(BAD_CAST "PFO_INSSPR"); break;
            case DBTYPE_PFO_AUDITOR: return(BAD_CAST "PFO_AUDITOR"); break;
            case DBTYPE_PO: return(BAD_CAST "PO"); break;
            case DBTYPE_PO_ZAK: return(BAD_CAST "PO_ZAK"); break;
            case DBTYPE_PO_REQ: return(BAD_CAST "PO_REQ"); break;
            case DBTYPE_OVM: return(BAD_CAST "OVM"); break;
            case DBTYPE_OVM_NOTAR: return(BAD_CAST "OVM_NOTAR"); break;
            case DBTYPE_OVM_EXEKUT: return(BAD_CAST "OVM_EXEKUT"); break;
            case DBTYPE_OVM_REQ: return(BAD_CAST "OVM_REQ"); break;
            case DBTYPE_OVM_FO: return(BAD_CAST "OVM_FO"); break;
            case DBTYPE_OVM_PFO: return(BAD_CAST "OVM_PFO"); break;
            case DBTYPE_OVM_PO: return(BAD_CAST "OVM_PO"); break;
            default: return NULL; break;
        }
}


/* Convert UTF-8 @string representation of ISDS userType to enum @type */
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
    else if (!xmlStrcmp(string, BAD_CAST "OFFICIAL_CERT"))
        *type = USERTYPE_OFFICIAL_CERT;
    else if (!xmlStrcmp(string, BAD_CAST "LIQUIDATOR"))
        *type = USERTYPE_LIQUIDATOR;
    else if (!xmlStrcmp(string, BAD_CAST "RECEIVER"))
        *type = USERTYPE_RECEIVER;
    else if (!xmlStrcmp(string, BAD_CAST "GUARDIAN"))
        *type = USERTYPE_GUARDIAN;
    else
        return IE_ENUM;
    return IE_SUCCESS;
}


/* Convert ISDS userType enum @type to UTF-8 string.
 * @Return pointer to static string, or NULL if unknown enum value */
static const xmlChar *isds_UserType2string(const isds_UserType type) {
    switch(type) {
        case USERTYPE_PRIMARY: return(BAD_CAST "PRIMARY_USER"); break;
        case USERTYPE_ENTRUSTED: return(BAD_CAST "ENTRUSTED_USER"); break;
        case USERTYPE_ADMINISTRATOR: return(BAD_CAST "ADMINISTRATOR"); break;
        case USERTYPE_OFFICIAL: return(BAD_CAST "OFFICIAL"); break;
        case USERTYPE_OFFICIAL_CERT: return(BAD_CAST "OFFICIAL_CERT"); break;
        case USERTYPE_LIQUIDATOR: return(BAD_CAST "LIQUIDATOR"); break;
        case USERTYPE_RECEIVER: return(BAD_CAST "RECEIVER"); break;
        case USERTYPE_GUARDIAN: return(BAD_CAST "GUARDIAN"); break;
        default: return NULL; break;
    }
}


/* Convert UTF-8 @string representation of ISDS sender type to enum @type */
static isds_error string2isds_sender_type(const xmlChar *string,
        isds_sender_type *type) {
    if (!string || !type) return IE_INVAL;

    if (!xmlStrcmp(string, BAD_CAST "PRIMARY_USER"))
        *type = SENDERTYPE_PRIMARY;
    else if (!xmlStrcmp(string, BAD_CAST "ENTRUSTED_USER"))
        *type = SENDERTYPE_ENTRUSTED;
    else if (!xmlStrcmp(string, BAD_CAST "ADMINISTRATOR"))
        *type = SENDERTYPE_ADMINISTRATOR;
    else if (!xmlStrcmp(string, BAD_CAST "OFFICIAL"))
        *type = SENDERTYPE_OFFICIAL;
    else if (!xmlStrcmp(string, BAD_CAST "VIRTUAL"))
        *type = SENDERTYPE_VIRTUAL;
    else if (!xmlStrcmp(string, BAD_CAST "OFFICIAL_CERT"))
        *type = SENDERTYPE_OFFICIAL_CERT;
    else if (!xmlStrcmp(string, BAD_CAST "LIQUIDATOR"))
        *type = SENDERTYPE_LIQUIDATOR;
    else if (!xmlStrcmp(string, BAD_CAST "RECEIVER"))
        *type = SENDERTYPE_RECEIVER;
    else if (!xmlStrcmp(string, BAD_CAST "GUARDIAN"))
        *type = SENDERTYPE_GUARDIAN;
    else
        return IE_ENUM;
    return IE_SUCCESS;
}


/* Convert UTF-8 @string representation of ISDS PDZType to enum @type */
static isds_error string2isds_payment_type(const xmlChar *string,
        isds_payment_type *type) {
    if (!string || !type) return IE_INVAL;

    if (!xmlStrcmp(string, BAD_CAST "K"))
        *type = PAYMENT_SENDER;
    else if (!xmlStrcmp(string, BAD_CAST "O"))
        *type = PAYMENT_RESPONSE;
    else if (!xmlStrcmp(string, BAD_CAST "G"))
        *type = PAYMENT_SPONSOR;
    else if (!xmlStrcmp(string, BAD_CAST "Z"))
        *type = PAYMENT_SPONSOR_LIMITED;
    else if (!xmlStrcmp(string, BAD_CAST "D"))
        *type = PAYMENT_SPONSOR_EXTERNAL;
    else if (!xmlStrcmp(string, BAD_CAST "E"))
        *type = PAYMENT_STAMP;
    else
        return IE_ENUM;
    return IE_SUCCESS;
}


/* Convert UTF-8 @string representation of ISDS ciEventType to enum @type.
 * ciEventType is integer but we convert it from string representation
 * directly. */
static isds_error string2isds_credit_event_type(const xmlChar *string,
        isds_credit_event_type *type) {
    if (!string || !type) return IE_INVAL;

    if (!xmlStrcmp(string, BAD_CAST "1"))
        *type = ISDS_CREDIT_CHARGED;
    else if (!xmlStrcmp(string, BAD_CAST "2"))
        *type = ISDS_CREDIT_DISCHARGED;
    else if (!xmlStrcmp(string, BAD_CAST "3"))
        *type = ISDS_CREDIT_MESSAGE_SENT;
    else if (!xmlStrcmp(string, BAD_CAST "4"))
        *type = ISDS_CREDIT_STORAGE_SET;
    else if (!xmlStrcmp(string, BAD_CAST "5"))
        *type = ISDS_CREDIT_EXPIRED;
    else
        return IE_ENUM;
    return IE_SUCCESS;
}


/* Convert ISDS dmFileMetaType enum @type to UTF-8 string.
 * @Return pointer to static string, or NULL if unknown enum value */
static const xmlChar *isds_FileMetaType2string(const isds_FileMetaType type) {
     switch(type) {
            case FILEMETATYPE_MAIN: return(BAD_CAST "main"); break;
            case FILEMETATYPE_ENCLOSURE: return(BAD_CAST "enclosure"); break;
            case FILEMETATYPE_SIGNATURE: return(BAD_CAST "signature"); break;
            case FILEMETATYPE_META: return(BAD_CAST "meta"); break;
            default: return NULL; break;
        }
}


/* Convert isds_fulltext_target enum @type to UTF-8 string for
 * ISDSSearch2/searchType value.
 * @Return pointer to static string, or NULL if unknown enum value */
static const xmlChar *isds_fulltext_target2string(
        const isds_fulltext_target type) {
     switch(type) {
            case FULLTEXT_ALL: return(BAD_CAST "GENERAL"); break;
            case FULLTEXT_ADDRESS: return(BAD_CAST "ADDRESS"); break;
            case FULLTEXT_IC: return(BAD_CAST "ICO"); break;
            case FULLTEXT_BOX_ID: return(BAD_CAST "DBID"); break;
            default: return NULL; break;
        }
}
#endif /* HAVE_LIBCURL */


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


#if HAVE_LIBCURL
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
    time_t seconds_as_time_t;

    if (!time || !string) return IE_INVAL;

    /* MinGW32 GCC 4.8+ uses 64-bit time_t but time->tv_sec is defined as
     * 32-bit long in Microsoft API. Convert value to the type expected by
     * gmtime_r(). */
    seconds_as_time_t = time->tv_sec;
    if (!gmtime_r(&seconds_as_time_t, &broken)) return IE_DATE;
    if (time->tv_usec < 0 || time->tv_usec > 999999) return IE_DATE;

    /* TODO: small negative year should be formatted as "-0012". This is not
     * true for glibc "%04d". We should implement it.
     * time->tv_usec type is su_seconds_t which is required to be signed
     * integer to accomodate values from range [-1, 1000000].
     * See <http://www.w3.org/TR/2001/REC-xmlschema-2-20010502/#dateTime> */ 
    /* XXX: Do not format time->tv_usec as intmax_t because %jd is not
     * supported and PRIdMAX is broken on MingGW. We can use int32_t because
     * of the range check above. */
    if (-1 == isds_asprintf((char **) string,
                "%04d-%02d-%02dT%02d:%02d:%02d.%06" PRId32,
                broken.tm_year + 1900, broken.tm_mon + 1, broken.tm_mday,
                broken.tm_hour, broken.tm_min, broken.tm_sec,
                (int32_t)time->tv_usec))
        return IE_ERROR;

    return IE_SUCCESS;
}
#endif /* HAVE_LIBCURL */


/* Convert UTF-8 ISO 8601 date-time @string to static struct timeval.
 * It respects microseconds too. Microseconds are rounded half up.
 * In case of error, @time will be undefined. */
static isds_error timestring2static_timeval(const xmlChar *string,
        struct timeval *time) {
    struct tm broken;
    char *offset, *delim, *endptr;
    const int subsecond_resolution = 6;
    char subseconds[subsecond_resolution + 1];
    _Bool round_up = 0;
    int offset_hours, offset_minutes;
    int i;
    long int long_number;
#ifdef _WIN32
    int tmp;
#endif
    
    if (!time) return IE_INVAL;
    if (!string) {
        return IE_INVAL;
    }

    memset(&broken, 0, sizeof(broken));
    memset(time, 0, sizeof(*time));


    /* xsd:date is ISO 8601 string, thus ASCII */
    /*TODO: negative year */

#ifdef _WIN32
    i = 0;
    if ((tmp = sscanf((const char*)string, "%d-%d-%dT%d:%d:%d%n",
        &broken.tm_year, &broken.tm_mon, &broken.tm_mday,
        &broken.tm_hour, &broken.tm_min, &broken.tm_sec,
        &i)) < 6) {
        return IE_DATE;
    }

    broken.tm_year -= 1900;
    broken.tm_mon--;
    broken.tm_isdst = -1;
    offset = (char*)string + i;
#else
    /* Parse date and time without subseconds and offset */
    offset = strptime((char*)string, "%Y-%m-%dT%T", &broken);
    if (!offset) {
        return IE_DATE;
    }
#endif
    
    /* Get subseconds */
    if (*offset == '.' ) {
        offset++;

        /* Copy first 6 digits, pad it with zeros.
         * Current server implementation uses only millisecond resolution. */
        /* TODO: isdigit() is locale sensitive */
        for (i = 0;
                i < subsecond_resolution && isdigit(*offset);
                i++, offset++) {
            subseconds[i] = *offset;
        }
        if (subsecond_resolution == i && isdigit(*offset)) {
            /* Check 7th digit for rounding */
            if (*offset >= '5') round_up = 1;
            offset++;
        }
        for (; i < subsecond_resolution; i++) {
            subseconds[i] = '0';
        }
        subseconds[subsecond_resolution] = '\0';

        /* Convert it into integer */
        long_number = strtol(subseconds, &endptr, 10);
        if (*endptr != '\0' || long_number == LONG_MIN ||
                long_number == LONG_MAX) {
            return IE_DATE;
        }
        /* POSIX sys_time.h(0p) defines tv_usec timeval member as su_seconds_t
         * type. sys_types.h(0p) defines su_seconds_t as "used for time in
         * microseconds" and "the type shall be a signed integer capable of
         * storing values at least in the range [-1, 1000000]. */
        if (long_number < -1 || long_number >= 1000000) {
            return IE_DATE;
        }
        time->tv_usec = long_number;

        /* Round the subseconds */
        if (round_up) {
            if (999999 == time->tv_usec) {
                time->tv_usec = 0;
                broken.tm_sec++;
            } else {
                time->tv_usec++;
            }
        }

        /* move to the zone offset delimiter or signal NULL*/
        delim = strchr(offset, '-');
        if (!delim)
            delim = strchr(offset, '+');
        if (!delim)
            delim = strchr(offset, 'Z');
        offset = delim;
    }

    /* Get zone offset */
    /* ISO allows zone offset string only: "" | "Z" | ("+"|"-" "<HH>:<MM>")
     * "" equals to "Z" and it means UTC zone. */
    /* One can not use strptime(, "%z",) becase it's RFC E-MAIL format without
     * colon separator */
    if (offset && (*offset == '-' || *offset == '+')) {
        if (2 != sscanf(offset + 1, "%2d:%2d", &offset_hours, &offset_minutes)) {
            return IE_DATE;
        }
        if (*offset == '+') {
            broken.tm_hour -= offset_hours;
            broken.tm_min -= offset_minutes;
        } else {
            broken.tm_hour += offset_hours;
            broken.tm_min += offset_minutes;
        }
    }

    /* Convert to time_t */
    time->tv_sec = _isds_timegm(&broken);
    if (time->tv_sec == (time_t) -1) {
        return IE_DATE;
    }

    return IE_SUCCESS;
}


/* Convert UTF-8 ISO 8601 date-time @string to reallocated struct timeval.
 * It respects microseconds too. Microseconds are rounded half up.
 * In case of error, @time will be freed. */
static isds_error timestring2timeval(const xmlChar *string,
        struct timeval **time) {
    isds_error error;
    
    if (!time) return IE_INVAL;
    if (!string) {
        zfree(*time);
        return IE_INVAL;
    }

    if (!*time) {
        *time = calloc(1, sizeof(**time));
        if (!*time) return IE_NOMEM;
    } else {
        memset(*time, 0, sizeof(**time));
    }

    error = timestring2static_timeval(string, *time);
    if (error) {
        zfree(*time);
    }

    return error;
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
        isds_printf_message(context, _("Invalid message status value: %lu"),
                *number);
        return IE_ENUM;
    }

    *status = malloc(sizeof(**status));
    if (!*status) return IE_NOMEM;

    **status = 1 << *number;
    return IE_SUCCESS;
}


/* Convert event description string into isds_event members type and
 * description
 * @string is raw event description starting with event prefix
 * @event is structure where to store type and stripped description to
 * @return standard error code, unknown prefix is not classified as an error.
 * */
static isds_error eventstring2event(const xmlChar *string,
        struct isds_event* event) {
    const xmlChar *known_prefixes[] = {
        BAD_CAST "EV0:",
        BAD_CAST "EV1:",
        BAD_CAST "EV2:",
        BAD_CAST "EV3:",
        BAD_CAST "EV4:",
        BAD_CAST "EV5:",
        BAD_CAST "EV8:",
        BAD_CAST "EV11:",
        BAD_CAST "EV12:",
        BAD_CAST "EV13:"
    };
    const isds_event_type types[] = {
        EVENT_ENTERED_SYSTEM, 
        EVENT_ACCEPTED_BY_RECIPIENT,
        EVENT_ACCEPTED_BY_FICTION,
        EVENT_UNDELIVERABLE,
        EVENT_COMMERCIAL_ACCEPTED,
        EVENT_DELIVERED,
        EVENT_UNDELIVERED_AV_CHECK,
        EVENT_PRIMARY_LOGIN,
        EVENT_ENTRUSTED_LOGIN,
        EVENT_SYSCERT_LOGIN
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
            /* TODO: Recognize all white spaces from UCS blank class and
             * operate on UTF-8 chars. */
            for (; string[length] != '\0' && string[length] == ' '; length++);
            event->description = strdup((char *) (string + length));
            if (!(event->description)) return IE_NOMEM;

            return IE_SUCCESS;
        }
    }
    
    /* Unknown event prefix.
     * XSD allows any string */
    char *string_locale = _isds_utf82locale((char *) string);
    isds_log(ILF_ISDS, ILL_WARNING,
            _("Unknown delivery info event prefix: %s\n"), string_locale);
    free(string_locale);

    *event->type = EVENT_UKNOWN;
    event->description = strdup((char *) string);
    if (!(event->description)) return IE_NOMEM;

    return IE_SUCCESS;
}


/* Following EXTRACT_* macros expect @result, @xpath_ctx, @err, @context
 * and leave label */
#define EXTRACT_STRING(element, string) { \
    xmlXPathFreeObject(result); \
    result = xmlXPathEvalExpression(BAD_CAST element "/text()", xpath_ctx); \
    if (NULL == (result)) { \
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
        if (NULL == (string)) { \
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
                char *string_locale = _isds_utf82locale((char*)string); \
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

#define EXTRACT_BOOLEANNOPTR(element, boolean) \
    { \
        char *string = NULL; \
        EXTRACT_STRING(element, string); \
         \
        if (NULL == string) { \
            isds_printf_message(context, _("%s element is empty"), element); \
            err = IE_ERROR; \
            goto leave; \
        } \
        if (!xmlStrcmp((xmlChar *)string, BAD_CAST "true") || \
                !xmlStrcmp((xmlChar *)string, BAD_CAST "1")) \
            (boolean) = 1; \
        else if (!xmlStrcmp((xmlChar *)string, BAD_CAST "false") || \
                !xmlStrcmp((xmlChar *)string, BAD_CAST "0")) \
            (boolean) = 0; \
        else { \
            char *string_locale = _isds_utf82locale((char*)string); \
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
                char *string_locale = _isds_utf82locale((char *)string); \
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
                char *string_locale = _isds_utf82locale((char *)string); \
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
                char *string_locale = _isds_utf82locale((char *)string); \
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
                char *string_locale = _isds_utf82locale((char *)string); \
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

#define EXTRACT_DATE(element, tmPtr) { \
    char *string = NULL; \
    EXTRACT_STRING(element, string); \
    if (NULL != string) { \
        (tmPtr) = calloc(1, sizeof(*(tmPtr))); \
        if (NULL == (tmPtr)) { \
            free(string); \
            err = IE_NOMEM; \
            goto leave; \
        } \
        err = _isds_datestring2tm((xmlChar *)string, (tmPtr)); \
        if (err) { \
            if (err == IE_NOTSUP) { \
                err = IE_ISDS; \
                char *string_locale = _isds_utf82locale(string); \
                char *element_locale = _isds_utf82locale(element); \
                isds_printf_message(context, _("Invalid %s value: %s"), \
                        element_locale, string_locale); \
                free(string_locale); \
                free(element_locale); \
            } \
            free(string); \
            goto leave; \
        } \
        free(string); \
    } \
}

#define EXTRACT_STRING_ATTRIBUTE(attribute, string, required) { \
    (string) = (char *) xmlGetNsProp(xpath_ctx->node, ( BAD_CAST attribute), \
            NULL); \
    if ((required) && (!string)) { \
        char *attribute_locale = _isds_utf82locale(attribute); \
        char *element_locale = \
            _isds_utf82locale((char *)xpath_ctx->node->name); \
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

/* Requires attribute_node variable, do not free it. Can be used to reffer to
 * new attribute. */
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
 * it. The child must be uniq and must exist. Otherwise fails.
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
        char *parent_locale = _isds_utf82locale((char*) xpath_ctx->node->name);
        char *child_locale = _isds_utf82locale((char*) child);
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
        char *parent_locale = _isds_utf82locale((char*) xpath_ctx->node->name);
        char *child_locale = _isds_utf82locale((char*) child);
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



#if HAVE_LIBCURL
/* Find and convert XSD:gPersonName group in current node into structure
 * @context is ISDS context
 * @personName is automatically reallocated person name structure. If no member
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
 * @address is automatically reallocated address structure. If no member
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
 * @biDate is automatically reallocated birth date structure. If no member
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
        err = _isds_datestring2tm((xmlChar *)string, *biDate);
        if (err) {
            if (err == IE_NOTSUP) {
                err = IE_ISDS;
                char *string_locale = _isds_utf82locale(string);
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
 * @db_owner_info is automatically reallocated box owner info structure
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
                char *string_locale = _isds_utf82locale(string);
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
 * @context is session context
 * @owner is libisds structure with box description
 * @db_owner_info is XML element of XSD:tDbOwnerInfo */
static isds_error insert_DbOwnerInfo(struct isds_ctx *context,
        const struct isds_DbOwnerInfo *owner, xmlNodePtr db_owner_info) {

    isds_error err = IE_SUCCESS;
    xmlNodePtr node;
    xmlChar *string = NULL;
    const xmlChar *type_string = NULL;

    if (!context) return IE_INVALID_CONTEXT;
    if (!owner || !db_owner_info) return IE_INVAL;


    /* Build XSD:tDbOwnerInfo */
    /* XXX: All the elements except email and telNumber are mandatory. */
    CHECK_FOR_STRING_LENGTH(owner->dbID, 0, 7, "dbID")
    INSERT_STRING(db_owner_info, "dbID", owner->dbID);

    /* dbType */
    if (owner->dbType) {
        type_string = isds_DbType2string(*(owner->dbType));
        if (!type_string) {
            isds_printf_message(context, _("Invalid dbType value: %d"),
                    *(owner->dbType));
            err = IE_ENUM;
            goto leave;
        }
    }
    INSERT_STRING(db_owner_info, "dbType", type_string);

    INSERT_STRING(db_owner_info, "ic", owner->ic);

    INSERT_STRING(db_owner_info, "pnFirstName",
            (NULL == owner->personName) ? NULL: owner->personName->pnFirstName);
    INSERT_STRING(db_owner_info, "pnMiddleName",
            (NULL == owner->personName) ? NULL: owner->personName->pnMiddleName);
    INSERT_STRING(db_owner_info, "pnLastName",
            (NULL == owner->personName) ? NULL: owner->personName->pnLastName);
    INSERT_STRING(db_owner_info, "pnLastNameAtBirth",
            (NULL == owner->personName) ? NULL:
                owner->personName->pnLastNameAtBirth);

    INSERT_STRING(db_owner_info, "firmName", owner->firmName);

    if (NULL != owner->birthInfo && NULL != owner->birthInfo->biDate) {
        err = tm2datestring(owner->birthInfo->biDate, &string);
        if (err) goto leave;
    }
    INSERT_STRING(db_owner_info, "biDate", string);
    zfree(string);

    INSERT_STRING(db_owner_info, "biCity",
            (NULL == owner->birthInfo) ? NULL: owner->birthInfo->biCity);
    INSERT_STRING(db_owner_info, "biCounty",
            (NULL == owner->birthInfo) ? NULL: owner->birthInfo->biCounty);
    INSERT_STRING(db_owner_info, "biState",
            (NULL == owner->birthInfo) ? NULL: owner->birthInfo->biState);

    INSERT_STRING(db_owner_info, "adCity",
            (NULL == owner->address) ? NULL: owner->address->adCity);
    INSERT_STRING(db_owner_info, "adStreet",
            (NULL == owner->address) ? NULL: owner->address->adStreet);
    INSERT_STRING(db_owner_info, "adNumberInStreet",
            (NULL == owner->address) ? NULL: owner->address->adNumberInStreet);
    INSERT_STRING(db_owner_info, "adNumberInMunicipality",
            (NULL == owner->address) ? NULL: owner->address->adNumberInMunicipality);
    INSERT_STRING(db_owner_info, "adZipCode",
            (NULL == owner->address) ? NULL: owner->address->adZipCode);
    INSERT_STRING(db_owner_info, "adState",
            (NULL == owner->address) ? NULL: owner->address->adState);

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
 * @db_user_info is automatically reallocated user info structure
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
                char *string_locale = _isds_utf82locale(string);
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

    /* ???: Default value is "CZ" according specification. Should we provide
     * it? */
    EXTRACT_STRING("isds:caState", (*db_user_info)->caState);

leave:
    if (err) isds_DbUserInfo_free(db_user_info);
    free(string);
    xmlXPathFreeObject(result);
    return err;
}


/* Insert struct isds_DbUserInfo data (user description) into XML tree
 * @context is session context
 * @user is libisds structure with user description
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
    INSERT_STRING(db_user_info, "caState", user->caState);

leave:
    free(string);
    return err;
}


/* Convert XSD:tPDZRec XML tree into structure
 * @context is ISDS context
 * @permission is automatically reallocated commercial permission structure
 * @xpath_ctx is XPath context with current node as XSD:tPDZRec element
 * In case of error @permission will be freed. */
static isds_error extract_DbPDZRecord(struct isds_ctx *context,
        struct isds_commercial_permission **permission,
        xmlXPathContextPtr xpath_ctx) {
    isds_error err = IE_SUCCESS;
    xmlXPathObjectPtr result = NULL;
    char *string = NULL;

    if (!context) return IE_INVALID_CONTEXT;
    if (!permission) return IE_INVAL;
    isds_commercial_permission_free(permission);
    if (!xpath_ctx) return IE_INVAL;


    *permission = calloc(1, sizeof(**permission));
    if (!*permission) {
        err = IE_NOMEM;
        goto leave;
    }

    EXTRACT_STRING("isds:PDZType", string);
    if (string) {
        err = string2isds_payment_type((xmlChar *)string,
                &(*permission)->type);
        if (err) {
            if (err == IE_ENUM) {
                err = IE_ISDS;
                char *string_locale = _isds_utf82locale(string);
                isds_printf_message(context,
                        _("Unknown isds:PDZType value: %s"), string_locale);
                free(string_locale);
            }
            goto leave;
        }
        zfree(string);
    }

    EXTRACT_STRING("isds:PDZRecip", (*permission)->recipient);
    EXTRACT_STRING("isds:PDZPayer", (*permission)->payer);

    EXTRACT_STRING("isds:PDZExpire", string);
    if (string) {
        err = timestring2timeval((xmlChar *) string,
                &((*permission)->expiration));
        if (err) {
            char *string_locale = _isds_utf82locale(string);
            if (err == IE_DATE) err = IE_ISDS;
            isds_printf_message(context,
                    _("Could not convert PDZExpire as ISO time: %s"),
                    string_locale);
            free(string_locale);
            goto leave;
        }
        zfree(string);
    }

    EXTRACT_ULONGINT("isds:PDZCnt", (*permission)->count, 0); 
    EXTRACT_STRING("isds:ODZIdent", (*permission)->reply_identifier);

leave:
    if (err) isds_commercial_permission_free(permission);
    free(string);
    xmlXPathFreeObject(result);
    return err;
}


/* Convert XSD:tCiRecord XML tree into structure
 * @context is ISDS context
 * @event is automatically reallocated commercial credit event structure
 * @xpath_ctx is XPath context with current node as XSD:tCiRecord element
 * In case of error @event will be freed. */
static isds_error extract_CiRecord(struct isds_ctx *context,
        struct isds_credit_event **event,
        xmlXPathContextPtr xpath_ctx) {
    isds_error err = IE_SUCCESS;
    xmlXPathObjectPtr result = NULL;
    char *string = NULL;
    long int *number_ptr;

    if (!context) return IE_INVALID_CONTEXT;
    if (!event) return IE_INVAL;
    isds_credit_event_free(event);
    if (!xpath_ctx) return IE_INVAL;


    *event = calloc(1, sizeof(**event));
    if (!*event) {
        err = IE_NOMEM;
        goto leave;
    }

    EXTRACT_STRING("isds:ciEventTime", string);
    if (string) {
        err = timestring2timeval((xmlChar *) string,
                &(*event)->time);
        if (err) {
            char *string_locale = _isds_utf82locale(string);
            if (err == IE_DATE) err = IE_ISDS;
            isds_printf_message(context,
                    _("Could not convert ciEventTime as ISO time: %s"),
                    string_locale);
            free(string_locale);
            goto leave;
        }
        zfree(string);
    }

    EXTRACT_STRING("isds:ciEventType", string);
    if (string) {
        err = string2isds_credit_event_type((xmlChar *)string,
                &(*event)->type);
        if (err) {
            if (err == IE_ENUM) {
                err = IE_ISDS;
                char *string_locale = _isds_utf82locale(string);
                isds_printf_message(context,
                        _("Unknown isds:ciEventType value: %s"), string_locale);
                free(string_locale);
            }
            goto leave;
        }
        zfree(string);
    }

    number_ptr = &((*event)->credit_change);
    EXTRACT_LONGINT("isds:ciCreditChange", number_ptr, 1);
    number_ptr = &(*event)->new_credit;
    EXTRACT_LONGINT("isds:ciCreditAfter", number_ptr, 1);

    switch((*event)->type) {
        case ISDS_CREDIT_CHARGED:
            EXTRACT_STRING("isds:ciTransID",
                    (*event)->details.charged.transaction);
            break;
        case ISDS_CREDIT_DISCHARGED:
            EXTRACT_STRING("isds:ciTransID",
                    (*event)->details.discharged.transaction);
            break;
        case ISDS_CREDIT_MESSAGE_SENT:
            EXTRACT_STRING("isds:ciRecipientID",
                (*event)->details.message_sent.recipient);
            EXTRACT_STRING("isds:ciPDZID",
                (*event)->details.message_sent.message_id);
            break;
        case ISDS_CREDIT_STORAGE_SET:
            number_ptr = &((*event)->details.storage_set.new_capacity);
            EXTRACT_LONGINT("isds:ciNewCapacity", number_ptr, 1);
            EXTRACT_DATE("isds:ciNewFrom",
                (*event)->details.storage_set.new_valid_from);
            EXTRACT_DATE("isds:ciNewTo",
                (*event)->details.storage_set.new_valid_to);
            EXTRACT_LONGINT("isds:ciOldCapacity",
                (*event)->details.storage_set.old_capacity, 0);
            EXTRACT_DATE("isds:ciOldFrom",
                (*event)->details.storage_set.old_valid_from);
            EXTRACT_DATE("isds:ciOldTo",
                (*event)->details.storage_set.old_valid_to);
            EXTRACT_STRING("isds:ciDoneBy",
                    (*event)->details.storage_set.initiator);
            break; 
        case ISDS_CREDIT_EXPIRED:
            break;
    }

leave:
    if (err) isds_credit_event_free(event);
    free(string);
    xmlXPathFreeObject(result);
    return err;
}


#endif /* HAVE_LIBCURL */


/* Convert XSD gMessageEnvelopeSub group of elements from XML tree into
 * isds_envelope structure. The envelope is automatically allocated but not
 * reallocated. The date are just appended into envelope structure.
 * @context is ISDS context
 * @envelope is automatically allocated message envelope structure
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
        zfree((*envelope)->dmRecipientOrgUnitNum);
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
            (*envelope)->dmRecipientOrgUnitNum, 0);
    EXTRACT_STRING("isds:dmToHands", (*envelope)->dmToHands);
    EXTRACT_STRING("isds:dmAnnotation", (*envelope)->dmAnnotation);
    EXTRACT_STRING("isds:dmRecipientRefNumber",
            (*envelope)->dmRecipientRefNumber);
    EXTRACT_STRING("isds:dmSenderRefNumber", (*envelope)->dmSenderRefNumber);
    EXTRACT_STRING("isds:dmRecipientIdent", (*envelope)->dmRecipientIdent);
    EXTRACT_STRING("isds:dmSenderIdent", (*envelope)->dmSenderIdent);

    /* Extract envelope elements regarding law reference */
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
 * @envelope is automatically allocated message envelope structure
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
    /* XML Schema does not guarantee enumeration. It's plain xs:int. */
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
 * @envelope is automatically allocated message envelope structure
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
            char *string_locale = _isds_utf82locale(string);
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
            char *string_locale = _isds_utf82locale(string);
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
 * @envelope is automatically allocated message envelope structure
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
        char *type_locale = _isds_utf82locale((*envelope)->dmType);
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


#if HAVE_LIBCURL
/* Convert dmType isds_envelope member into XML attribute and append it to
 * current node.
 * @context is ISDS context
 * @type is UTF-8 encoded string one multibyte long exactly or NULL to omit
 * @dm_envelope is XML element the resulting attribute will be appended to.
 * @return error code, in case of error context' message is filled. */
static isds_error insert_message_type(struct isds_ctx *context,
        const char *type, xmlNodePtr dm_envelope) {
    isds_error err = IE_SUCCESS;
    xmlAttrPtr attribute_node;

    if (!context) return IE_INVALID_CONTEXT;
    if (!dm_envelope) return IE_INVAL;

    /* Insert optional message type */
    if (type) {
        if (1 != xmlUTF8Strlen((xmlChar *) type)) {
            char *type_locale = _isds_utf82locale(type);
            isds_printf_message(context,
                    _("Message type in envelope is not 1 character long: %s"),
                    type_locale);
            free(type_locale);
            err = IE_INVAL;
            goto leave;
        }
        INSERT_STRING_ATTRIBUTE(dm_envelope, "dmType", type);
    }

leave:
    return err;
}
#endif /* HAVE_LIBCURL */


/* Extract message document into reallocated document structure
 * @context is ISDS context
 * @document is automatically reallocated message documents structure
 * @xpath_ctx is XPath context with current node as isds:dmFile
 * In case of error @document will be freed. */
static isds_error extract_document(struct isds_ctx *context,
        struct isds_document **document, xmlXPathContextPtr xpath_ctx) {
    isds_error err = IE_SUCCESS;
    xmlXPathObjectPtr result = NULL;
    xmlNodePtr file_node;
    char *string = NULL;

    if (!context) return IE_INVALID_CONTEXT;
    if (!document) return IE_INVAL;
    isds_document_free(document);
    if (!xpath_ctx) return IE_INVAL;
    file_node = xpath_ctx->node;

    *document = calloc(1, sizeof(**document));
    if (!*document) {
        err = IE_NOMEM;
        goto leave;
    }

    /* Extract document meta data */
    EXTRACT_STRING_ATTRIBUTE("dmMimeType", (*document)->dmMimeType, 1)
    if (context->normalize_mime_type) {
        const char *normalized_type =
            isds_normalize_mime_type((*document)->dmMimeType);
        if (NULL != normalized_type &&
                normalized_type != (*document)->dmMimeType) {
            char *new_type = strdup(normalized_type);
            if (NULL == new_type) {
                isds_printf_message(context,
                        _("Not enough memory to normalize document MIME type"));
                err = IE_NOMEM;
                goto leave;
            }
            free((*document)->dmMimeType);
            (*document)->dmMimeType = new_type;
        }
    }

    EXTRACT_STRING_ATTRIBUTE("dmFileMetaType", string, 1)
    err = string2isds_FileMetaType((xmlChar*)string,
            &((*document)->dmFileMetaType));
    if (err) {
        char *meta_type_locale = _isds_utf82locale(string);
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

    /* Check for dmEncodedContent */
    result = xmlXPathEvalExpression(BAD_CAST "isds:dmEncodedContent",
            xpath_ctx);
    if (!result) {
        err = IE_XML;
        goto leave;
    }

    if (!xmlXPathNodeSetIsEmpty(result->nodesetval)) {
        /* Here we have Base64 blob */
        (*document)->is_xml = 0;

        if (result->nodesetval->nodeNr > 1) {
            isds_printf_message(context,
                    _("Document has more dmEncodedContent elements"));
            err = IE_ISDS;
            goto leave;
        }

        xmlXPathFreeObject(result); result = NULL;
        EXTRACT_STRING("isds:dmEncodedContent", string);

        /* Decode non-empty document */
        if (string && string[0] != '\0') {
            (*document)->data_length =
                _isds_b64decode(string, &((*document)->data));
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
            (*document)->is_xml = 1;

            if (result->nodesetval->nodeNr > 1) {
                isds_printf_message(context,
                        _("Document has more dmXMLContent elements"));
                err = IE_ISDS;
                goto leave;
            }

            /* XXX: We cannot serialize the content simply because:
             * - XML document may point out of its scope (e.g. to message
             *   envelope)
             * - isds:dmXMLContent can contain more elements, no element,
             *   a text node only
             * - it's not the XML way
             * Thus we provide the only right solution: XML DOM. Let's
             * application to cope with this hot potato :) */
            (*document)->xml_node_list =
                result->nodesetval->nodeTab[0]->children;
        } else {
            /* No base64 blob, nor XML document */
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
 * @documents is automatically reallocated message documents list structure
 * @xpath_ctx is XPath context with current node as XSD tFilesArray
 * In case of error @documents will be freed. */
static isds_error extract_documents(struct isds_ctx *context,
        struct isds_list **documents, xmlXPathContextPtr xpath_ctx) {
    isds_error err = IE_SUCCESS;
    xmlXPathObjectPtr result = NULL;
    xmlNodePtr files_node;
    struct isds_list *document, *prev_document = NULL;

    if (!context) return IE_INVALID_CONTEXT;
    if (!documents) return IE_INVAL;
    isds_list_free(documents);
    if (!xpath_ctx) return IE_INVAL;
    files_node = xpath_ctx->node;

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


#if HAVE_LIBCURL
/* Convert isds:dmRecord XML tree into structure
 * @context is ISDS context
 * @envelope is automatically reallocated message envelope structure
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

    /* Get message type */
    err = append_message_type(context, envelope, xpath_ctx);
    if (err) goto leave;
    
    
leave:
    if (err) isds_envelope_free(envelope);
    xmlXPathFreeObject(result);
    return err;
}


/* Convert XSD:tStateChangesRecord type XML tree into structure
 * @context is ISDS context
 * @changed_status is automatically reallocated message state change structure
 * @xpath_ctx is XPath context with current node as element of
 * XSD:tStateChangesRecord type
 * In case of error @changed_status will be freed. */
static isds_error extract_StateChangesRecord(struct isds_ctx *context,
        struct isds_message_status_change **changed_status,
        xmlXPathContextPtr xpath_ctx) {
    isds_error err = IE_SUCCESS;
    xmlXPathObjectPtr result = NULL;
    unsigned long int *unumber = NULL;
    char *string = NULL;

    if (!context) return IE_INVALID_CONTEXT;
    if (!changed_status) return IE_INVAL;
    isds_message_status_change_free(changed_status);
    if (!xpath_ctx) return IE_INVAL;


    *changed_status = calloc(1, sizeof(**changed_status));
    if (!*changed_status) {
        err = IE_NOMEM;
        goto leave;
    }


    /* Extract tGetStateChangesInput data */
    EXTRACT_STRING("isds:dmID", (*changed_status)->dmID);

    /* dmEventTime is mandatory */
    EXTRACT_STRING("isds:dmEventTime", string);
    if (string) {
        err = timestring2timeval((xmlChar *) string,
                &((*changed_status)->time));
        if (err) {
            char *string_locale = _isds_utf82locale(string);
            if (err == IE_DATE) err = IE_ISDS;
            isds_printf_message(context,
                    _("Could not convert dmEventTime as ISO time: %s"),
                    string_locale);
            free(string_locale);
            goto leave;
        }
        zfree(string);
    }

    /* dmMessageStatus element is mandatory */
    EXTRACT_ULONGINT("isds:dmMessageStatus", unumber, 0);
    if (!unumber) {
        isds_log_message(context,
                _("Missing mandatory isds:dmMessageStatus integer"));
        err = IE_ISDS;
        goto leave;
    }
    err = uint2isds_message_status(context, unumber,
            &((*changed_status)->dmMessageStatus));
    if (err) {
        if (err == IE_ENUM) err = IE_ISDS;
        goto leave;
    }
    zfree(unumber);

    
leave:
    free(unumber);
    free(string);
    if (err) isds_message_status_change_free(changed_status);
    xmlXPathFreeObject(result);
    return err;
}
#endif /* HAVE_LIBCURL */


/* Find and convert isds:dmHash XML tree into structure
 * @context is ISDS context
 * @envelope is automatically reallocated message hash structure
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
            char *string_locale = _isds_utf82locale(string);
            isds_printf_message(context, _("Unsupported hash algorithm: %s"),
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
    (*hash)->length = _isds_b64decode(string, &((*hash)->value));
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


/* Find and append isds:dmQTimestamp XML tree into envelope.
 * Because one service is allowed to miss time-stamp content, and we think
 * other could too (flaw in specification), this function is deliberated and
 * will not fail (i.e. will return IE_SUCCESS), if time-stamp is missing.
 * @context is ISDS context
 * @envelope is automatically allocated envelope structure
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
        isds_log(ILF_ISDS, ILL_INFO, _("Missing dmQTimestamp element content\n"));
        goto leave;
    }
    (*envelope)->timestamp_length =
        _isds_b64decode(string, &((*envelope)->timestamp));
    if ((*envelope)->timestamp_length == (size_t) -1) {
        isds_printf_message(context,
                _("Error while Base64-decoding time stamp value"));
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
 * It does not store serialized XML tree into message->raw.
 * It does store (pointer to) parsed XML tree into message->xml if needed.
 * @context is ISDS context
 * @include_documents Use true if documents must be extracted
 * (tReturnedMessage XSD type), use false if documents shall be omitted
 * (tReturnedMessageEnvelope).
 * @message is automatically reallocated message structure
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
        struct isds_list *item;

        /* Extract dmFiles */
        err = move_xpathctx_to_child(context, BAD_CAST "isds:dmFiles",
                xpath_ctx);
        if (err == IE_NOEXIST || err == IE_NOTUNIQ) {
            err = IE_ISDS; goto leave;
        }
        if (err) { err = IE_ERROR; goto leave; }
        err = extract_documents(context, &((*message)->documents), xpath_ctx);
        if (err) goto leave;

        /* Store xmlDoc of this message if needed */
        /* Only if we got a XML document in all the documents. */
        for (item = (*message)->documents; item; item = item->next) {
            if (item->data && ((struct isds_document *)item->data)->is_xml) {
                (*message)->xml = xpath_ctx->doc;
                break;
            }
        }
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
 * @event is automatically reallocated message event structure
 * @xpath_ctx is XPath context with current node as isds:dmEvent
 * In case of error @event will be freed. */
static isds_error extract_event(struct isds_ctx *context,
        struct isds_event **event, xmlXPathContextPtr xpath_ctx) {
    isds_error err = IE_SUCCESS;
    xmlXPathObjectPtr result = NULL;
    xmlNodePtr event_node;
    char *string = NULL;

    if (!context) return IE_INVALID_CONTEXT;
    if (!event) return IE_INVAL;
    isds_event_free(event);
    if (!xpath_ctx) return IE_INVAL;
    event_node = xpath_ctx->node;

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
            char *string_locale = _isds_utf82locale(string);
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
 * @events is automatically reallocated list of event structures
 * @xpath_ctx is XPath context with current node as tEventsArray
 * In case of error @events will be freed. */
static isds_error extract_events(struct isds_ctx *context,
        struct isds_list **events, xmlXPathContextPtr xpath_ctx) {
    isds_error err = IE_SUCCESS;
    xmlXPathObjectPtr result = NULL;
    xmlNodePtr events_node;
    struct isds_list *event, *prev_event = NULL;

    if (!context) return IE_INVALID_CONTEXT;
    if (!events) return IE_INVAL;
    if (!xpath_ctx) return IE_INVAL;
    events_node = xpath_ctx->node;

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


#if HAVE_LIBCURL
/* Insert Base64 encoded data as element with text child.
 * @context is session context
 * @parent is XML node to append @element with @data as child
 * @ns is XML namespace of @element, use NULL to inherit from @parent
 * @element is UTF-8 encoded name of new element
 * @data is bit stream to encode into @element
 * @length is size of @data in bytes
 * @return standard error code and fill long error message if needed */
static isds_error insert_base64_encoded_string(struct isds_ctx *context,
        xmlNodePtr parent, const xmlNsPtr ns, const char *element,
        const void *data, size_t length) {
    isds_error err = IE_SUCCESS;
    xmlNodePtr node;

    if (!context) return IE_INVALID_CONTEXT;
    if (!data && length > 0) return IE_INVAL;
    if (!parent || !element) return IE_INVAL;

    xmlChar *base64data = NULL;
    base64data = (xmlChar *) _isds_b64encode(data, length);
    if (!base64data) {
        isds_printf_message(context,
                ngettext("Not enough memory to encode %zd byte into Base64",
                    "Not enough memory to encode %zd bytes into Base64",
                    length),
                length);
        err = IE_NOMEM;
        goto leave;
    }
    INSERT_STRING_WITH_NS(parent, ns, element, base64data);

leave:
    free(base64data);
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


    /* Insert content (body) of the document. */
    if (document->is_xml) {
        /* XML document requested */

        /* Allocate new dmXMLContent */
        xmlNodePtr xmlcontent = xmlNewNode(file->ns, BAD_CAST "dmXMLContent");
        if (!xmlcontent) {
            isds_printf_message(context,
                    _("Could not allocate dmXMLContent element"));
            err = IE_ERROR;
            goto leave;
        }
        /* Append it */
        node = xmlAddChild(file, xmlcontent);
        if (!node) {
            xmlFreeNode(xmlcontent); xmlcontent = NULL;
            isds_printf_message(context,
                    _("Could not add dmXMLContent child to %s element"),
                    file->name);
            err = IE_ERROR;
            goto leave;
        }

        /* Copy non-empty node list */
        if (document->xml_node_list) {
            xmlNodePtr content = xmlDocCopyNodeList(node->doc,
                    document->xml_node_list);
            if (!content) {
                isds_printf_message(context,
                        _("Not enough memory to copy XML document"));
                err = IE_NOMEM;
                goto leave;
            }

            if (!xmlAddChildList(node, content)) {
                xmlFreeNodeList(content);
                isds_printf_message(context,
                        _("Error while adding XML document into dmXMLContent"));
                err = IE_XML;
                goto leave;
            }
            /* XXX: We cannot free the content here because it's part of node's
             * document since now. It will be freed with it automatically. */
        }
    } else {
        /* Binary document requested */
        err = insert_base64_encoded_string(context, file, NULL, "dmEncodedContent",
                document->data, document->data_length);
        if (err) goto leave;
    }

leave:
    return err;
}


/* Append XSD tMStatus XML tree into isds_message_copy structure.
 * The copy must be preallocated, the date are just appended into structure.
 * @context is ISDS context
 * @copy is message copy structure
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
            copy->dmStatus = _isds_astrcat3(code, ": ", message);
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
 * @context is session context
 * @approval is libisds structure with approval description. NULL is
 * acceptable.
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
 * @response is reallocated server SOAP body response as XML document
 * @raw_response is reallocated bit stream with response body. Use
 * NULL if you don't care
 * @raw_response_length is size of @raw_response in bytes
 * @code is reallocated ISDS status code
 * @status_message is reallocated ISDS status message
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
    zfree(*code);
    zfree(*status_message);


    /* Check if connection is established
     * TODO: This check should be done downstairs. */
    if (!context->curl) return IE_CONNECTION_CLOSED;

    service_name_locale = _isds_utf82locale((char*)service_name);
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
    err = _isds(context, SERVICE_DB_ACCESS, request, response,
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
        char *code_locale = _isds_utf82locale((char*) *code);
        char *status_message_locale = 
            _isds_utf82locale((char*) *status_message);
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
#endif


/* Get data about logged in user and his box.
 * @context is session context
 * @db_owner_info is reallocated box owner description. It will be freed on
 * error.
 * @return error code from lower layer, context message will be set up
 * appropriately. */
isds_error isds_GetOwnerInfoFromLogin(struct isds_ctx *context,
        struct isds_DbOwnerInfo **db_owner_info) {
    isds_error err = IE_SUCCESS;
#if HAVE_LIBCURL
    xmlDocPtr response = NULL;
    xmlChar *code = NULL, *message = NULL;
    xmlXPathContextPtr xpath_ctx = NULL;
    xmlXPathObjectPtr result = NULL;
    char *string = NULL;
#endif

    if (!context) return IE_INVALID_CONTEXT;
    zfree(context->long_message);
    if (!db_owner_info) return IE_INVAL;
    isds_DbOwnerInfo_free(db_owner_info);

#if HAVE_LIBCURL
    /* Check if connection is established */
    if (!context->curl) return IE_CONNECTION_CLOSED;


    /* Do request and check for success */
    err = build_send_check_dbdummy_request(context,
            BAD_CAST "GetOwnerInfoFromLogin",
            &response, NULL, NULL, &code, &message);
    if (err) goto leave;


    /* Extract data */
    /* Prepare structure */
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
    if (_isds_register_namespaces(xpath_ctx, MESSAGE_NS_UNSIGNED)) {
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
#else /* not HAVE_LIBCURL */
    err = IE_NOTSUP;
#endif

    return err;
}


/* Get data about logged in user. */
isds_error isds_GetUserInfoFromLogin(struct isds_ctx *context,
        struct isds_DbUserInfo **db_user_info) {
    isds_error err = IE_SUCCESS;
#if HAVE_LIBCURL
    xmlDocPtr response = NULL;
    xmlChar *code = NULL, *message = NULL;
    xmlXPathContextPtr xpath_ctx = NULL;
    xmlXPathObjectPtr result = NULL;
#endif

    if (!context) return IE_INVALID_CONTEXT;
    zfree(context->long_message);
    if (!db_user_info) return IE_INVAL;
    isds_DbUserInfo_free(db_user_info);

#if HAVE_LIBCURL
    /* Check if connection is established */
    if (!context->curl) return IE_CONNECTION_CLOSED;


    /* Do request and check for success */
    err = build_send_check_dbdummy_request(context,
            BAD_CAST "GetUserInfoFromLogin",
            &response, NULL, NULL, &code, &message);
    if (err) goto leave;


    /* Extract data */
    /* Prepare structure */
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
    if (_isds_register_namespaces(xpath_ctx, MESSAGE_NS_UNSIGNED)) {
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
#else /* not HAVE_LIBCURL */
    err = IE_NOTSUP;
#endif

    return err;
}


/* Get expiration time of current password
 * @context is session context
 * @expiration is automatically reallocated time when password expires. If
 * password expiration is disabled, NULL will be returned. In case of error
 * it will be nulled too. */
isds_error isds_get_password_expiration(struct isds_ctx *context,
        struct timeval **expiration) {
    isds_error err = IE_SUCCESS;
#if HAVE_LIBCURL
    xmlDocPtr response = NULL;
    xmlChar *code = NULL, *message = NULL;
    xmlXPathContextPtr xpath_ctx = NULL;
    xmlXPathObjectPtr result = NULL;
    char *string = NULL;
#endif

    if (!context) return IE_INVALID_CONTEXT;
    zfree(context->long_message);
    if (!expiration) return IE_INVAL;
    zfree(*expiration);

#if HAVE_LIBCURL
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
    if (_isds_register_namespaces(xpath_ctx, MESSAGE_NS_UNSIGNED)) {
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
    if (string) {
        /* And convert it if any returned. Otherwise expiration is disabled. */
        err = timestring2timeval((xmlChar *) string, expiration);
        if (err) {
            char *string_locale = _isds_utf82locale(string);
            if (err == IE_DATE) err = IE_ISDS;
            isds_printf_message(context,
                    _("Could not convert pswExpDate as ISO time: %s"),
                    string_locale);
            free(string_locale);
            goto leave;
        }
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
#else /* not HAVE_LIBCURL */
    err = IE_NOTSUP;
#endif

    return err;
}


#if HAVE_LIBCURL
/* Request delivering new TOTP code from ISDS through side channel before
 * changing password.
 * @context is session context
 * @password is current password.
 * @otp auxiliary data required, returns fine grade resolution of OTP procedure.
 * Please note the @otp argument must have TOTP OTP method. See isds_login()
 * function for more details.
 * @refnumber is reallocated serial number of request assigned by ISDS. Use
 * NULL, if you don't care.
 * @return IE_SUCCESS, if new TOTP code has been sent. Or returns appropriate
 * error code. */
static isds_error _isds_request_totp_code(struct isds_ctx *context,
        const char *password, struct isds_otp *otp, char **refnumber) {
    isds_error err = IE_SUCCESS;
    char *saved_url = NULL; /* No copy */
#if HAVE_CURL_REAUTHORIZATION_BUG
    CURL *saved_curl = NULL; /* No copy */
#endif
    xmlNsPtr isds_ns = NULL;
    xmlNodePtr request = NULL;
    xmlDocPtr response = NULL;
    xmlChar *code = NULL, *message = NULL;
    const xmlChar *codes[] = {
        BAD_CAST "2300",
        BAD_CAST "2301",
        BAD_CAST "2302"
    };
    const char *meanings[] = {
        N_("Unexpected error"),
        N_("One-time code cannot be re-send faster than once a 30 seconds"),
        N_("One-time code could not been sent. Try later again.")
    };
    const isds_otp_resolution resolutions[] = {
        OTP_RESOLUTION_UNKNOWN,
        OTP_RESOLUTION_TO_FAST,
        OTP_RESOLUTION_TOTP_NOT_SENT
    };

    if (NULL == context) return IE_INVALID_CONTEXT;
    zfree(context->long_message);
    if (NULL == password) {
        isds_log_message(context,
                _("Second argument (password) of isds_change_password() "
                    "is NULL"));
        return IE_INVAL;
    }

    /* Check if connection is established
     * TODO: This check should be done downstairs. */
    if (!context->curl) return IE_CONNECTION_CLOSED;

    if (!context->otp) {
        isds_log_message(context, _("This function requires OTP-authenticated "
                    "context"));
        return IE_INVALID_CONTEXT;
    }
    if (NULL == otp) {
        isds_log_message(context, _("If one-time password authentication "
                    "method is in use, requesting new OTP code requires "
                    "one-time credentials argument either"));
        return IE_INVAL;
    }
    if (otp->method != OTP_TIME) {
        isds_log_message(context, _("Requesting new time-based OTP code from "
                    "server requires one-time password authentication "
                    "method"));
        return IE_INVAL;
    }
    if (otp->otp_code != NULL) {
        isds_log_message(context, _("Requesting new time-based OTP code from "
                    "server requires undefined OTP code member in "
                    "one-time credentials argument"));
        return IE_INVAL;
    }


    /* Build request */
    request = xmlNewNode(NULL, BAD_CAST "SendSMSCode");
    if (!request) {
        isds_log_message(context, _("Could not build SendSMSCode request"));
        return IE_ERROR;
    }
    isds_ns = xmlNewNs(request, BAD_CAST OISDS_NS, NULL);
    if(!isds_ns) {
        isds_log_message(context, _("Could not create ISDS name space"));
        xmlFreeNode(request);
        return IE_ERROR;
    }
    xmlSetNs(request, isds_ns);

    /* Change URL temporarily for sending this request only */
    {
        char *new_url = NULL;
        if ((err = _isds_build_url_from_context(context,
                    "%.*sasws/changePassword", &new_url))) {
            goto leave;
        }
        saved_url = context->url;
        context->url = new_url;
    }

    /* Store credentials for sending this request only */
    context->otp_credentials = otp;
    _isds_discard_credentials(context, 0);
    if ((err = _isds_store_credentials(context, context->saved_username,
                password, NULL))) {
        _isds_discard_credentials(context, 0);
        goto leave;
    }
#if HAVE_CURL_REAUTHORIZATION_BUG
    saved_curl = context->curl;
    context->curl = curl_easy_init();
    if (NULL == context->curl) {
        err = IE_ERROR;
        goto leave;
    }
    if (context->timeout) {
        err = isds_set_timeout(context, context->timeout);
        if (err) goto leave;
    }
#endif

    isds_log(ILF_ISDS, ILL_DEBUG, _("Sending SendSMSCode request to ISDS\n"));

    /* Sent request */
    err = _isds(context, SERVICE_ASWS, request, &response, NULL, NULL);
   
    /* Remove temporal credentials */
    _isds_discard_credentials(context, 0);
    /* Detach pointer to OTP credentials from context */
    context->otp_credentials = NULL;
    /* Keep context->otp true to keep signaling this is OTP session */

    /* Destroy request */
    xmlFreeNode(request); request = NULL;

    if (err) {
        isds_log(ILF_ISDS, ILL_DEBUG,
                _("Processing ISDS response on SendSMSCode request failed\n"));
        goto leave;
    }

    /* Check for response status */
    err = isds_response_status(context, SERVICE_ASWS, response,
            &code, &message, (xmlChar **)refnumber);
    if (err) {
        isds_log(ILF_ISDS, ILL_DEBUG,
                _("ISDS response on SendSMSCode request is missing "
                    "status\n"));
        goto leave;
    }

    /* Check for error */
    if (xmlStrcmp(code, BAD_CAST "0000")) {
        char *code_locale = _isds_utf82locale((char*)code);
        char *message_locale = _isds_utf82locale((char*)message);
        size_t i;
        isds_log(ILF_ISDS, ILL_DEBUG,
                _("Server refused to send new code on SendSMSCode "
                    "request (code=%s, message=%s)\n"),
                code_locale, message_locale);

        /* Check for known error codes */
        for (i = 0; i < sizeof(codes)/sizeof(*codes); i++) {
            if (!xmlStrcmp(code, codes[i])) break;
        }
        if (i < sizeof(codes)/sizeof(*codes)) {
            isds_log_message(context, _(meanings[i]));
            /* Mimic otp->resolution according to the code, specification does
             * prescribe OTP header to be available. */
            if (OTP_RESOLUTION_SUCCESS == otp->resolution &&
                    OTP_RESOLUTION_UNKNOWN != resolutions[i])
                otp->resolution = resolutions[i];
        } else
            isds_log_message(context, message_locale);

        free(code_locale);
        free(message_locale);

        err = IE_ISDS;
        goto leave;
    }
    
    /* Otherwise new code sent successfully */
    /* Mimic otp->resolution according to the code, specification does
     * prescribe OTP header to be available. */
    if (OTP_RESOLUTION_SUCCESS == otp->resolution)
        otp->resolution = OTP_RESOLUTION_TOTP_SENT;

leave:
    if (NULL != saved_url) {
        /* Revert URL to original one */
        zfree(context->url);
        context->url = saved_url;
    }
#if HAVE_CURL_REAUTHORIZATION_BUG
    if (NULL != saved_curl) {
        if (context->curl != NULL) curl_easy_cleanup(context->curl);
        context->curl = saved_curl;
    }
#endif

    free(code);
    free(message);
    xmlFreeDoc(response);
    xmlFreeNode(request);

    if (!err)
        isds_log(ILF_ISDS, ILL_DEBUG,
                _("New OTP code has been sent successfully on SendSMSCode "
                    "request.\n"));
    return err;
}


/* Convert response status code to isds_error code and set long message
 * @context is context to save long message to
 * @map is mapping from codes to errors and messages. Pass NULL for generic
 * handling.
 * @code is status code to translate
 * @message is non-localized status message to put into long message in case
 * of uknown error. It can be NULL if server did not provide any.
 * @return desired isds_error or IE_ISDS for unknown code or IE_INVAL for
 * invalid invocation. */
static isds_error statuscode2isds_error(struct isds_ctx *context,
        const struct code_map_isds_error *map,
        const xmlChar *code, const xmlChar *message) {
    if (NULL == code) {
        isds_log_message(context,
                _("NULL status code passed to statuscode2isds_error()"));
        return IE_INVAL;
    }

    if (NULL != map) {
        /* Check for known error codes */
        for (int i=0; map->codes[i] != NULL; i++) {
            if (!xmlStrcmp(code, map->codes[i])) {
                isds_log_message(context, _(map->meanings[i]));
                return map->errors[i];
            }
        }
    }

    /* Other error */
    if (xmlStrcmp(code, BAD_CAST "0000")) {
        char *message_locale = _isds_utf82locale((char*)message);
        if (NULL == message_locale)
            isds_log_message(context, _("ISDS server returned unknown error"));
        else
            isds_log_message(context, message_locale);
        free(message_locale);
        return IE_ISDS;
    }

    return IE_SUCCESS;
}
#endif


/* Change user password in ISDS.
 * User must supply old password, new password will takes effect after some
 * time, current session can continue. Password must fulfill some constraints.
 * @context is session context
 * @old_password is current password.
 * @new_password is requested new password
 * @otp auxiliary data required if one-time password authentication is in use,
 * defines OTP code (if known) and returns fine grade resolution of OTP
 * procedure. Pass NULL, if one-time password authentication is not needed.
 * Please note the @otp argument must match OTP method used at log-in time. See
 * isds_login() function for more details.
 * @refnumber is reallocated serial number of request assigned by ISDS. Use
 * NULL, if you don't care.
 * @return IE_SUCCESS, if password has been changed. Or returns appropriate
 * error code. It can return IE_PARTIAL_SUCCESS if OTP is in use and server is
 * awaiting OTP code that has been delivered by side channel to the user. */
isds_error isds_change_password(struct isds_ctx *context,
        const char *old_password, const char *new_password,
        struct isds_otp *otp, char **refnumber) {
    isds_error err = IE_SUCCESS;
#if HAVE_LIBCURL
    char *saved_url = NULL; /* No copy */
#if HAVE_CURL_REAUTHORIZATION_BUG
    CURL *saved_curl = NULL; /* No copy */
#endif
    xmlNsPtr isds_ns = NULL;
    xmlNodePtr request = NULL, node;
    xmlDocPtr response = NULL;
    xmlChar *code = NULL, *message = NULL;
    const xmlChar *codes[] = {
        BAD_CAST "1066",
        BAD_CAST "1067",
        BAD_CAST "1079",
        BAD_CAST "1080",
        BAD_CAST "1081",
        BAD_CAST "1082",
        BAD_CAST "1083",
        BAD_CAST "1090",
        BAD_CAST "1091",
        BAD_CAST "2300",
        BAD_CAST "9204"
    };
    const char *meanings[] = {
        N_("Password length must be between 8 and 32 characters"),
        N_("Password cannot be reused"), /* Server does not distinguish 1067
                                            and 1091 on ChangePasswordOTP */
        N_("Password contains forbidden character"),
        N_("Password must contain at least one upper-case letter, "
                "one lower-case, and one digit"),
        N_("Password cannot contain sequence of three identical characters"),
        N_("Password cannot contain user identifier"),
        N_("Password is too simmple"),
        N_("Old password is not valid"),
        N_("Password cannot be reused"),
        N_("Unexpected error"),
        N_("LDAP update error")
    };
#endif

    if (!context) return IE_INVALID_CONTEXT;
    zfree(context->long_message);
    if (NULL != refnumber)
        zfree(*refnumber);
    if (NULL == old_password) {
        isds_log_message(context,
                _("Second argument (old password) of isds_change_password() "
                    "is NULL"));
        return IE_INVAL;
    }
    if (NULL == otp && NULL == new_password) {
        isds_log_message(context,
                _("Third argument (new password) of isds_change_password() "
                    "is NULL"));
        return IE_INVAL;
    }

#if HAVE_LIBCURL
    /* Check if connection is established
     * TODO: This check should be done downstairs. */
    if (!context->curl) return IE_CONNECTION_CLOSED;

    if (context->otp && NULL == otp) {
        isds_log_message(context, _("If one-time password authentication "
                    "method is in use, changing password requires one-time "
                    "credentials either"));
        return IE_INVAL;
    }

    /* Build ChangeISDSPassword request */
    request = xmlNewNode(NULL, (NULL == otp) ? BAD_CAST "ChangeISDSPassword" :
            BAD_CAST "ChangePasswordOTP");
    if (!request) {
        isds_log_message(context, (NULL == otp) ?
                _("Could not build ChangeISDSPassword request") :
                _("Could not build ChangePasswordOTP request"));
        return IE_ERROR;
    }
    isds_ns = xmlNewNs(request,
            (NULL == otp) ? BAD_CAST ISDS_NS : BAD_CAST OISDS_NS,
            NULL);
    if(!isds_ns) {
        isds_log_message(context, _("Could not create ISDS name space"));
        xmlFreeNode(request);
        return IE_ERROR;
    }
    xmlSetNs(request, isds_ns);

    INSERT_STRING(request, "dbOldPassword", old_password);
    INSERT_STRING(request, "dbNewPassword", new_password);

    if (NULL != otp) {
        otp->resolution = OTP_RESOLUTION_UNKNOWN;
        switch (otp->method) {
            case OTP_HMAC: 
                isds_log(ILF_SEC, ILL_INFO,
                        _("Selected authentication method: "
                            "HMAC-based one-time password\n"));
                INSERT_STRING(request, "dbOTPType", BAD_CAST "HOTP");
                break;
            case OTP_TIME: 
                isds_log(ILF_SEC, ILL_INFO,
                        _("Selected authentication method: "
                            "Time-based one-time password\n"));
                INSERT_STRING(request, "dbOTPType", BAD_CAST "TOTP");
                if (otp->otp_code == NULL) {
                    isds_log(ILF_SEC, ILL_INFO,
                            _("OTP code has not been provided by "
                                "application, requesting server for "
                                "new one.\n"));
                    err = _isds_request_totp_code(context, old_password, otp,
                            refnumber);
                    if (err == IE_SUCCESS) err = IE_PARTIAL_SUCCESS;
                    goto leave;

                } else {
                    isds_log(ILF_SEC, ILL_INFO,
                            _("OTP code has been provided by "
                                "application, not requesting server "
                                "for new one.\n"));
                }
                break;
            default:
                isds_log_message(context,
                        _("Unknown one-time password authentication "
                            "method requested by application"));
                err = IE_ENUM;
                goto leave;
        }

        /* Change URL temporarily for sending this request only */
        {
            char *new_url = NULL;
            if ((err = _isds_build_url_from_context(context,
                        "%.*sasws/changePassword", &new_url))) {
                goto leave;
            }
            saved_url = context->url;
            context->url = new_url;
        }

        /* Store credentials for sending this request only */
        context->otp_credentials = otp;
        _isds_discard_credentials(context, 0);
        if ((err = _isds_store_credentials(context, context->saved_username,
                    old_password, NULL))) {
            _isds_discard_credentials(context, 0);
            goto leave;
        }
#if HAVE_CURL_REAUTHORIZATION_BUG
        saved_curl = context->curl;
        context->curl = curl_easy_init();
        if (NULL == context->curl) {
            err = IE_ERROR;
            goto leave;
        }
        if (context->timeout) {
            err = isds_set_timeout(context, context->timeout);
            if (err) goto leave;
        }
#endif
    }

    isds_log(ILF_ISDS, ILL_DEBUG, (NULL == otp) ?
            _("Sending ChangeISDSPassword request to ISDS\n") :
            _("Sending ChangePasswordOTP request to ISDS\n"));

    /* Sent request */
    err = _isds(context, (NULL == otp) ? SERVICE_DB_ACCESS : SERVICE_ASWS,
            request, &response, NULL, NULL);
   
    if (otp) {
        /* Remove temporal credentials */
        _isds_discard_credentials(context, 0);
        /* Detach pointer to OTP credentials from context */
        context->otp_credentials = NULL;
        /* Keep context->otp true to keep signaling this is OTP session */
    }

    /* Destroy request */
    xmlFreeNode(request); request = NULL;

    if (err) {
        isds_log(ILF_ISDS, ILL_DEBUG, (NULL == otp) ?
                _("Processing ISDS response on ChangeISDSPassword "
                    "request failed\n") :
                _("Processing ISDS response on ChangePasswordOTP "
                    "request failed\n"));
        goto leave;
    }

    /* Check for response status */
    err = isds_response_status(context,
            (NULL == otp) ? SERVICE_DB_ACCESS : SERVICE_ASWS, response,
            &code, &message, (xmlChar **)refnumber);
    if (err) {
        isds_log(ILF_ISDS, ILL_DEBUG, (NULL == otp) ?
                _("ISDS response on ChangeISDSPassword request is missing "
                    "status\n") :
                _("ISDS response on ChangePasswordOTP request is missing "
                    "status\n"));
        goto leave;
    }

    /* Check for known error codes */
    for (size_t i = 0; i < sizeof(codes)/sizeof(*codes); i++) {
        if (!xmlStrcmp(code, codes[i])) {
            char *code_locale = _isds_utf82locale((char*)code);
            char *message_locale = _isds_utf82locale((char*)message);
            isds_log(ILF_ISDS, ILL_DEBUG, (NULL == otp) ?
                    _("Server refused to change password on ChangeISDSPassword "
                        "request (code=%s, message=%s)\n") :
                    _("Server refused to change password on ChangePasswordOTP "
                        "request (code=%s, message=%s)\n"),
                    code_locale, message_locale);
            free(code_locale);
            free(message_locale);
            isds_log_message(context, _(meanings[i]));
            err = IE_INVAL;
            goto leave;
        }
    }

    /* Other error */
    if (xmlStrcmp(code, BAD_CAST "0000")) {
        char *code_locale = _isds_utf82locale((char*)code);
        char *message_locale = _isds_utf82locale((char*)message);
        isds_log(ILF_ISDS, ILL_DEBUG, (NULL == otp) ?
                _("Server refused to change password on ChangeISDSPassword "
                    "request (code=%s, message=%s)\n") :
                _("Server refused to change password on ChangePasswordOTP "
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
    if (NULL != saved_url) {
        /* Revert URL to original one */
        zfree(context->url);
        context->url = saved_url;
    }
#if HAVE_CURL_REAUTHORIZATION_BUG
    if (NULL != saved_curl) {
        if (context->curl != NULL) curl_easy_cleanup(context->curl);
        context->curl = saved_curl;
    }
#endif

    free(code);
    free(message);
    xmlFreeDoc(response);
    xmlFreeNode(request);

    if (!err)
        isds_log(ILF_ISDS, ILL_DEBUG, (NULL == otp) ?
                _("Password changed successfully on ChangeISDSPassword "
                    "request.\n") :
                _("Password changed successfully on ChangePasswordOTP "
                    "request.\n"));
#else /* not HAVE_LIBCURL */
    err = IE_NOTSUP;
#endif

    return err;
}


#if HAVE_LIBCURL
/* Generic middle part with request sending and response check.
 * It sends prepared request and checks for error code.
 * @context is ISDS session context.
 * @service is ISDS service handler
 * @service_name is name in scope of given @service
 * @request is XML tree with request. Will be freed to save memory.
 * @response is XML document outputting ISDS response.
 * @refnumber is reallocated serial number of request assigned by ISDS. Use
 * @map is mapping from status code to library error. Pass NULL if no special
 * handling is requested.
 * NULL, if you don't care. */
static isds_error send_destroy_request_check_response(
        struct isds_ctx *context,
        const isds_service service, const xmlChar *service_name, 
        xmlNodePtr *request, xmlDocPtr *response, xmlChar **refnumber,
        const struct code_map_isds_error *map) {
    isds_error err = IE_SUCCESS;
    char *service_name_locale = NULL;
    xmlChar *code = NULL, *message = NULL;


    if (!context) return IE_INVALID_CONTEXT;
    if (!service_name || *service_name == '\0' || !request || !*request ||
            !response)
        return IE_INVAL;

    /* Check if connection is established
     * TODO: This check should be done downstairs. */
    if (!context->curl) return IE_CONNECTION_CLOSED;

    service_name_locale = _isds_utf82locale((char*) service_name);
    if (!service_name_locale) {
        err = IE_NOMEM;
        goto leave;
    }

    isds_log(ILF_ISDS, ILL_DEBUG, _("Sending %s request to ISDS\n"),
            service_name_locale);

    /* Send request */
    err = _isds(context, service, *request, response, NULL, NULL);
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

    err = statuscode2isds_error(context, map, code, message);

    /* Request processed, but server failed */
    if (xmlStrcmp(code, BAD_CAST "0000")) {
        char *code_locale = _isds_utf82locale((char*) code);
        char *message_locale = _isds_utf82locale((char*) message);
        isds_log(ILF_ISDS, ILL_DEBUG,
                    _("Server refused %s request (code=%s, message=%s)\n"),
                service_name_locale, code_locale, message_locale);
        free(code_locale);
        free(message_locale);
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
            service, service_name, request, &response, refnumber, NULL);

    xmlFreeDoc(response);

    if (*request) {
        xmlFreeNode(*request);
        *request = NULL;
    }

    if (!err) {
        char *service_name_locale = _isds_utf82locale((char *) service_name);
        isds_log(ILF_ISDS, ILL_DEBUG,
                _("%s request processed by server successfully.\n"),
                service_name_locale);
        free(service_name_locale);
    }

    return err;
}


/* Insert isds_credentials_delivery structure into XML request if not NULL
 * @context is session context
 * @credentials_delivery is NULL if to omit, non-NULL to signal on-line
 * credentials delivery. The email field is passed.
 * @parent is XML element where to insert */
static isds_error insert_credentials_delivery(struct isds_ctx *context,
        const struct isds_credentials_delivery *credentials_delivery,
        xmlNodePtr parent) {
    isds_error err = IE_SUCCESS;
    xmlNodePtr node;

    if (!context) return IE_INVALID_CONTEXT;
    if (!parent) return IE_INVAL;
        
    if (credentials_delivery) {
        /* Following elements are valid only for services:
         * NewAccessData, AddDataBoxUser, CreateDataBox */
        INSERT_SCALAR_BOOLEAN(parent, "dbVirtual", 1);
        INSERT_STRING(parent, "email", credentials_delivery->email);
    }

leave:
    return err;
}


/* Extract credentials delivery from ISDS response.
 * @context is session context
 * @credentials_delivery is pointer to valid structure to fill in returned
 * user's password (and new log-in name). If NULL, do not extract the data.
 * @response is pointer to XML document with ISDS response
 * @request_name is UTF-8 encoded name of ISDS service the @response it to.
 * @return IE_SUCCESS even if new user name has not been found because it's not
 * clear whether it's returned always. */ 
static isds_error extract_credentials_delivery(struct isds_ctx *context,
        struct isds_credentials_delivery *credentials_delivery,
        xmlDocPtr response, const char *request_name) {
    isds_error err = IE_SUCCESS;
    xmlXPathContextPtr xpath_ctx = NULL;
    xmlXPathObjectPtr result = NULL;
    char *xpath_query = NULL;

    if (!context) return IE_INVALID_CONTEXT;
    if (credentials_delivery) {
        zfree(credentials_delivery->token);
        zfree(credentials_delivery->new_user_name);
    }
    if (!response || !request_name || !*request_name) return IE_INVAL;


    /* Extract optional token */
    if (credentials_delivery) {
        xpath_ctx = xmlXPathNewContext(response);
        if (!xpath_ctx) {
            err = IE_ERROR;
            goto leave;
        }
        if (_isds_register_namespaces(xpath_ctx, MESSAGE_NS_UNSIGNED)) {
            err = IE_ERROR;
            goto leave;
        }

        /* Verify root element */
        if (-1 == isds_asprintf(&xpath_query, "/isds:%sResponse",
                    request_name)) {
            err = IE_NOMEM;
            goto leave;
        }
        result = xmlXPathEvalExpression(BAD_CAST xpath_query, xpath_ctx);
        if (!result) {
            err = IE_ERROR;
            goto leave;
        }
        if (!xmlXPathNodeSetIsEmpty(result->nodesetval)) {
            char *request_name_locale = _isds_utf82locale(request_name);
            isds_log(ILF_ISDS, ILL_WARNING,
                    _("Wrong element in ISDS response for %s request "
                        "while extracting credentials delivery details\n"),
                    request_name_locale);
            free(request_name_locale);
            err = IE_ERROR;
            goto leave;
        }
        xpath_ctx->node = result->nodesetval->nodeTab[0];


        /* XXX: isds:dbUserID is provided only on NewAccessData. Leave it
         * optional. */
        EXTRACT_STRING("isds:dbUserID", credentials_delivery->new_user_name);

        EXTRACT_STRING("isds:dbAccessDataId", credentials_delivery->token);
        if (!credentials_delivery->token) {
            char *request_name_locale = _isds_utf82locale(request_name);
            isds_log(ILF_ISDS, ILL_ERR,
                    _("ISDS did not return token on %s request "
                        "even if requested\n"), request_name_locale);
            free(request_name_locale);
            err = IE_ERROR;
        }
    }

leave:
    free(xpath_query);
    xmlXPathFreeObject(result);
    xmlXPathFreeContext(xpath_ctx);

    return err;
}


/* Build XSD:tCreateDBInput request type for box creating.
 * @context is session context
 * @request outputs built XML tree
 * @service_name is request name of SERVICE_DB_MANIPULATION service
 * @box is box description to create including single primary user (in case of
 * FO box type)
 * @users is list of struct isds_DbUserInfo (primary users in case of non-FO
 * box, or contact address of PFO box owner)
 * @former_names is optional former name of box owner. Pass NULL if otherwise.
 * @upper_box_id is optional ID of supper box if currently created box is
 * subordinated.
 * @ceo_label is optional title of OVM box owner (e.g. mayor); NULL, if you
 * don't care.
 * @credentials_delivery is valid pointer if ISDS should return token that box
 * owner can use to obtain his new credentials in on-line way. Then valid email
 * member value should be supplied.
 * @approval is optional external approval of box manipulation */
static isds_error build_CreateDBInput_request(struct isds_ctx *context,
        xmlNodePtr *request, const xmlChar *service_name,
        const struct isds_DbOwnerInfo *box, const struct isds_list *users,
        const xmlChar *former_names, const xmlChar *upper_box_id,
        const xmlChar *ceo_label,
        const struct isds_credentials_delivery *credentials_delivery,
        const struct isds_approval *approval) {
    isds_error err = IE_SUCCESS;
    xmlNsPtr isds_ns = NULL;
    xmlNodePtr node, dbPrimaryUsers;
    xmlChar *string = NULL;
    const struct isds_list *item;


    if (!context) return IE_INVALID_CONTEXT;
    if (!request || !service_name || service_name[0] == '\0' || !box)
        return IE_INVAL;


    /* Build CreateDataBox-similar request */
    *request = xmlNewNode(NULL, service_name);
    if (!*request) {
        char *service_name_locale = _isds_utf82locale((char*) service_name);
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
     * verbose documentation allows none dbUserInfo */
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
    
    err = insert_credentials_delivery(context, credentials_delivery, *request);
    if (err) goto leave; 

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
#endif /* HAVE_LIBCURL */


/* Create new box.
 * @context is session context
 * @box is box description to create including single primary user (in case of
 * FO box type). It outputs box ID assigned by ISDS in dbID element.
 * @users is list of struct isds_DbUserInfo (primary users in case of non-FO
 * box, or contact address of PFO box owner)
 * @former_names is optional former name of box owner. Pass NULL if you don't care.
 * @upper_box_id is optional ID of supper box if currently created box is
 * subordinated.
 * @ceo_label is optional title of OVM box owner (e.g. mayor)
 * @credentials_delivery is NULL if new password should be delivered off-line
 * to box owner. It is valid pointer if owner should obtain new password on-line
 * on dedicated web server. Then input @credentials_delivery.email value is
 * his e-mail address he must provide to dedicated web server together
 * with output reallocated @credentials_delivery.token member. Output
 * member @credentials_delivery.new_user_name is unused up on this call.
 * @approval is optional external approval of box manipulation
 * @refnumber is reallocated serial number of request assigned by ISDS. Use
 * NULL, if you don't care.*/
isds_error isds_add_box(struct isds_ctx *context,
        struct isds_DbOwnerInfo *box, const struct isds_list *users,
        const char *former_names, const char *upper_box_id,
        const char *ceo_label,
        struct isds_credentials_delivery *credentials_delivery,
        const struct isds_approval *approval, char **refnumber) {
    isds_error err = IE_SUCCESS;
#if HAVE_LIBCURL
    xmlNodePtr request = NULL;
    xmlDocPtr response = NULL;
    xmlXPathContextPtr xpath_ctx = NULL;
    xmlXPathObjectPtr result = NULL;
#endif


    if (!context) return IE_INVALID_CONTEXT;
    zfree(context->long_message);
    if (credentials_delivery) {
        zfree(credentials_delivery->token);
        zfree(credentials_delivery->new_user_name);
    }
    if (!box) return IE_INVAL;

#if HAVE_LIBCURL
    /* Scratch box ID */
    zfree(box->dbID);

    /* Build CreateDataBox request */
    err = build_CreateDBInput_request(context,
            &request, BAD_CAST "CreateDataBox",
            box, users, (xmlChar *) former_names, (xmlChar *) upper_box_id,
            (xmlChar *) ceo_label, credentials_delivery, approval);
    if (err) goto leave;

    /* Send it to server and process response */
    err = send_destroy_request_check_response(context,
            SERVICE_DB_MANIPULATION, BAD_CAST "CreateDataBox", &request,
            &response, (xmlChar **) refnumber, NULL);

    /* Extract box ID */
    xpath_ctx = xmlXPathNewContext(response);
    if (!xpath_ctx) {
        err = IE_ERROR;
        goto leave;
    }
    if (_isds_register_namespaces(xpath_ctx, MESSAGE_NS_UNSIGNED)) {
        err = IE_ERROR;
        goto leave;
    }
    EXTRACT_STRING("/isds:CreateDataBoxResponse/isds:dbID", box->dbID);

    /* Extract optional token */
    err = extract_credentials_delivery(context, credentials_delivery, response,
            "CreateDataBox");

leave:
    xmlXPathFreeObject(result);
    xmlXPathFreeContext(xpath_ctx);
    xmlFreeDoc(response);
    xmlFreeNode(request);

    if (!err) {
        isds_log(ILF_ISDS, ILL_DEBUG,
                _("CreateDataBox request processed by server successfully.\n"));
    }
#else /* not HAVE_LIBCURL */
    err = IE_NOTSUP;
#endif

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
#if HAVE_LIBCURL
    xmlNodePtr request = NULL;
#endif

    if (!context) return IE_INVALID_CONTEXT;
    zfree(context->long_message);
    if (!box) return IE_INVAL;

#if HAVE_LIBCURL
    /* Build CreateDataBoxPFOInfo request */
    err = build_CreateDBInput_request(context,
            &request, BAD_CAST "CreateDataBoxPFOInfo",
            box, users, (xmlChar *) former_names, (xmlChar *) upper_box_id,
            (xmlChar *) ceo_label, NULL, approval);
    if (err) goto leave;

    /* Send it to server and process response */
    err = send_request_check_drop_response(context,
            SERVICE_DB_MANIPULATION, BAD_CAST "CreateDataBox", &request,
            (xmlChar **) refnumber);
    /* XXX: XML Schema names output dbID element but textual documentation
     * states no box identifier is returned. */
leave:
    xmlFreeNode(request);
#else /* not HAVE_LIBCURL */
    err = IE_NOTSUP;
#endif
    return err;
}


/* Common implementation for removing given box.
 * @context is session context
 * @service_name is UTF-8 encoded name fo ISDS service
 * @box is box description to delete
 * @since is date of box owner cancellation. Only tm_year, tm_mon and tm_mday
 * carry sane value. If NULL, do not inject this information into request.
 * @approval is optional external approval of box manipulation
 * @refnumber is reallocated serial number of request assigned by ISDS. Use
 * NULL, if you don't care.*/
static isds_error _isds_delete_box_common(struct isds_ctx *context,
        const xmlChar *service_name, 
        const struct isds_DbOwnerInfo *box, const struct tm *since,
        const struct isds_approval *approval, char **refnumber) {
    isds_error err = IE_SUCCESS;
#if HAVE_LIBCURL
    xmlNsPtr isds_ns = NULL;
    xmlNodePtr request = NULL;
    xmlNodePtr node;
    xmlChar *string = NULL;
#endif


    if (!context) return IE_INVALID_CONTEXT;
    zfree(context->long_message);
    if (!service_name || !*service_name || !box) return IE_INVAL;


#if HAVE_LIBCURL
    /* Build DeleteDataBox(Promptly) request */
    request = xmlNewNode(NULL, service_name);
    if (!request) {
        char *service_name_locale = _isds_utf82locale((char*)service_name);
        isds_printf_message(context,
                _("Could build %s request"), service_name_locale);
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

    if (since) {
        err = tm2datestring(since, &string);
        if (err) {
            isds_log_message(context,
                    _("Could not convert `since' argument to ISO date string"));
            goto leave;
        }
        INSERT_STRING(request, "dbOwnerTerminationDate", string);
        zfree(string);
    }

    err = insert_GExtApproval(context, approval, request);
    if (err) goto leave;


    /* Send it to server and process response */
    err = send_request_check_drop_response(context, SERVICE_DB_MANIPULATION,
            service_name, &request, (xmlChar **) refnumber);

leave:
    xmlFreeNode(request);
    free(string);
#else /* not HAVE_LIBCURL */
    err = IE_NOTSUP;
#endif
    return err;
}


/* Remove given box permanently.
 * @context is session context
 * @box is box description to delete
 * @since is date of box owner cancellation. Only tm_year, tm_mon and tm_mday
 * carry sane value.
 * @approval is optional external approval of box manipulation
 * @refnumber is reallocated serial number of request assigned by ISDS. Use
 * NULL, if you don't care.*/
isds_error isds_delete_box(struct isds_ctx *context,
        const struct isds_DbOwnerInfo *box, const struct tm *since,
        const struct isds_approval *approval, char **refnumber) {
    if (!context) return IE_INVALID_CONTEXT;
    zfree(context->long_message);
    if (!box || !since) return IE_INVAL;

    return _isds_delete_box_common(context, BAD_CAST "DeleteDataBox",
            box, since, approval, refnumber);
}


/* Undocumented function.
 * @context is session context
 * @box is box description to delete
 * @approval is optional external approval of box manipulation
 * @refnumber is reallocated serial number of request assigned by ISDS. Use
 * NULL, if you don't care.*/
isds_error isds_delete_box_promptly(struct isds_ctx *context,
        const struct isds_DbOwnerInfo *box,
        const struct isds_approval *approval, char **refnumber) {
    if (!context) return IE_INVALID_CONTEXT;
    zfree(context->long_message);
    if (!box) return IE_INVAL;

    return _isds_delete_box_common(context, BAD_CAST "DeleteDataBoxPromptly",
            box, NULL, approval, refnumber);
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
#if HAVE_LIBCURL
    xmlNsPtr isds_ns = NULL;
    xmlNodePtr request = NULL;
    xmlNodePtr node;
#endif


    if (!context) return IE_INVALID_CONTEXT;
    zfree(context->long_message);
    if (!old_box || !new_box) return IE_INVAL;


#if HAVE_LIBCURL
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
#else /* not HAVE_LIBCURL */
    err = IE_NOTSUP;
#endif

    return err;
}


#if HAVE_LIBCURL
/* Build ISDS request of XSD tIdDbInput type, sent it and check for error
 * code
 * @context is session context
 * @service is SOAP service
 * @service_name is name of request in @service
 * @box_id_element is name of element to wrap the @box_id. NULL means "dbID".
 * @box_id is box ID of interest
 * @approval is optional external approval of box manipulation
 * @response is server SOAP body response as XML document
 * @refnumber is reallocated serial number of request assigned by ISDS. Use
 * NULL, if you don't care.
 * @return error coded from lower layer, context message will be set up
 * appropriately. */
static isds_error build_send_dbid_request_check_response(
        struct isds_ctx *context, const isds_service service,
        const xmlChar *service_name, const xmlChar *box_id_element,
        const xmlChar *box_id, const struct isds_approval *approval,
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
    service_name_locale = _isds_utf82locale((char*)service_name);
    if (!service_name_locale) {
        err = IE_NOMEM;
        goto leave;
    }
    box_id_locale = _isds_utf82locale((char*)box_id);
    if (!box_id_locale) {
        err = IE_NOMEM;
        goto leave;
    }

    /* Build request */
    request = xmlNewNode(NULL, service_name);
    if (!request) {
        isds_printf_message(context,
                _("Could not build %s request for %s box"), service_name_locale,
                box_id_locale);
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
    if (NULL == box_id_element) box_id_element = BAD_CAST "dbID";
    INSERT_STRING(request, box_id_element, box_id);
    err = insert_GExtApproval(context, approval, request);
    if (err) goto leave;

    /* Send request and check response*/
    err = send_destroy_request_check_response(context,
            service, service_name, &request, response, refnumber, NULL);

leave:
    free(service_name_locale);
    free(box_id_locale);
    xmlFreeNode(request);
    return err;
}
#endif /* HAVE_LIBCURL */


/* Get data about all users assigned to given box.
 * @context is session context
 * @box_id is box ID
 * @users is automatically reallocated list of struct isds_DbUserInfo */
isds_error isds_GetDataBoxUsers(struct isds_ctx *context, const char *box_id,
        struct isds_list **users) {
    isds_error err = IE_SUCCESS;
#if HAVE_LIBCURL
    xmlDocPtr response = NULL;
    xmlXPathContextPtr xpath_ctx = NULL;
    xmlXPathObjectPtr result = NULL;
    int i;
    struct isds_list *item, *prev_item = NULL;
#endif

    if (!context) return IE_INVALID_CONTEXT;
    zfree(context->long_message);
    if (!users || !box_id) return IE_INVAL;
    isds_list_free(users);


#if HAVE_LIBCURL
    /* Do request and check for success */
    err = build_send_dbid_request_check_response(context,
            SERVICE_DB_MANIPULATION, BAD_CAST "GetDataBoxUsers", NULL,
            BAD_CAST box_id, NULL, &response, NULL);
    if (err) goto leave;


    /* Extract data */
    /* Prepare structure */
    xpath_ctx = xmlXPathNewContext(response);
    if (!xpath_ctx) {
        err = IE_ERROR;
        goto leave;
    }
    if (_isds_register_namespaces(xpath_ctx, MESSAGE_NS_UNSIGNED)) {
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
    if (!xmlXPathNodeSetIsEmpty(result->nodesetval)) {
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
#else /* not HAVE_LIBCURL */
    err = IE_NOTSUP;
#endif

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
#if HAVE_LIBCURL
    xmlNsPtr isds_ns = NULL;
    xmlNodePtr request = NULL;
    xmlNodePtr node;
#endif


    if (!context) return IE_INVALID_CONTEXT;
    zfree(context->long_message);
    if (!box || !old_user || !new_user) return IE_INVAL;


#if HAVE_LIBCURL
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
#else /* not HAVE_LIBCURL */
    err = IE_NOTSUP;
#endif

    return err;
}


/* Undocumented function. 
 * @context is session context
 * @box_id is UTF-8 encoded box identifier
 * @token is UTF-8 encoded temporary password
 * @user_id outputs UTF-8 encoded reallocated user identifier
 * @password outpus UTF-8 encoded reallocated user password
 * Output arguments will be nulled in case of error */
isds_error isds_activate(struct isds_ctx *context,
        const char *box_id, const char *token,
        char **user_id, char **password) {
    isds_error err = IE_SUCCESS;
#if HAVE_LIBCURL
    xmlNsPtr isds_ns = NULL;
    xmlNodePtr request = NULL, node;
    xmlDocPtr response = NULL;
    xmlXPathContextPtr xpath_ctx = NULL;
    xmlXPathObjectPtr result = NULL;
#endif


    if (!context) return IE_INVALID_CONTEXT;
    zfree(context->long_message);

    if (user_id) zfree(*user_id);
    if (password) zfree(*password);

    if (!box_id || !token || !user_id || !password) return IE_INVAL;


#if HAVE_LIBCURL
    /* Build Activate request */
    request = xmlNewNode(NULL, BAD_CAST "Activate");
    if (!request) {
        isds_log_message(context, _("Could build Activate request"));
        return IE_ERROR;
    }
    isds_ns = xmlNewNs(request, BAD_CAST ISDS_NS, NULL);
    if(!isds_ns) {
        isds_log_message(context, _("Could not create ISDS name space"));
        xmlFreeNode(request);
        return IE_ERROR;
    }
    xmlSetNs(request, isds_ns);

    INSERT_STRING(request, "dbAccessDataId", token);
    CHECK_FOR_STRING_LENGTH(box_id, 7, 7, "dbID");
    INSERT_STRING(request, "dbID", box_id);


    /* Send request and check response*/
    err = send_destroy_request_check_response(context,
            SERVICE_DB_MANIPULATION, BAD_CAST "Activate", &request,
            &response, NULL, NULL);
    if (err) goto leave;


    /* Extract data */
    xpath_ctx = xmlXPathNewContext(response);
    if (!xpath_ctx) {
        err = IE_ERROR;
        goto leave;
    }
    if (_isds_register_namespaces(xpath_ctx, MESSAGE_NS_UNSIGNED)) {
        err = IE_ERROR;
        goto leave;
    }
    result = xmlXPathEvalExpression(BAD_CAST "/isds:ActivateResponse",
            xpath_ctx);
    if (!result) {
        err = IE_ERROR;
        goto leave;
    }
    if (xmlXPathNodeSetIsEmpty(result->nodesetval)) {
        isds_log_message(context, _("Missing ActivateResponse element"));
        err = IE_ISDS;
        goto leave;
    }
    if (result->nodesetval->nodeNr > 1) {
        isds_log_message(context, _("Multiple ActivateResponse element"));
        err = IE_ISDS;
        goto leave;
    }
    xpath_ctx->node = result->nodesetval->nodeTab[0];
    xmlXPathFreeObject(result); result = NULL;

    EXTRACT_STRING("isds:userId", *user_id);
    if (!*user_id) 
        isds_log(ILF_ISDS, ILL_ERR, _("Server accepted Activate request, "
                    "but did not return `userId' element.\n"));

    EXTRACT_STRING("isds:password", *password);
    if (!*password) 
        isds_log(ILF_ISDS, ILL_ERR, _("Server accepted Activate request, "
                    "but did not return `password' element.\n"));

leave:
    xmlXPathFreeObject(result);
    xmlXPathFreeContext(xpath_ctx);
    xmlFreeDoc(response);
    xmlFreeNode(request);

    if (!err)
        isds_log(ILF_ISDS, ILL_DEBUG,
                _("Activate request processed by server successfully.\n"));
#else /* not HAVE_LIBCURL */
    err = IE_NOTSUP;
#endif

    return err;
}


/* Reset credentials of user assigned to given box.
 * @context is session context
 * @box is box identification
 * @user identifies user to reset password
 * @fee_paid is true if fee has been paid, false otherwise
 * @approval is optional external approval of box manipulation
 * @credentials_delivery is NULL if new password should be delivered off-line
 * to the user. It is valid pointer if user should obtain new password on-line
 * on dedicated web server. Then input @credentials_delivery.email value is
 * user's e-mail address user must provide to dedicated web server together
 * with @credentials_delivery.token. The output reallocated token user needs
 * to use to authorize on the web server to view his new password. Output
 * reallocated @credentials_delivery.new_user_name is user's log-in name that
 * ISDS changed up on this call. (No reason why server could change the name
 * is known now.)
 * @refnumber is reallocated serial number of request assigned by ISDS. Use
 * NULL, if you don't care.*/
isds_error isds_reset_password(struct isds_ctx *context,
        const struct isds_DbOwnerInfo *box,
        const struct isds_DbUserInfo *user,
        const _Bool fee_paid, const struct isds_approval *approval,
        struct isds_credentials_delivery *credentials_delivery,
        char **refnumber) {
    isds_error err = IE_SUCCESS;
#if HAVE_LIBCURL
    xmlNsPtr isds_ns = NULL;
    xmlNodePtr request = NULL, node;
    xmlDocPtr response = NULL;
#endif


    if (!context) return IE_INVALID_CONTEXT;
    zfree(context->long_message);

    if (credentials_delivery) {
        zfree(credentials_delivery->token);
        zfree(credentials_delivery->new_user_name);
    }
    if (!box || !user) return IE_INVAL;


#if HAVE_LIBCURL
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

    err = insert_credentials_delivery(context, credentials_delivery, request);
    if (err) goto leave;

    err = insert_GExtApproval(context, approval, request);
    if (err) goto leave;

    /* Send request and check response*/
    err = send_destroy_request_check_response(context,
            SERVICE_DB_MANIPULATION, BAD_CAST "NewAccessData", &request,
            &response, (xmlChar **) refnumber, NULL);
    if (err) goto leave;


    /* Extract optional token */
    err = extract_credentials_delivery(context, credentials_delivery,
            response, "NewAccessData");

leave:
    xmlFreeDoc(response);
    xmlFreeNode(request);

    if (!err)
        isds_log(ILF_ISDS, ILL_DEBUG,
                _("NewAccessData request processed by server "
                    "successfully.\n"));
#else /* not HAVE_LIBCURL */
    err = IE_NOTSUP;
#endif

    return err;
}


/* Build ISDS request of XSD tAddDBUserInput type, sent it, check for error
 * code, destroy response and log success.
 * @context is ISDS session context.
 * @service_name is name of SERVICE_DB_MANIPULATION service
 * @box is box identification
 * @user identifies user to remove
 * @credentials_delivery is NULL if new user's password should be delivered
 * off-line to the user. It is valid pointer if user should obtain new
 * password on-line on dedicated web server. Then input
 * @credentials_delivery.email value is user's e-mail address user must
 * provide to dedicated web server together with @credentials_delivery.token.
 * The output reallocated token user needs to use to authorize on the web
 * server to view his new password. Output reallocated
 * @credentials_delivery.new_user_name is user's log-in name that ISDS
 * assingned or changed up on this call.
 * @approval is optional external approval of box manipulation
 * @refnumber is reallocated serial number of request assigned by ISDS. Use
 * NULL, if you don't care. */
static isds_error build_send_manipulationboxuser_request_check_drop_response(
        struct isds_ctx *context, const xmlChar *service_name,
        const struct isds_DbOwnerInfo *box, const struct isds_DbUserInfo *user,
        struct isds_credentials_delivery *credentials_delivery,
        const struct isds_approval *approval, xmlChar **refnumber) {
    isds_error err = IE_SUCCESS;
#if HAVE_LIBCURL
    xmlNsPtr isds_ns = NULL;
    xmlNodePtr request = NULL, node;
    xmlDocPtr response = NULL;
#endif


    if (!context) return IE_INVALID_CONTEXT;
    zfree(context->long_message);
    if (credentials_delivery) {
        zfree(credentials_delivery->token);
        zfree(credentials_delivery->new_user_name);
    }
    if (!service_name || service_name[0] == '\0' || !box || !user)
        return IE_INVAL;


#if HAVE_LIBCURL
    /* Build NewAccessData or similar request */
    request = xmlNewNode(NULL, service_name);
    if (!request) {
        char *service_name_locale = _isds_utf82locale((char *) service_name);
        isds_printf_message(context, _("Could not build %s request"),
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

    err = insert_credentials_delivery(context, credentials_delivery, request);
    if (err) goto leave;

    err = insert_GExtApproval(context, approval, request);
    if (err) goto leave;


    /* Send request and check response*/
    err = send_destroy_request_check_response(context,
            SERVICE_DB_MANIPULATION, service_name, &request, &response,
            refnumber, NULL);

    xmlFreeNode(request);
    request = NULL;

    /* Pick up credentials_delivery if requested */
    err = extract_credentials_delivery(context, credentials_delivery, response,
            (char *)service_name);

leave:
    xmlFreeDoc(response);
    if (request) xmlFreeNode(request);

    if (!err) {
        char *service_name_locale = _isds_utf82locale((char *) service_name);
        isds_log(ILF_ISDS, ILL_DEBUG,
                _("%s request processed by server successfully.\n"),
                service_name_locale);
        free(service_name_locale);
    }
#else /* not HAVE_LIBCURL */
    err = IE_NOTSUP;
#endif

    return err;
}


/* Assign new user to given box.
 * @context is session context
 * @box is box identification
 * @user defines new user to add
 * @credentials_delivery is NULL if new user's password should be delivered
 * off-line to the user. It is valid pointer if user should obtain new
 * password on-line on dedicated web server. Then input
 * @credentials_delivery.email value is user's e-mail address user must
 * provide to dedicated web server together with @credentials_delivery.token.
 * The output reallocated token user needs to use to authorize on the web
 * server to view his new password. Output reallocated
 * @credentials_delivery.new_user_name is user's log-in name that ISDS
 * assingned up on this call.
 * @approval is optional external approval of box manipulation
 * @refnumber is reallocated serial number of request assigned by ISDS. Use
 * NULL, if you don't care.*/
isds_error isds_add_user(struct isds_ctx *context,
        const struct isds_DbOwnerInfo *box, const struct isds_DbUserInfo *user,
        struct isds_credentials_delivery *credentials_delivery,
        const struct isds_approval *approval, char **refnumber) {
    return build_send_manipulationboxuser_request_check_drop_response(context,
            BAD_CAST "AddDataBoxUser", box, user, credentials_delivery,
            approval, (xmlChar **) refnumber);
}


/* Remove user assigned to given box.
 * @context is session context
 * @box is box identification
 * @user identifies user to remove
 * @approval is optional external approval of box manipulation
 * @refnumber is reallocated serial number of request assigned by ISDS. Use
 * NULL, if you don't care.*/
isds_error isds_delete_user(struct isds_ctx *context,
        const struct isds_DbOwnerInfo *box, const struct isds_DbUserInfo *user,
        const struct isds_approval *approval, char **refnumber) {
    return build_send_manipulationboxuser_request_check_drop_response(context,
            BAD_CAST "DeleteDataBoxUser", box, user, NULL, approval,
            (xmlChar **) refnumber);
}


/* Get list of boxes in ZIP archive.
 * @context is session context
 * @list_identifier is UTF-8 encoded string identifying boxes of interrest.
 * System recognizes following values currently: ALL (all boxes), UPG
 * (effectively OVM boxes), POA (active boxes allowing receiving commercial
 * messages), OVM (OVM gross type boxes), OPN (boxes allowing receiving
 * commercial messages). This argument is a string because specification
 * states new values can appear in the future. Not all list types are
 * available to all users.
 * @buffer is automatically reallocated memory to store the list of boxes. The
 * list is zipped CSV file.
 * @buffer_length is size of @buffer data in bytes.
 * In case of error @buffer will be freed and @buffer_length will be
 * undefined.*/
isds_error isds_get_box_list_archive(struct isds_ctx *context,
        const char *list_identifier, void **buffer, size_t *buffer_length) {
    isds_error err = IE_SUCCESS;
#if HAVE_LIBCURL
    xmlNsPtr isds_ns = NULL;
    xmlNodePtr request = NULL, node;
    xmlDocPtr response = NULL;
    xmlXPathContextPtr xpath_ctx = NULL;
    xmlXPathObjectPtr result = NULL;
    char *string = NULL;
#endif


    if (!context) return IE_INVALID_CONTEXT;
    zfree(context->long_message);
    if (buffer) zfree(*buffer);
    if (!buffer || !buffer_length) return IE_INVAL;


#if HAVE_LIBCURL
    /* Check if connection is established
     * TODO: This check should be done downstairs. */
    if (!context->curl) return IE_CONNECTION_CLOSED;


    /* Build AuthenticateMessage request */
    request = xmlNewNode(NULL, BAD_CAST "GetDataBoxList");
    if (!request) {
        isds_log_message(context,
                _("Could not build GetDataBoxList request"));
        return IE_ERROR;
    }
    isds_ns = xmlNewNs(request, BAD_CAST ISDS_NS, NULL);
    if(!isds_ns) {
        isds_log_message(context, _("Could not create ISDS name space"));
        xmlFreeNode(request);
        return IE_ERROR;
    }
    xmlSetNs(request, isds_ns);
    INSERT_STRING(request, "dblType", list_identifier);

    /* Send request to server and process response */
    err = send_destroy_request_check_response(context,
            SERVICE_DB_SEARCH, BAD_CAST "GetDataBoxList", &request,
            &response, NULL, NULL);
    if (err) goto leave;


    /* Extract Base-64 encoded ZIP file */
    xpath_ctx = xmlXPathNewContext(response);
    if (!xpath_ctx) {
        err = IE_ERROR;
        goto leave;
    }
    if (_isds_register_namespaces(xpath_ctx, MESSAGE_NS_UNSIGNED)) {
        err = IE_ERROR;
        goto leave;
    }
    EXTRACT_STRING("/isds:GetDataBoxListResponse/isds:dblData", string);

    /* Decode non-empty archive */
    if (string && string[0] != '\0') {
        *buffer_length = _isds_b64decode(string, buffer);
        if (*buffer_length == (size_t) -1) {
            isds_printf_message(context,
                    _("Error while Base64-decoding box list archive"));
            err = IE_ERROR;
            goto leave;
        }
    }


leave:
    free(string);
    xmlXPathFreeObject(result);
    xmlXPathFreeContext(xpath_ctx);
    xmlFreeDoc(response);
    xmlFreeNode(request);

    if (!err) {
        isds_log(ILF_ISDS, ILL_DEBUG, _("GetDataBoxList request "
                    "processed by server successfully.\n"));
    }
#else /* not HAVE_LIBCURL */
    err = IE_NOTSUP;
#endif

    return err;
}


/* Find boxes suiting given criteria.
 * @criteria is filter. You should fill in at least some members.
 * @boxes is automatically reallocated list of isds_DbOwnerInfo structures,
 * possibly empty. Input NULL or valid old structure.
 * @return:
 *  IE_SUCCESS if search succeeded, @boxes contains useful data
 *  IE_NOEXIST if no such box exists, @boxes will be NULL
 *  IE_2BIG if too much boxes exist and server truncated the results, @boxes
 *      contains still valid data
 *  other code if something bad happens. @boxes will be NULL. */
isds_error isds_FindDataBox(struct isds_ctx *context,
        const struct isds_DbOwnerInfo *criteria,
        struct isds_list **boxes) {
    isds_error err = IE_SUCCESS;
#if HAVE_LIBCURL
    _Bool truncated = 0;
    xmlNsPtr isds_ns = NULL;
    xmlNodePtr request = NULL;
    xmlDocPtr response = NULL;
    xmlChar *code = NULL, *message = NULL;
    xmlNodePtr db_owner_info;
    xmlXPathContextPtr xpath_ctx = NULL;
    xmlXPathObjectPtr result = NULL;
    xmlChar *string = NULL;
#endif


    if (!context) return IE_INVALID_CONTEXT;
    zfree(context->long_message);
    if (!boxes) return IE_INVAL;
    isds_list_free(boxes);

    if (!criteria) {
        return IE_INVAL;
    }

#if HAVE_LIBCURL
    /* Check if connection is established
     * TODO: This check should be done downstairs. */
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
    err = _isds(context, SERVICE_DB_SEARCH, request, &response, NULL, NULL);
   
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
        char *code_locale = _isds_utf82locale((char*)code);
        char *message_locale = _isds_utf82locale((char*)message);
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
        char *code_locale = _isds_utf82locale((char*)code);
        char *message_locale = _isds_utf82locale((char*)message);
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
        char *code_locale = _isds_utf82locale((char*)code);
        char *message_locale = _isds_utf82locale((char*)message);
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
    if (_isds_register_namespaces(xpath_ctx, MESSAGE_NS_UNSIGNED)) {
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
#else /* not HAVE_LIBCURL */
    err = IE_NOTSUP;
#endif

    return err;
}


#if HAVE_LIBCURL
/* Convert a string with match markers into a plain string with list of
 * pointers to the matches
 * @string is an UTF-8 encoded non-constant string with match markers
 * "|$*HL_START*$|" for start and "|$*HL_END*$|" for end of a match.
 * The markers will be removed from the string.
 * @starts is a reallocated list of static pointers into the @string pointing
 * to places where match start markers occured.
 * @ends is a reallocated list of static pointers into the @string pointing
 * to places where match end markers occured.
 * @return IE_SUCCESS in case of no failure. */
static isds_error interpret_matches(xmlChar *string,
        struct isds_list **starts, struct isds_list **ends) {
    isds_error err = IE_SUCCESS;
    xmlChar *pointer, *destination, *source;
    struct isds_list *item, *prev_start = NULL, *prev_end = NULL;

    isds_list_free(starts);
    isds_list_free(ends);
    if (NULL == starts || NULL == ends) return IE_INVAL;
    if (NULL == string) return IE_SUCCESS;

    for (pointer = string; *pointer != '\0';) {
        if (!xmlStrncmp(pointer, BAD_CAST "|$*HL_START*$|", 14)) {
            /* Remove the start marker */
            for (source = pointer + 14, destination = pointer;
                    *source != '\0'; source++, destination++) {
                *destination = *source;
            }
            *destination = '\0';
            /* Append the pointer into the list */
            item = calloc(1, sizeof(*item));
            if (!item) {
                err = IE_NOMEM;
                goto leave;
            }
            item->destructor = (void (*)(void **))NULL;
            item->data = pointer;
            if (NULL == prev_start) *starts = item;
            else prev_start->next = item;
            prev_start = item;
        } else if (!xmlStrncmp(pointer, BAD_CAST "|$*HL_END*$|", 12)) {
            /* Remove the end marker */
            for (source = pointer + 12, destination = pointer;
                    *source != '\0'; source++, destination++) {
                *destination = *source;
            }
            *destination = '\0';
            /* Append the pointer into the list */
            item = calloc(1, sizeof(*item));
            if (!item) {
                err = IE_NOMEM;
                goto leave;
            }
            item->destructor = (void (*)(void **))NULL;
            item->data = pointer;
            if (NULL == prev_end) *ends = item;
            else prev_end->next = item;
            prev_end = item;
        } else {
            pointer++;
        }
    }

leave:
    if (err) {
        isds_list_free(starts);
        isds_list_free(ends);
    }
    return err;
}


/* Convert isds:dbResult XML tree into structure
 * @context is ISDS context.
 * @fulltext_result is automatically reallocated found box structure.
 * @xpath_ctx is XPath context with current node as isds:dbResult element.
 * @collect_matches is true to interpret match markers.
 * In case of error @result will be freed. */
static isds_error extract_dbResult(struct isds_ctx *context,
        struct isds_fulltext_result **fulltext_result,
        xmlXPathContextPtr xpath_ctx, _Bool collect_matches) {
    isds_error err = IE_SUCCESS;
    xmlXPathObjectPtr result = NULL;
    char *string = NULL;

    if (NULL == context) return IE_INVALID_CONTEXT;
    if (NULL == fulltext_result) return IE_INVAL;
    isds_fulltext_result_free(fulltext_result);
    if (!xpath_ctx) return IE_INVAL;


    *fulltext_result = calloc(1, sizeof(**fulltext_result));
    if (NULL == *fulltext_result) {
        err = IE_NOMEM;
        goto leave;
    }

    /* Extract data */
    EXTRACT_STRING("isds:dbID", (*fulltext_result)->dbID);

    EXTRACT_STRING("isds:dbType", string);
    if (NULL == string) {
        err = IE_ISDS;
        isds_log_message(context, _("Empty isds:dbType element"));
        goto leave;
    }
    err = string2isds_DbType((xmlChar *)string, &(*fulltext_result)->dbType);
    if (err) {
        if (err == IE_ENUM) {
            err = IE_ISDS;
            char *string_locale = _isds_utf82locale(string);
            isds_printf_message(context, _("Unknown isds:dbType: %s"),
                string_locale);
            free(string_locale);
        }
        goto leave;
    }
    zfree(string);

    EXTRACT_STRING("isds:dbName", (*fulltext_result)->name);
    EXTRACT_STRING("isds:dbAddress", (*fulltext_result)->address);

    err = extract_BiDate(context, &(*fulltext_result)->biDate, xpath_ctx);
    if (err) goto leave;

    EXTRACT_STRING("isds:dbICO", (*fulltext_result)->ic);
    EXTRACT_BOOLEANNOPTR("isds:dbEffectiveOVM",
            (*fulltext_result)->dbEffectiveOVM);

    EXTRACT_STRING("isds:dbSendOptions", string);
    if (NULL == string) {
        err = IE_ISDS;
        isds_log_message(context, _("Empty isds:dbSendOptions element"));
        goto leave;
    }
    if (!xmlStrcmp(BAD_CAST string, BAD_CAST "DZ")) {
        (*fulltext_result)->active = 1;
        (*fulltext_result)->public_sending = 1;
        (*fulltext_result)->commercial_sending = 0;
    } else if (!xmlStrcmp(BAD_CAST string, BAD_CAST "ALL")) {
        (*fulltext_result)->active = 1;
        (*fulltext_result)->public_sending = 1;
        (*fulltext_result)->commercial_sending = 1;
    } else if (!xmlStrcmp(BAD_CAST string, BAD_CAST "PDZ")) {
        (*fulltext_result)->active = 1;
        (*fulltext_result)->public_sending = 0;
        (*fulltext_result)->commercial_sending = 1;
    } else if (!xmlStrcmp(BAD_CAST string, BAD_CAST "NONE")) {
        (*fulltext_result)->active = 1;
        (*fulltext_result)->public_sending = 0;
        (*fulltext_result)->commercial_sending = 0;
    } else if (!xmlStrcmp(BAD_CAST string, BAD_CAST "DISABLED")) {
        (*fulltext_result)->active = 0;
        (*fulltext_result)->public_sending = 0;
        (*fulltext_result)->commercial_sending = 0;
    } else {
        err = IE_ISDS;
        char *string_locale = _isds_utf82locale(string);
        isds_printf_message(context, _("Unknown isds:dbSendOptions value: %s"),
            string_locale);
        free(string_locale);
        goto leave;
    }
    zfree(string);

    /* Interpret match marks */
    if (collect_matches) {
        err = interpret_matches(BAD_CAST (*fulltext_result)->name,
                &((*fulltext_result)->name_match_start),
                &((*fulltext_result)->name_match_end));
        if (err) goto leave;
        err = interpret_matches(BAD_CAST (*fulltext_result)->address,
                &((*fulltext_result)->address_match_start),
                &((*fulltext_result)->address_match_end));
        if (err) goto leave;
    }

leave:
    if (err) isds_fulltext_result_free(fulltext_result);
    free(string);
    xmlXPathFreeObject(result);
    return err;
}
#endif /* HAVE_LIBCURL */


/* Find boxes matching a given full-text criteria.
 * @context is a session context
 * @query is a non-empty string which consists of words to search
 * @target selects box attributes to search for @query words. Pass NULL if you
 * don't care.
 * @box_type restricts searching to given box type. Value DBTYPE_SYSTEM means
 * to search in all box types. Value DBTYPE_OVM_MAIN means to search in
 * non-subsudiary OVM box types. Pass NULL to let server to use default value
 * which is DBTYPE_SYSTEM.
 * @page_size defines count of boxes to constitute a response page. It counts
 * from zero. Pass NULL to let server to use a default value (50 now).
 * @page_number defines ordinar number of the response page to return. It
 * counts from zero. Pass NULL to let server to use a default value (0 now).
 * @track_matches points to true for marking @query words found in the box
 * attributes. It points to false for not marking. Pass NULL to let the server
 * to use default value (false now).
 * @total_matching_boxes outputs reallocated number of all boxes matching the
 * query. Will be pointer to NULL if server did not provide the value.
 * Pass NULL if you don't care.
 * @current_page_beginning outputs reallocated ordinar number of the first box
 * in this @boxes page. It counts from zero. It will be pointer to NULL if the
 * server did not provide the value. Pass NULL if you don't care.
 * @current_page_size outputs reallocated count of boxes in the this @boxes
 * page. It will be pointer to NULL if the server did not provide the value.
 * Pass NULL if you don't care.
 * @last_page outputs pointer to reallocated boolean. True if this @boxes page
 * is the last one, false if more boxes match, NULL if the server did not
 * provude the value. Pass NULL if you don't care.
 * @boxes outputs reallocated list of isds_fulltext_result structures,
 * possibly empty.
 * @return:
 *  IE_SUCCESS if search succeeded
 *  IE_2BIG if @page_size is too large
 *  other code if something bad happens; output arguments will be NULL. */
isds_error isds_find_box_by_fulltext(struct isds_ctx *context,
        const char *query,
        const isds_fulltext_target *target,
        const isds_DbType *box_type,
        const unsigned long int *page_size,
        const unsigned long int *page_number,
        const _Bool *track_matches,
        unsigned long int **total_matching_boxes,
        unsigned long int **current_page_beginning,
        unsigned long int **current_page_size,
        _Bool **last_page,
        struct isds_list **boxes) {
    isds_error err = IE_SUCCESS;
#if HAVE_LIBCURL
    xmlNsPtr isds_ns = NULL;
    xmlNodePtr request = NULL;
    xmlDocPtr response = NULL;
    xmlNodePtr node;
    xmlXPathContextPtr xpath_ctx = NULL;
    xmlXPathObjectPtr result = NULL;
    const xmlChar *static_string = NULL;
    xmlChar *string = NULL;

    const xmlChar *codes[] = {
        BAD_CAST "1004",
        BAD_CAST "1152",
        BAD_CAST "1153",
        BAD_CAST "1154",
        BAD_CAST "1155",
        BAD_CAST "1156",
        BAD_CAST "9002",
        NULL
    };
    const char *meanings[] = {
        N_("You are not allowed to perform the search"),
        N_("The query string is empty"),
        N_("Searched box ID is malformed"),
        N_("Searched organization ID is malformed"),
        N_("Invalid input"),
        N_("Requested page size is too large"),
        N_("Search engine internal error")
    };
    const isds_error errors[] = {
        IE_ISDS,
        IE_INVAL,
        IE_INVAL,
        IE_INVAL,
        IE_INVAL,
        IE_2BIG,
        IE_ISDS
    };
    struct code_map_isds_error map = {
        .codes = codes,
        .meanings = meanings,
        .errors = errors
    };
#endif


    if (NULL != total_matching_boxes) zfree(*total_matching_boxes);
    if (NULL != current_page_beginning) zfree(*current_page_beginning);
    if (NULL != current_page_size) zfree(*current_page_size);
    if (NULL != last_page) zfree(*last_page);
    isds_list_free(boxes);

    if (NULL == context) return IE_INVALID_CONTEXT;
    zfree(context->long_message);

    if (NULL == boxes) return IE_INVAL;

    if (NULL == query || !xmlStrcmp(BAD_CAST query, BAD_CAST "")) {
        isds_log_message(context, _("Query string must be non-empty"));
        return IE_INVAL;
    }

#if HAVE_LIBCURL
    /* Check if connection is established
     * TODO: This check should be done downstairs. */
    if (NULL == context->curl) return IE_CONNECTION_CLOSED;

    /* Build FindDataBox request */
    request = xmlNewNode(NULL, BAD_CAST "ISDSSearch2");
    if (NULL == request) {
        isds_log_message(context,
                _("Could not build ISDSSearch2 request"));
        return IE_ERROR;
    }
    isds_ns = xmlNewNs(request, BAD_CAST ISDS_NS, NULL);
    if(NULL == isds_ns) {
        isds_log_message(context, _("Could not create ISDS name space"));
        xmlFreeNode(request);
        return IE_ERROR;
    }
    xmlSetNs(request, isds_ns);

    INSERT_STRING(request, "searchText", query);

    if (NULL != target) {
        static_string = isds_fulltext_target2string(*(target));
        if (NULL == static_string) {
            isds_printf_message(context, _("Invalid target value: %d"),
                    *(target));
            err = IE_ENUM;
            goto leave;
        }
    }
    INSERT_STRING(request, "searchType", static_string);
    static_string = NULL;

    if (NULL != box_type) {
        /* XXX: Handle DBTYPE_SYSTEM value as "ALL" */
        if (DBTYPE_SYSTEM == *box_type) {
            static_string = BAD_CAST "ALL";
        } else if (DBTYPE_OVM_MAIN == *box_type) {
            static_string = BAD_CAST "OVM_MAIN";
        } else {
            static_string = isds_DbType2string(*(box_type));
            if (NULL == static_string) {
                isds_printf_message(context, _("Invalid box type value: %d"),
                        *(box_type));
                err = IE_ENUM;
                goto leave;
            }
        }
    }
    INSERT_STRING(request, "searchScope", static_string);
    static_string = NULL;

    INSERT_ULONGINT(request, "page", page_number, string);
    INSERT_ULONGINT(request, "pageSize", page_size, string);
    INSERT_BOOLEAN(request, "highlighting", track_matches);

    /* Send request and check response */
    err = send_destroy_request_check_response(context,
            SERVICE_DB_SEARCH, BAD_CAST "ISDSSearch2",
            &request, &response, NULL, &map);
    if (err) goto leave;

    /* Parse response */
    xpath_ctx = xmlXPathNewContext(response);
    if (NULL == xpath_ctx) {
        err = IE_ERROR;
        goto leave;
    }
    if (_isds_register_namespaces(xpath_ctx, MESSAGE_NS_UNSIGNED)) {
        err = IE_ERROR;
        goto leave;
    }
    result = xmlXPathEvalExpression(BAD_CAST "/isds:ISDSSearch2Response",
            xpath_ctx);
    if (!result) {
        err = IE_ERROR;
        goto leave;
    }
    if (xmlXPathNodeSetIsEmpty(result->nodesetval)) {
        isds_log_message(context, _("Missing ISDSSearch2 element"));
        err = IE_ISDS;
        goto leave;
    }
    if (result->nodesetval->nodeNr > 1) {
        isds_log_message(context, _("Multiple ISDSSearch2 element"));
        err = IE_ISDS;
        goto leave;
    }
    xpath_ctx->node = result->nodesetval->nodeTab[0];
    xmlXPathFreeObject(result); result = NULL;


    /* Extract counters */
    if (NULL != total_matching_boxes) {
        EXTRACT_ULONGINT("isds:totalCount", *total_matching_boxes, 0);
    }
    if (NULL != current_page_size) {
        EXTRACT_ULONGINT("isds:currentCount", *current_page_size, 0);
    }
    if (NULL != current_page_beginning) {
        EXTRACT_ULONGINT("isds:position", *current_page_beginning, 0);
    }
    if (NULL != last_page) {
        EXTRACT_BOOLEAN("isds:lastPage", *last_page);
    }
    xmlXPathFreeObject(result); result = NULL;

    /* Extract boxes if they present */
    result = xmlXPathEvalExpression(BAD_CAST
            "isds:dbResults/isds:dbResult", xpath_ctx);
    if (NULL == result) {
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

            item->destructor = (void (*)(void **))isds_fulltext_result_free;
            if (i == 0) *boxes = item;
            else prev_item->next = item;
            prev_item = item;

            xpath_ctx->node = result->nodesetval->nodeTab[i];
            err = extract_dbResult(context,
                    (struct isds_fulltext_result **) &(item->data), xpath_ctx,
                    (NULL == track_matches) ? 0 : *track_matches);
            if (err) goto leave;
        }
    }

leave:
    if (err) {
        if (NULL != total_matching_boxes) zfree(*total_matching_boxes);
        if (NULL != current_page_beginning) zfree(*current_page_beginning);
        if (NULL != current_page_size) zfree(*current_page_size);
        if (NULL != last_page) zfree(*last_page);
        isds_list_free(boxes);
    }

    free(string);
    xmlFreeNode(request);
    xmlXPathFreeObject(result);
    xmlXPathFreeContext(xpath_ctx);
    xmlFreeDoc(response);

    if (!err)
        isds_log(ILF_ISDS, ILL_DEBUG,
                _("ISDSSearch2 request processed by server successfully.\n"));
#else /* not HAVE_LIBCURL */
    err = IE_NOTSUP;
#endif

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
#if HAVE_LIBCURL
    xmlNsPtr isds_ns = NULL;
    xmlNodePtr request = NULL, db_id;
    xmlDocPtr response = NULL;
    xmlXPathContextPtr xpath_ctx = NULL;
    xmlXPathObjectPtr result = NULL;
    xmlChar *string = NULL;

    const xmlChar *codes[] = {
        BAD_CAST "5001",
        BAD_CAST "1007",
        BAD_CAST "2011",
        NULL
    };
    const char *meanings[] = {
        "The box does not exist",
        "Box ID is malformed",
        "Box ID malformed",
    };
    const isds_error errors[] = {
        IE_NOEXIST,
        IE_INVAL,
        IE_INVAL,
    };
    struct code_map_isds_error map = {
        .codes = codes,
        .meanings = meanings,
        .errors = errors
    };
#endif

    if (!context) return IE_INVALID_CONTEXT;
    zfree(context->long_message);
    if (!box_status || !box_id || *box_id == '\0') return IE_INVAL;

#if HAVE_LIBCURL
    /* Check if connection is established
     * TODO: This check should be done downstairs. */
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


    /* Send request and check response*/
    err = send_destroy_request_check_response(context,
            SERVICE_DB_SEARCH, BAD_CAST "CheckDataBox",
            &request, &response, NULL, &map);
    if (err) goto leave;


    /* Extract data */
    xpath_ctx = xmlXPathNewContext(response);
    if (!xpath_ctx) {
        err = IE_ERROR;
        goto leave;
    }
    if (_isds_register_namespaces(xpath_ctx, MESSAGE_NS_UNSIGNED)) {
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

    xmlFreeDoc(response);

    if (!err)
        isds_log(ILF_ISDS, ILL_DEBUG,
                _("CheckDataBox request processed by server successfully.\n"));
#else /* not HAVE_LIBCURL */
    err = IE_NOTSUP;
#endif

    return err;
}


#if HAVE_LIBCURL
/* Convert XSD:tdbPeriod XML tree into structure
 * @context is ISDS context.
 * @period is automatically reallocated found box status period structure.
 * @xpath_ctx is XPath context with current node as element of
 * XSD:tDbPeriod type.
 * In case of error @period will be freed. */
static isds_error extract_Period(struct isds_ctx *context,
        struct isds_box_state_period **period, xmlXPathContextPtr xpath_ctx) {
    isds_error err = IE_SUCCESS;
    xmlXPathObjectPtr result = NULL;
    char *string = NULL;
    long int *dbState_ptr;

    if (NULL == context) return IE_INVALID_CONTEXT;
    if (NULL == period) return IE_INVAL;
    isds_box_state_period_free(period);
    if (!xpath_ctx) return IE_INVAL;


    *period = calloc(1, sizeof(**period));
    if (NULL == *period) {
        err = IE_NOMEM;
        goto leave;
    }

    /* Extract data */
    EXTRACT_STRING("isds:PeriodFrom", string);
    if (NULL == string) {
        err = IE_XML;
        isds_log_message(context,
                _("Could not find PeriodFrom element value"));
        goto leave;
    }
    err = timestring2static_timeval((xmlChar *) string,
            &((*period)->from));
    if (err) {
        char *string_locale = _isds_utf82locale(string);
        if (err == IE_DATE) err = IE_ISDS;
        isds_printf_message(context,
                _("Could not convert PeriodFrom as ISO time: %s"),
                string_locale);
        free(string_locale);
        goto leave;
    }
    zfree(string);

    EXTRACT_STRING("isds:PeriodTo", string);
    if (NULL == string) {
        err = IE_XML;
        isds_log_message(context,
                _("Could not find PeriodTo element value"));
        goto leave;
    }
    err = timestring2static_timeval((xmlChar *) string,
            &((*period)->to));
    if (err) {
        char *string_locale = _isds_utf82locale(string);
        if (err == IE_DATE) err = IE_ISDS;
        isds_printf_message(context,
                _("Could not convert PeriodTo as ISO time: %s"),
                string_locale);
        free(string_locale);
        goto leave;
    }
    zfree(string);

    dbState_ptr = &((*period)->dbState);
    EXTRACT_LONGINT("isds:DbState", dbState_ptr, 1);

leave:
    if (err) isds_box_state_period_free(period);
    free(string);
    xmlXPathFreeObject(result);
    return err;
}
#endif /* HAVE_LIBCURL */


/* Get history of box state changes.
 * @context is ISDS session context.
 * @box_id is UTF-8 encoded sender box identifier as zero terminated string.
 * @from_time is first second of history to return in @history. Server ignores
 * subseconds. NULL means time of creating the box.
 * @to_time is last second of history to return in @history. Server ignores
 * subseconds. It's valid to have the @from_time equaled to the @to_time. The
 * interval is closed from both ends. NULL means now.
 * @history outputs auto-reallocated list of pointers to struct
 * isds_box_state_period. Each item describes a continues time when the box
 * was in one state. The state is 1 for accessible box. Otherwise the box
 * is inaccessible (priviledged users will get exact box state as enumerated
 * in isds_DbState, other users 0).
 * @return:
 *  IE_SUCCESS if the history has been obtained correctly,
 *  or other appropriate error. Please note that server allows to retrieve
 *  the history only to some users. */
isds_error isds_get_box_state_history(struct isds_ctx *context,
        const char *box_id,
        const struct timeval *from_time, const struct timeval *to_time,
        struct isds_list **history) {
    isds_error err = IE_SUCCESS;
#if HAVE_LIBCURL
    char *box_id_locale = NULL;
    xmlNodePtr request = NULL, node;
    xmlNsPtr isds_ns = NULL;
    xmlChar *string = NULL;

    xmlDocPtr response = NULL;
    xmlXPathContextPtr xpath_ctx = NULL;
    xmlXPathObjectPtr result = NULL;
#endif

    if (!context) return IE_INVALID_CONTEXT;
    zfree(context->long_message);

    /* Free output argument */
    isds_list_free(history);

#if HAVE_LIBCURL
    /* Check if connection is established */
    if (NULL == context->curl) return IE_CONNECTION_CLOSED;

    /* ??? XML schema allows empty box ID, textual documentation
     * requries the value. */
    /* Allow undefined box_id */
    if (NULL != box_id) {
        box_id_locale = _isds_utf82locale((char*)box_id);
        if (NULL == box_id_locale) {
            err = IE_NOMEM;
            goto leave;
        }
    }

    /* Build request */
    request = xmlNewNode(NULL, BAD_CAST "GetDataBoxActivityStatus");
    if (NULL == request) {
        isds_printf_message(context,
                _("Could not build GetDataBoxActivityStatus request "
                    "for %s box"),
                box_id_locale);
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

    /* Add mandatory XSD:tIdDbInput child */
    INSERT_STRING(request, BAD_CAST "dbID", box_id);
    /* Add times elements only when defined */
    /* ???: XML schema requires the values, textual documentation does not. */
    if (from_time) {
        err = timeval2timestring(from_time, &string);
        if (err) {
            isds_log_message(context,
                    _("Could not convert `from_time' argument to ISO time "
                        "string"));
            goto leave;
        }
        INSERT_STRING(request, "baFrom", string);
        zfree(string);
    }
    if (to_time) {
        err = timeval2timestring(to_time, &string);
        if (err) {
            isds_log_message(context,
                    _("Could not convert `to_time' argument to ISO time "
                        "string"));
            goto leave;
        }
        INSERT_STRING(request, "baTo", string);
        zfree(string);
    }

    /* Send request and check response*/
    err = send_destroy_request_check_response(context,
            SERVICE_DB_SEARCH, BAD_CAST "GetDataBoxActivityStatus",
            &request, &response, NULL, NULL);
    if (err) goto leave;


    /* Extract data */
    /* Set context to the root */
    xpath_ctx = xmlXPathNewContext(response);
    if (!xpath_ctx) {
        err = IE_ERROR;
        goto leave;
    }
    if (_isds_register_namespaces(xpath_ctx, MESSAGE_NS_UNSIGNED)) {
        err = IE_ERROR;
        goto leave;
    }
    result = xmlXPathEvalExpression(BAD_CAST "/isds:GetDataBoxActivityStatusResponse",
            xpath_ctx);
    if (!result) {
        err = IE_ERROR;
        goto leave;
    }
    if (xmlXPathNodeSetIsEmpty(result->nodesetval)) {
        isds_log_message(context, _("Missing GetDataBoxActivityStatusResponse element"));
        err = IE_ISDS;
        goto leave;
    }
    if (result->nodesetval->nodeNr > 1) {
        isds_log_message(context, _("Multiple GetDataBoxActivityStatusResponse element"));
        err = IE_ISDS;
        goto leave;
    }
    xpath_ctx->node = result->nodesetval->nodeTab[0];
    xmlXPathFreeObject(result); result = NULL;

    /* Ignore dbID, it's the same as the input argument. */

    /* Extract records */
    if (NULL == history) goto leave;
    result = xmlXPathEvalExpression(BAD_CAST "isds:Periods/isds:Period",
            xpath_ctx);
    if (!result) {
        err = IE_ERROR;
        goto leave;
    }
    if (!xmlXPathNodeSetIsEmpty(result->nodesetval)) {
        struct isds_list *prev_item = NULL;

        /* Iterate over all records */
        for (int i = 0; i < result->nodesetval->nodeNr; i++) {
            struct isds_list *item;

            /* Prepare structure */
            item = calloc(1, sizeof(*item));
            if (!item) {
                err = IE_NOMEM;
                goto leave;
            }
            item->destructor = (void(*)(void**))isds_box_state_period_free;
            if (i == 0) *history = item;
            else prev_item->next = item;
            prev_item = item;

            /* Extract it */
            xpath_ctx->node = result->nodesetval->nodeTab[i];
            err = extract_Period(context,
                    (struct isds_box_state_period **) (&item->data),
                    xpath_ctx);
            if (err) goto leave;
        }
    }

leave:
    if (!err) {
        isds_log(ILF_ISDS, ILL_DEBUG,
                _("GetDataBoxActivityStatus request for %s box "
                    "processed by server successfully.\n"), box_id_locale);
    }
    if (err) {
        isds_list_free(history);
    }

    free(box_id_locale);
    xmlXPathFreeObject(result);
    xmlXPathFreeContext(xpath_ctx);
    xmlFreeDoc(response);

#else /* not HAVE_LIBCURL */
    err = IE_NOTSUP;
#endif

    return err;
}


/* Get list of permissions to send commercial messages.
 * @context is ISDS session context.
 * @box_id is UTF-8 encoded sender box identifier as zero terminated string
 * @permissions is a reallocated list of permissions (struct
 * isds_commercial_permission*) to send commercial messages from @box_id. The
 * order of permissions is significant as the server applies the permissions
 * and associated pre-paid credits in the order. Empty list means no
 * permission.
 * @return:
 *  IE_SUCCESS if the list has been obtained correctly,
 *  or other appropriate error. */
isds_error isds_get_commercial_permissions(struct isds_ctx *context,
        const char *box_id, struct isds_list **permissions) {
    isds_error err = IE_SUCCESS;
#if HAVE_LIBCURL
    xmlDocPtr response = NULL;
    xmlXPathContextPtr xpath_ctx = NULL;
    xmlXPathObjectPtr result = NULL;
#endif

    if (!context) return IE_INVALID_CONTEXT;
    zfree(context->long_message);
    if (NULL == permissions) return IE_INVAL;
    isds_list_free(permissions);
    if (NULL == box_id) return IE_INVAL;

#if HAVE_LIBCURL
    /* Check if connection is established */
    if (!context->curl) return IE_CONNECTION_CLOSED;

    /* Do request and check for success */
    err = build_send_dbid_request_check_response(context,
            SERVICE_DB_SEARCH, BAD_CAST "PDZInfo", BAD_CAST "PDZSender",
            BAD_CAST box_id, NULL, &response, NULL);
    if (!err) {
        isds_log(ILF_ISDS, ILL_DEBUG,
                _("PDZInfo request processed by server successfully.\n"));
    }

    /* Extract data */
    /* Prepare structure */
    xpath_ctx = xmlXPathNewContext(response);
    if (!xpath_ctx) {
        err = IE_ERROR;
        goto leave;
    }
    if (_isds_register_namespaces(xpath_ctx, MESSAGE_NS_UNSIGNED)) {
        err = IE_ERROR;
        goto leave;
    }

    /* Set context node */
    result = xmlXPathEvalExpression(BAD_CAST
            "/isds:PDZInfoResponse/isds:dbPDZRecords/isds:dbPDZRecord",
            xpath_ctx);
    if (!result) {
        err = IE_ERROR;
        goto leave;
    }
    if (!xmlXPathNodeSetIsEmpty(result->nodesetval)) {
        struct isds_list *prev_item = NULL;

        /* Iterate over all permission records */
        for (int i = 0; i < result->nodesetval->nodeNr; i++) {
            struct isds_list *item;

            /* Prepare structure */
            item = calloc(1, sizeof(*item));
            if (!item) {
                err = IE_NOMEM;
                goto leave;
            }
            item->destructor = (void(*)(void**))isds_commercial_permission_free;
            if (i == 0) *permissions = item;
            else prev_item->next = item;
            prev_item = item;

            /* Extract it */
            xpath_ctx->node = result->nodesetval->nodeTab[i];
            err = extract_DbPDZRecord(context,
                    (struct isds_commercial_permission **) (&item->data),
                    xpath_ctx);
            if (err) goto leave;
        }
    }

leave:
    if (err) {
        isds_list_free(permissions);
    }

    xmlXPathFreeObject(result);
    xmlXPathFreeContext(xpath_ctx);
    xmlFreeDoc(response);

#else /* not HAVE_LIBCURL */
    err = IE_NOTSUP;
#endif

    return err;
}


/* Get details about credit for sending pre-paid commercial messages.
 * @context is ISDS session context.
 * @box_id is UTF-8 encoded sender box identifier as zero terminated string.
 * @from_date is first day of credit history to return in @history. Only
 * tm_year, tm_mon and tm_mday carry sane value.
 * @to_date is last day of credit history to return in @history. Only
 * tm_year, tm_mon and tm_mday carry sane value.
 * @credit outputs current credit value into pre-allocated memory. Pass NULL
 * if you don't care. This and all other credit values are integers in
 * hundredths of Czech Crowns.
 * @email outputs notification e-mail address where notifications about credit
 * are sent. This is automatically reallocated string. Pass NULL if you don't
 * care. It can return NULL if no address is defined.
 * @history outputs auto-reallocated list of pointers to struct
 * isds_credit_event. Events in closed interval @from_time to @to_time are
 * returned. Pass NULL @to_time and @from_time if you don't care. The events
 * are sorted by time.
 * @return:
 *  IE_SUCCESS if the credit details have been obtained correctly,
 *  or other appropriate error. Please note that server allows to retrieve
 *  only limited history of events. */
isds_error isds_get_commercial_credit(struct isds_ctx *context,
        const char *box_id,
        const struct tm *from_date, const struct tm *to_date,
        long int *credit, char **email, struct isds_list **history) {
    isds_error err = IE_SUCCESS;
#if HAVE_LIBCURL
    char *box_id_locale = NULL;
    xmlNodePtr request = NULL, node;
    xmlNsPtr isds_ns = NULL;
    xmlChar *string = NULL;

    xmlDocPtr response = NULL;
    xmlXPathContextPtr xpath_ctx = NULL;
    xmlXPathObjectPtr result = NULL;

    const xmlChar *codes[] = {
        BAD_CAST "1004",
        BAD_CAST "2011",
        BAD_CAST "1093",
        BAD_CAST "1137",
        BAD_CAST "1058",
        NULL
    };
    const char *meanings[] = {
        "Insufficient priviledges for the box",
        "The box does not exist",
        "Date is too long (history is not available after 15 months)",
        "Interval is too long (limit is 3 months)",
        "Invalid date"
    };
    const isds_error errors[] = {
        IE_ISDS,
        IE_NOEXIST,
        IE_DATE,
        IE_DATE,
        IE_DATE,
    };
    struct code_map_isds_error map = {
        .codes = codes,
        .meanings = meanings,
        .errors = errors
    };
#endif

    if (!context) return IE_INVALID_CONTEXT;
    zfree(context->long_message);

    /* Free output argument */
    if (NULL != credit) *credit = 0;
    if (NULL != email) zfree(*email);
    isds_list_free(history);

    if (NULL == box_id) return IE_INVAL;

#if HAVE_LIBCURL
    /* Check if connection is established */
    if (NULL == context->curl) return IE_CONNECTION_CLOSED;

    box_id_locale = _isds_utf82locale((char*)box_id);
    if (NULL == box_id_locale) {
        err = IE_NOMEM;
        goto leave;
    }

    /* Build request */
    request = xmlNewNode(NULL, BAD_CAST "DataBoxCreditInfo");
    if (NULL == request) {
        isds_printf_message(context,
                _("Could not build DataBoxCreditInfo request for %s box"),
                box_id_locale);
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

    /* Add mandatory XSD:tIdDbInput child */
    INSERT_STRING(request, BAD_CAST "dbID", box_id);
    /* Add mandatory dates elements with optional values */
    if (from_date) {
        err = tm2datestring(from_date, &string);
        if (err) {
            isds_log_message(context,
                    _("Could not convert `from_date' argument to ISO date "
                        "string"));
            goto leave;
        }
        INSERT_STRING(request, "ciFromDate", string);
        zfree(string);
    } else {
        INSERT_STRING(request, "ciFromDate", NULL);
    }
    if (to_date) {
        err = tm2datestring(to_date, &string);
        if (err) {
            isds_log_message(context,
                    _("Could not convert `to_date' argument to ISO date "
                        "string"));
            goto leave;
        }
        INSERT_STRING(request, "ciTodate", string);
        zfree(string);
    } else {
        INSERT_STRING(request, "ciTodate", NULL);
    }

    /* Send request and check response*/
    err = send_destroy_request_check_response(context,
            SERVICE_DB_SEARCH, BAD_CAST "DataBoxCreditInfo",
            &request, &response, NULL, &map);
    if (err) goto leave;


    /* Extract data */
    /* Set context to the root */
    xpath_ctx = xmlXPathNewContext(response);
    if (!xpath_ctx) {
        err = IE_ERROR;
        goto leave;
    }
    if (_isds_register_namespaces(xpath_ctx, MESSAGE_NS_UNSIGNED)) {
        err = IE_ERROR;
        goto leave;
    }
    result = xmlXPathEvalExpression(BAD_CAST "/isds:DataBoxCreditInfoResponse",
            xpath_ctx);
    if (!result) {
        err = IE_ERROR;
        goto leave;
    }
    if (xmlXPathNodeSetIsEmpty(result->nodesetval)) {
        isds_log_message(context, _("Missing DataBoxCreditInfoResponse element"));
        err = IE_ISDS;
        goto leave;
    }
    if (result->nodesetval->nodeNr > 1) {
        isds_log_message(context, _("Multiple DataBoxCreditInfoResponse element"));
        err = IE_ISDS;
        goto leave;
    }
    xpath_ctx->node = result->nodesetval->nodeTab[0];
    xmlXPathFreeObject(result); result = NULL;

    /* Extract common data */
    if (NULL != credit) EXTRACT_LONGINT("isds:currentCredit", credit, 1);
    if (NULL != email) EXTRACT_STRING("isds:notifEmail", *email);

    /* result gets overwritten in next step */
    xmlXPathFreeObject(result); result = NULL;

    /* Extract records */
    if (NULL == history) goto leave;
    result = xmlXPathEvalExpression(BAD_CAST "isds:ciRecords/isds:ciRecord",
            xpath_ctx);
    if (!result) {
        err = IE_ERROR;
        goto leave;
    }
    if (!xmlXPathNodeSetIsEmpty(result->nodesetval)) {
        struct isds_list *prev_item = NULL;

        /* Iterate over all records */
        for (int i = 0; i < result->nodesetval->nodeNr; i++) {
            struct isds_list *item;

            /* Prepare structure */
            item = calloc(1, sizeof(*item));
            if (!item) {
                err = IE_NOMEM;
                goto leave;
            }
            item->destructor = (void(*)(void**))isds_credit_event_free;
            if (i == 0) *history = item;
            else prev_item->next = item;
            prev_item = item;

            /* Extract it */
            xpath_ctx->node = result->nodesetval->nodeTab[i];
            err = extract_CiRecord(context,
                    (struct isds_credit_event **) (&item->data),
                    xpath_ctx);
            if (err) goto leave;
        }
    }

leave:
    if (!err) {
        isds_log(ILF_ISDS, ILL_DEBUG,
                _("DataBoxCreditInfo request processed by server successfully.\n"));
    }
    if (err) {
        isds_list_free(history);
        if (NULL != email) zfree(*email)
    }

    free(box_id_locale);
    xmlXPathFreeObject(result);
    xmlXPathFreeContext(xpath_ctx);
    xmlFreeDoc(response);

#else /* not HAVE_LIBCURL */
    err = IE_NOTSUP;
#endif

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
#if HAVE_LIBCURL
    xmlDocPtr response = NULL;
#endif

    if (!context) return IE_INVALID_CONTEXT;
    zfree(context->long_message);
    if (!service_name || *service_name == '\0' || !box_id) return IE_INVAL;

#if HAVE_LIBCURL
    /* Check if connection is established */
    if (!context->curl) return IE_CONNECTION_CLOSED;

    /* Do request and check for success */
    err = build_send_dbid_request_check_response(context,
            SERVICE_DB_MANIPULATION, service_name, NULL, box_id, approval,
            &response, refnumber);
    xmlFreeDoc(response);

    if (!err) {
        char *service_name_locale = _isds_utf82locale((char *) service_name);
        isds_log(ILF_ISDS, ILL_DEBUG,
                _("%s request processed by server successfully.\n"),
                service_name_locale);
        free(service_name_locale);
    }
#else /* not HAVE_LIBCURL */
    err = IE_NOTSUP;
#endif

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
#if HAVE_LIBCURL
    char *service_name_locale = NULL;
    xmlNodePtr request = NULL, db_owner_info;
    xmlNsPtr isds_ns = NULL;
#endif


    if (!context) return IE_INVALID_CONTEXT;
    zfree(context->long_message);
    if (!service_name || *service_name == '\0' || !owner) return IE_INVAL;

#if HAVE_LIBCURL
    service_name_locale = _isds_utf82locale((char*)service_name);
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
#else /* not HAVE_LIBCURL */
    err = IE_NOTSUP;
#endif

    return err;
}


/* Switch box accessibility state on request of box owner.
 * Despite the name, owner must do the request off-line. This function is
 * designed for such off-line meeting points (e.g. Czech POINT).
 * @context is ISDS session context.
 * @box identifies box to switch accessibility state.
 * @allow is true for making accessible, false to disallow access.
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
 * @box identifies box to switch accessibility state.
 * @since is date since accessibility has been denied. This can be past too.
 * Only tm_year, tm_mon and tm_mday carry sane value.
 * @approval is optional external approval of box manipulation
 * @refnumber is reallocated serial number of request assigned by ISDS. Use
 * NULL, if you don't care. */
isds_error isds_disable_box_accessibility_externaly(
        struct isds_ctx *context, const struct isds_DbOwnerInfo *box,
        const struct tm *since, const struct isds_approval *approval,
        char **refnumber) {
    isds_error err = IE_SUCCESS;
#if HAVE_LIBCURL
    char *service_name_locale = NULL;
    xmlNodePtr request = NULL, node;
    xmlNsPtr isds_ns = NULL;
    xmlChar *string = NULL;
#endif


    if (!context) return IE_INVALID_CONTEXT;
    zfree(context->long_message);
    if (!box || !since) return IE_INVAL;

#if HAVE_LIBCURL
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
#else /* not HAVE_LIBCURL */
    err = IE_NOTSUP;
#endif

    return err;
}


#if HAVE_LIBCURL
/* Insert struct isds_message data (envelope (recipient data optional) and
 * documents into XML tree
 * @context is session context
 * @outgoing_message is libisds structure with message data
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

    /* Insert optional message type */
    err = insert_message_type(context, outgoing_message->envelope->dmType,
            envelope);
    if (err) goto leave;

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
    INSERT_BOOLEAN(envelope, "dmPublishOwnID",
            outgoing_message->envelope->dmPublishOwnID);


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

    /* Check for document hierarchy */
    err = _isds_check_documents_hierarchy(context, outgoing_message->documents);
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
#endif /* HAVE_LIBCURL */


/* Send a message via ISDS to a recipient
 * @context is session context
 * @outgoing_message is message to send; Some members are mandatory (like
 * dbIDRecipient), some are optional and some are irrelevant (especially data
 * about sender). Included pointer to isds_list documents must contain at
 * least one document of FILEMETATYPE_MAIN. This is read-write structure, some
 * members will be filled with valid data from ISDS. Exact list of write
 * members is subject to change. Currently dmID is changed.
 * @return ISDS_SUCCESS, or other error code if something goes wrong. */
isds_error isds_send_message(struct isds_ctx *context,
        struct isds_message *outgoing_message) {

    isds_error err = IE_SUCCESS;
#if HAVE_LIBCURL
    xmlNsPtr isds_ns = NULL;
    xmlNodePtr request = NULL;
    xmlDocPtr response = NULL;
    xmlChar *code = NULL, *message = NULL;
    xmlXPathContextPtr xpath_ctx = NULL;
    xmlXPathObjectPtr result = NULL;
    /*_Bool message_is_complete = 0;*/
#endif

    if (!context) return IE_INVALID_CONTEXT;
    zfree(context->long_message);
    if (!outgoing_message) return IE_INVAL;

#if HAVE_LIBCURL
    /* Check if connection is established
     * TODO: This check should be done downstairs. */
    if (!context->curl) return IE_CONNECTION_CLOSED;


    /* Build CreateMessage request */
    request = xmlNewNode(NULL, BAD_CAST "CreateMessage");
    if (!request) {
        isds_log_message(context,
                _("Could not build CreateMessage request"));
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


    /* Signal we can serialize message since now */
    /*message_is_complete = 1;*/
    

    isds_log(ILF_ISDS, ILL_DEBUG, _("Sending CreateMessage request to ISDS\n"));

    /* Sent request */
    err = _isds(context, SERVICE_DM_OPERATIONS, request, &response, NULL, NULL);
   
    /* Don't' destroy request, we want to provide it to application later */

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
            _isds_utf82locale((char*)outgoing_message->envelope->dbIDRecipient);
        char *code_locale = _isds_utf82locale((char*)code);
        char *message_locale = _isds_utf82locale((char*)message);
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
    if (_isds_register_namespaces(xpath_ctx, MESSAGE_NS_UNSIGNED)) {
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
#else /* not HAVE_LIBCURL */
    err = IE_NOTSUP;
#endif

    return err;
}


/* Send a message via ISDS to a multiple recipients
 * @context is session context
 * @outgoing_message is message to send; Some members are mandatory,
 * some are optional and some are irrelevant (especially data
 * about sender). Data about recipient will be substituted by ISDS from
 * @copies. Included pointer to isds_list documents must
 * contain at least one document of FILEMETATYPE_MAIN.
 * @copies is list of isds_message_copy structures addressing all desired
 * recipients. This is read-write structure, some members will be filled with
 * valid data from ISDS (message IDs, error codes, error descriptions).
 * @return
 *  ISDS_SUCCESS if all messages have been sent
 *  ISDS_PARTIAL_SUCCESS if sending of some messages has failed (failed and
 *      succeeded messages can be identified by copies->data->error),
 *  or other error code if something other goes wrong. */
isds_error isds_send_message_to_multiple_recipients(struct isds_ctx *context,
        const struct isds_message *outgoing_message,
        struct isds_list *copies) {

    isds_error err = IE_SUCCESS;
#if HAVE_LIBCURL
    isds_error append_err;
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
#endif

    if (!context) return IE_INVALID_CONTEXT;
    zfree(context->long_message);
    if (!outgoing_message || !copies) return IE_INVAL;

#if HAVE_LIBCURL
    /* Check if connection is established
     * TODO: This check should be done downstairs. */
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
    err = _isds(context, SERVICE_DM_OPERATIONS, request, &response, NULL, NULL);
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
            _isds_utf82locale((char*)outgoing_message->envelope->dbIDRecipient);
        char *code_locale = _isds_utf82locale((char*)code);
        char *message_locale = _isds_utf82locale((char*)message);
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
            _isds_utf82locale((char*)outgoing_message->envelope->dbIDRecipient);
        char *code_locale = _isds_utf82locale((char*)code);
        char *message_locale = _isds_utf82locale((char*)message);
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
    if (_isds_register_namespaces(xpath_ctx, MESSAGE_NS_UNSIGNED)) {
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
#else /* not HAVE_LIBCURL */
    err = IE_NOTSUP;
#endif

    return err;
}


/* Get list of messages. This is common core for getting sent or received
 * messages.
 * Any criterion argument can be NULL, if you don't care about it.
 * @context is session context. Must not be NULL.
 * @outgoing_direction is true if you want list of outgoing messages,
 * it's false if you want incoming messages.
 * @from_time is minimal time and date of message sending inclusive.
 * @to_time is maximal time and date of message sending inclusive
 * @organization_unit_number is number of sender/recipient respectively.
 * @status_filter is bit field of isds_message_status values. Use special
 * value MESSAGESTATE_ANY to signal you don't care. (It's defined as union of
 * all values, you can use bit-wise arithmetic if you want.)
 * @offset is index of first message we are interested in. First message is 1.
 * Set to 0 (or 1) if you don't care.
 * @number is maximal length of list you want to get as input value, outputs
 * number of messages matching these criteria. Can be NULL if you don't care
 * (applies to output value either).
 * @messages is automatically reallocated list of isds_message's. Be ware that
 * it returns only brief overview (envelope and some other fields) about each
 * message, not the complete message. FIXME: Specify exact fields.
 * The list is sorted by delivery time in ascending order.
 * Use NULL if you don't care about don't need the data (useful if you want to
 * know only the @number). If you provide &NULL, list will be allocated on
 * heap, if you provide pointer to non-NULL, list will be freed automatically
 * at first. Also in case of error the list will be NULLed.
 * @return IE_SUCCESS or appropriate error code. */
static isds_error isds_get_list_of_messages(struct isds_ctx *context,
        _Bool outgoing_direction,
        const struct timeval *from_time, const struct timeval *to_time,
        const long int *organization_unit_number,
        const unsigned int status_filter,
        const unsigned long int offset, unsigned long int *number,
        struct isds_list **messages) {

    isds_error err = IE_SUCCESS;
#if HAVE_LIBCURL
    xmlNsPtr isds_ns = NULL;
    xmlNodePtr request = NULL, node;
    xmlDocPtr response = NULL;
    xmlChar *code = NULL, *message = NULL;
    xmlXPathContextPtr xpath_ctx = NULL;
    xmlXPathObjectPtr result = NULL;
    xmlChar *string = NULL;
    int count = 0;
#endif

    if (!context) return IE_INVALID_CONTEXT;
    zfree(context->long_message);
   
    /* Free former message list if any */
    if (messages) isds_list_free(messages);

#if HAVE_LIBCURL
    /* Check if connection is established
     * TODO: This check should be done downstairs. */
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
    err = _isds(context, SERVICE_DM_INFO, request, &response, NULL, NULL);
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
        char *code_locale = _isds_utf82locale((char*)code);
        char *message_locale = _isds_utf82locale((char*)message);
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
    if (_isds_register_namespaces(xpath_ctx, MESSAGE_NS_UNSIGNED)) {
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
#else /* not HAVE_LIBCURL */
    err = IE_NOTSUP;
#endif
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
 * all values, you can use bit-wise arithmetic if you want.)
 * @offset is index of first message we are interested in. First message is 1.
 * Set to 0 (or 1) if you don't care.
 * @number is maximal length of list you want to get as input value, outputs
 * number of messages matching these criteria. Can be NULL if you don't care
 * (applies to output value either).
 * @messages is automatically reallocated list of isds_message's. Be ware that
 * it returns only brief overview (envelope and some other fields) about each
 * message, not the complete message. FIXME: Specify exact fields.
 * The list is sorted by delivery time in ascending order.
 * Use NULL if you don't care about the meta data (useful if you want to know
 * only the @number). If you provide &NULL, list will be allocated on heap,
 * if you provide pointer to non-NULL, list will be freed automatically at
 * first. Also in case of error the list will be NULLed.
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
 * all values, you can use bit-wise arithmetic if you want.)
 * @offset is index of first message we are interested in. First message is 1.
 * Set to 0 (or 1) if you don't care.
 * @number is maximal length of list you want to get as input value, outputs
 * number of messages matching these criteria. Can be NULL if you don't care
 * (applies to output value either).
 * @messages is automatically reallocated list of isds_message's. Be ware that
 * it returns only brief overview (envelope and some other fields) about each
 * message, not the complete message. FIXME: Specify exact fields.
 * Use NULL if you don't care about the meta data (useful if you want to know
 * only the @number). If you provide &NULL, list will be allocated on heap,
 * if you provide pointer to non-NULL, list will be freed automatically at
 * first. Also in case of error the list will be NULLed.
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


/* Get list of sent message state changes.
 * Any criterion argument can be NULL, if you don't care about it.
 * @context is session context. Must not be NULL.
 * @from_time is minimal time and date of status changes inclusive
 * @to_time is maximal time and date of status changes inclusive
 * @changed_states is automatically reallocated list of
 * isds_message_status_change's. If you provide &NULL, list will be allocated
 * on heap, if you provide pointer to non-NULL, list will be freed
 * automatically at first. Also in case of error the list will be NULLed.
 * XXX: The list item ordering is not specified.
 * XXX: Server provides only `recent' changes.
 * @return IE_SUCCESS or appropriate error code. */
isds_error isds_get_list_of_sent_message_state_changes(
        struct isds_ctx *context,
        const struct timeval *from_time, const struct timeval *to_time,
        struct isds_list **changed_states) {

    isds_error err = IE_SUCCESS;
#if HAVE_LIBCURL
    xmlNsPtr isds_ns = NULL;
    xmlNodePtr request = NULL, node;
    xmlDocPtr response = NULL;
    xmlXPathContextPtr xpath_ctx = NULL;
    xmlXPathObjectPtr result = NULL;
    xmlChar *string = NULL;
    int count = 0;
#endif

    if (!context) return IE_INVALID_CONTEXT;
    zfree(context->long_message);
   
    /* Free former message list if any */
    isds_list_free(changed_states);

#if HAVE_LIBCURL
    /* Check if connection is established
     * TODO: This check should be done downstairs. */
    if (!context->curl) return IE_CONNECTION_CLOSED;

    /* Build GetMessageStateChanges request */
    request = xmlNewNode(NULL, BAD_CAST "GetMessageStateChanges");
    if (!request) {
        isds_log_message(context,
                _("Could not build GetMessageStateChanges request"));
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
    zfree(string);

    if (to_time) {
        err = timeval2timestring(to_time, &string);
        if (err) goto leave;
    }
    INSERT_STRING(request, "dmToTime", string);
    zfree(string);


    /* Sent request */
    err = send_destroy_request_check_response(context,
            SERVICE_DM_INFO, BAD_CAST "GetMessageStateChanges", &request,
            &response, NULL, NULL);
    if (err) goto leave;


    /* Extract data */
    xpath_ctx = xmlXPathNewContext(response);
    if (!xpath_ctx) {
        err = IE_ERROR;
        goto leave;
    }
    if (_isds_register_namespaces(xpath_ctx, MESSAGE_NS_UNSIGNED)) {
        err = IE_ERROR;
        goto leave;
    }
    result = xmlXPathEvalExpression(
                BAD_CAST "/isds:GetMessageStateChangesResponse/"
                "isds:dmRecords/isds:dmRecord", xpath_ctx);
    if (!result) {
        err = IE_ERROR;
        goto leave;
    }
    
    /* Fill output arguments in */
    if (!xmlXPathNodeSetIsEmpty(result->nodesetval)) {
        struct isds_list *item = NULL, *last_item = NULL;

        for (count = 0; count < result->nodesetval->nodeNr; count++) {
            /* Create new status change  */
            item = calloc(1, sizeof(*item));
            if (!item) {
                err = IE_NOMEM;
                goto leave;
            }
            item->destructor =
                (void(*)(void**)) &isds_message_status_change_free;

            /* Extract message status change */
            xpath_ctx->node = result->nodesetval->nodeTab[count];
            err = extract_StateChangesRecord(context,
                    (struct isds_message_status_change **) &item->data,
                    xpath_ctx);
            if (err) {
                isds_list_free(&item);
                goto leave;
            }

            /* Append new message status change into the list */
            if (!*changed_states) { 
                *changed_states = last_item = item;
            } else {
                last_item->next = item;
                last_item = item;
            }
        }
    }

leave:
    if (err) {
        isds_list_free(changed_states);
    }

    free(string);
    xmlXPathFreeObject(result);
    xmlXPathFreeContext(xpath_ctx);
    xmlFreeDoc(response);
    xmlFreeNode(request);

    if (!err)
        isds_log(ILF_ISDS, ILL_DEBUG,
                _("GetMessageStateChanges request processed by server "
                    "successfully.\n"));
#else /* not HAVE_LIBCURL */
    err = IE_NOTSUP;
#endif
    return err;
}


#if HAVE_LIBCURL
/* Build ISDS request of XSD tIDMessInput type, sent it and check for error
 * code
 * @context is session context
 * @service is ISDS WS service handler
 * @service_name is name of SERVICE_DM_OPERATIONS
 * @message_id is message ID to send as service argument to ISDS
 * @response is reallocated server SOAP body response as XML document
 * @raw_response is reallocated bit stream with response body. Use
 * NULL if you don't care
 * @raw_response_length is size of @raw_response in bytes
 * @code is reallocated ISDS status code
 * @status_message is reallocated ISDS status message
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
    zfree(*code);
    zfree(*status_message);


    /* Check if connection is established
     * TODO: This check should be done downstairs. */
    if (!context->curl) return IE_CONNECTION_CLOSED;

    service_name_locale = _isds_utf82locale((char*)service_name);
    message_id_locale = _isds_utf82locale(message_id);
    if (!service_name_locale || !message_id_locale) {
        err = IE_NOMEM;
        goto leave;
    }

    /* Build request */
    request = xmlNewNode(NULL, service_name);
    if (!request) {
        isds_printf_message(context,
                _("Could not build %s request for %s message ID"),
                service_name_locale, message_id_locale);
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
    err = _isds(context, service, request, response,
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
        char *code_locale = _isds_utf82locale((char*) *code);
        char *status_message_locale = _isds_utf82locale((char*) *status_message);
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
 * @message_id is UTF-8 encoded message ID for logging purpose
 * @response is parsed XML document. It will be freed and NULLed in the middle
 * of function run to save memory. This is not guaranteed in case of error.
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
    xpath_expression = _isds_astrcat3("/isds:", (char *) request_name,
            "Response/isds:dmSignature");
    if (!xpath_expression) return IE_NOMEM;
   
    /* Extract data */
    xpath_ctx = xmlXPathNewContext(*response);
    if (!xpath_ctx) {
        err = IE_ERROR;
        goto leave;
    }
    if (_isds_register_namespaces(xpath_ctx, MESSAGE_NS_UNSIGNED)) {
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
        char *message_id_locale = _isds_utf82locale((char*) message_id);
        isds_printf_message(context,
                _("Server did not return any signed data for message ID `%s' "
                    "on %s request"),
                message_id_locale, request_name);
        free(message_id_locale);
        err = IE_ISDS;
        goto leave;
    }
    /* More responses */
    if (result->nodesetval->nodeNr > 1) {
        char *message_id_locale = _isds_utf82locale((char*) message_id);
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
    *raw_length = _isds_b64decode(encoded_structure, raw);
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
#endif /* HAVE_LIBCURL */


/* Download incoming message envelope identified by ID.
 * @context is session context
 * @message_id is message identifier (you can get them from
 * isds_get_list_of_received_messages())
 * @message is automatically reallocated message retrieved from ISDS.
 * It will miss documents per se. Use isds_get_received_message(), if you are
 * interested in documents (content) too.
 * Returned hash and timestamp require documents to be verifiable. */
isds_error isds_get_received_envelope(struct isds_ctx *context,
        const char *message_id, struct isds_message **message) {

    isds_error err = IE_SUCCESS;
#if HAVE_LIBCURL
    xmlDocPtr response = NULL;
    xmlChar *code = NULL, *status_message = NULL;
    xmlXPathContextPtr xpath_ctx = NULL;
    xmlXPathObjectPtr result = NULL;
#endif

    if (!context) return IE_INVALID_CONTEXT;
    zfree(context->long_message);
   
    /* Free former message if any */
    if (!message) return IE_INVAL;
    isds_message_free(message);

#if HAVE_LIBCURL
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
    if (_isds_register_namespaces(xpath_ctx, MESSAGE_NS_UNSIGNED)) {
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
        char *message_id_locale = _isds_utf82locale((char*) message_id);
        isds_printf_message(context,
                _("Server did not return any envelope for ID `%s' "
                    "on MessageEnvelopeDownload request"), message_id_locale);
        free(message_id_locale);
        err = IE_ISDS;
        goto leave;
    }
    /* More envelops */
    if (result->nodesetval->nodeNr > 1) {
        char *message_id_locale = _isds_utf82locale((char*) message_id);
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
    if (!*message || !(*message)->xml) {
        xmlFreeDoc(response);
    }

    if (!err)
        isds_log(ILF_ISDS, ILL_DEBUG,
                    _("MessageEnvelopeDownload request processed by server "
                        "successfully.\n")
                );
#else /* not HAVE_LIBCURL */
    err = IE_NOTSUP;
#endif
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
            err = _isds_extract_cms_data(context, buffer, length,
                    &xml_stream, &xml_stream_length);
            if (err) goto leave;
            break;

        default:
            isds_log_message(context, _("Bad raw delivery representation type"));
            return IE_INVAL;
            break;
    }

    if (_isds_sizet2int(xml_stream_length) >= 0) {
        isds_log(ILF_ISDS, ILL_DEBUG,
            _("Delivery info content:\n%.*s\nEnd of delivery info\n"),
            _isds_sizet2int(xml_stream_length), xml_stream);
    }

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
    if (_isds_register_namespaces(xpath_ctx, message_ns)) {
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
                _("XML document is not sisds:dmDelivery document"));
        err = IE_ISDS;
        goto leave;
    }
    /* More delivery info's */
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
    if (!*message || !(*message)->xml) {
        xmlFreeDoc(message_doc);
    }
    if (xml_stream != buffer) _isds_cms_data_free(xml_stream);

    if (!err)
        isds_log(ILF_ISDS, ILL_DEBUG,
                _("Delivery info loaded successfully.\n"));
    return err;
}


/* Download signed delivery info-sheet of given message identified by ID.
 * @context is session context
 * @message_id is message identifier (you can get them from
 * isds_get_list_of_{sent,received}_messages())
 * @message is automatically reallocated message retrieved from ISDS.
 * It will miss documents per se. Use isds_get_signed_received_message(),
 * if you are interested in documents (content). OTOH, only this function
 * can get list events message has gone through. */
isds_error isds_get_signed_delivery_info(struct isds_ctx *context,
        const char *message_id, struct isds_message **message) {

    isds_error err = IE_SUCCESS;
#if HAVE_LIBCURL
    xmlDocPtr response = NULL;
    xmlChar *code = NULL, *status_message = NULL;
    void *raw = NULL;
    size_t raw_length = 0;
#endif

    if (!context) return IE_INVALID_CONTEXT;
    zfree(context->long_message);
   
    /* Free former message if any */
    if (!message) return IE_INVAL;
    isds_message_free(message);

#if HAVE_LIBCURL
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
#else /* not HAVE_LIBCURL */
    err = IE_NOTSUP;
#endif
    return err;
}


/* Download delivery info-sheet of given message identified by ID.
 * @context is session context
 * @message_id is message identifier (you can get them from
 * isds_get_list_of_{sent,received}_messages())
 * @message is automatically reallocated message retrieved from ISDS.
 * It will miss documents per se. Use isds_get_received_message(), if you are
 * interested in documents (content). OTOH, only this function can get list
 * of events message has gone through. */
isds_error isds_get_delivery_info(struct isds_ctx *context,
        const char *message_id, struct isds_message **message) {

    isds_error err = IE_SUCCESS;
#if HAVE_LIBCURL
    xmlDocPtr response = NULL;
    xmlChar *code = NULL, *status_message = NULL;
    xmlNodePtr delivery_node = NULL;
    void *raw = NULL;
    size_t raw_length = 0;
#endif

    if (!context) return IE_INVALID_CONTEXT;
    zfree(context->long_message);
   
    /* Free former message if any */
    if (!message) return IE_INVAL;
    isds_message_free(message);

#if HAVE_LIBCURL
    /* Do request and check for success */
    err = build_send_check_message_request(context, SERVICE_DM_INFO,
            BAD_CAST "GetDeliveryInfo", message_id,
            &response, NULL, NULL, &code, &status_message);
    if (err) goto leave;


    /* Serialize delivery info */
    delivery_node = xmlDocGetRootElement(response);
    if (!delivery_node) {
        char *message_id_locale = _isds_utf82locale((char*) message_id);
        isds_printf_message(context,
                _("Server did not return any delivery info for ID `%s' "
                    "on GetDeliveryInfo request"), message_id_locale);
        free(message_id_locale);
        err = IE_ISDS;
        goto leave;
    }
    err = serialize_subtree(context, delivery_node, &raw, &raw_length);
    if (err) goto leave;

    /* Parse delivery info */
    /* TODO: Here we parse the response second time. We could single delivery
     * parser from isds_load_delivery_info() to make things faster. */
    err = isds_load_delivery_info(context,
            RAWTYPE_DELIVERYINFO, raw, raw_length,
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
                    _("GetDeliveryInfo request processed by server "
                        "successfully.\n")
                );
#else /* not HAVE_LIBCURL */
    err = IE_NOTSUP;
#endif
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
#if HAVE_LIBCURL
    xmlDocPtr response = NULL;
    void *xml_stream = NULL;
    size_t xml_stream_length;
    xmlChar *code = NULL, *status_message = NULL;
    xmlXPathContextPtr xpath_ctx = NULL;
    xmlXPathObjectPtr result = NULL;
    char *phys_path = NULL;
    size_t phys_start, phys_end;
#endif

    if (!context) return IE_INVALID_CONTEXT;
    zfree(context->long_message);
   
    /* Free former message if any */
    if (NULL == message) return IE_INVAL;
    if (message) isds_message_free(message);

#if HAVE_LIBCURL
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
    if (_isds_register_namespaces(xpath_ctx, MESSAGE_NS_UNSIGNED)) {
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
        char *message_id_locale = _isds_utf82locale((char*) message_id);
        isds_printf_message(context,
                _("Server did not return any message for ID `%s' "
                    "on MessageDownload request"), message_id_locale);
        free(message_id_locale);
        err = IE_ISDS;
        goto leave;
    }
    /* More messages */
    if (result->nodesetval->nodeNr > 1) {
        char *message_id_locale = _isds_utf82locale((char*) message_id);
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
    err = _isds_find_element_boundary(xml_stream, xml_stream_length,
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
    if (!*message || !(*message)->xml) {
        xmlFreeDoc(response);
    }

    if (!err)
        isds_log(ILF_ISDS, ILL_DEBUG,
                    _("MessageDownload request processed by server "
                        "successfully.\n")
                );
#else /* not HAVE_LIBCURL */
    err = IE_NOTSUP;
#endif
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
            err = _isds_extract_cms_data(context, buffer, length,
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
            err = _isds_extract_cms_data(context, buffer, length,
                    &xml_stream, &xml_stream_length);
            if (err) goto leave;
            break;

        default:
            isds_log_message(context, _("Bad raw message representation type"));
            return IE_INVAL;
            break;
    }

    if (_isds_sizet2int(xml_stream_length) >= 0) {
        isds_log(ILF_ISDS, ILL_DEBUG,
            _("Loading message:\n%.*s\nEnd of message\n"),
            _isds_sizet2int(xml_stream_length), xml_stream);
    }

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
    if (_isds_register_namespaces(xpath_ctx, message_ns)) {
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

    if (xml_stream != buffer) _isds_cms_data_free(xml_stream);
    xmlXPathFreeObject(result);
    xmlXPathFreeContext(xpath_ctx);
    if (!*message || !(*message)->xml) {
        xmlFreeDoc(message_doc);
    }

    if (!err)
        isds_log(ILF_ISDS, ILL_DEBUG, _("Message loaded successfully.\n"));
    return err;
}


/* Determine type of raw message or delivery info according some heuristics.
 * It does not validate the raw blob.
 * @context is session context
 * @raw_type returns content type of @buffer. Valid only if exit code of this
 * function is IE_SUCCESS. The pointer must be valid. This is no automatically
 * reallocated memory.
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
    err = _isds_extract_cms_data(context, buffer, length,
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
                _("Unknown name space"));
        err = IE_NOTSUP;
    }

leave:
    if (xml_stream != buffer) _isds_cms_data_free(xml_stream);
    xmlFreeDoc(document);
    return err;
}


/* Download signed incoming/outgoing message identified by ID.
 * @context is session context
 * @output is true for outgoing message, false for incoming message
 * @message_id is message identifier (you can get them from
 * isds_get_list_of_{sent,received}_messages())
 * @message is automatically reallocated message retrieved from ISDS. The raw
 * member will be filled with PKCS#7 structure in DER format. */
static isds_error isds_get_signed_message(struct isds_ctx *context,
        const _Bool outgoing, const char *message_id,
        struct isds_message **message) {

    isds_error err = IE_SUCCESS;
#if HAVE_LIBCURL
    xmlDocPtr response = NULL;
    xmlChar *code = NULL, *status_message = NULL;
    xmlXPathContextPtr xpath_ctx = NULL;
    xmlXPathObjectPtr result = NULL;
    char *encoded_structure = NULL;
    void *raw = NULL;
    size_t raw_length = 0;
#endif

    if (!context) return IE_INVALID_CONTEXT;
    zfree(context->long_message);
    if (!message) return IE_INVAL;
    isds_message_free(message);

#if HAVE_LIBCURL
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
#else /* not HAVE_LIBCURL */
    err = IE_NOTSUP;
#endif
    return err;
}


/* Download signed incoming message identified by ID.
 * @context is session context
 * @message_id is message identifier (you can get them from
 * isds_get_list_of_received_messages())
 * @message is automatically reallocated message retrieved from ISDS. The raw
 * member will be filled with PKCS#7 structure in DER format. */
isds_error isds_get_signed_received_message(struct isds_ctx *context,
        const char *message_id, struct isds_message **message) {
    return isds_get_signed_message(context, 0, message_id, message);
}


/* Download signed outgoing message identified by ID.
 * @context is session context
 * @message_id is message identifier (you can get them from
 * isds_get_list_of_sent_messages())
 * @message is automatically reallocated message retrieved from ISDS. The raw
 * member will be filled with PKCS#7 structure in DER format. */
isds_error isds_get_signed_sent_message(struct isds_ctx *context,
        const char *message_id, struct isds_message **message) {
    return isds_get_signed_message(context, 1, message_id, message);
}


/* Get type and name of user who sent a message identified by ID.
 * @context is session context
 * @message_id is message identifier
 * @sender_type is pointer to automatically allocated type of sender detected
 * from @raw_sender_type string. If @raw_sender_type is unknown to this
 * library or to the server, NULL will be returned. Pass NULL if you don't
 * care about it.
 * @raw_sender_type is automatically reallocated UTF-8 string describing
 * sender type or NULL if not known to server. Pass NULL if you don't care.
 * @sender_name is automatically reallocated UTF-8 name of user who sent the
 * message, or NULL if not known to ISDS. Pass NULL if you don't care. */
isds_error isds_get_message_sender(struct isds_ctx *context,
        const char *message_id, isds_sender_type **sender_type,
        char **raw_sender_type, char **sender_name) {
    isds_error err = IE_SUCCESS;
#if HAVE_LIBCURL
    xmlDocPtr response = NULL;
    xmlChar *code = NULL, *status_message = NULL;
    xmlXPathContextPtr xpath_ctx = NULL;
    xmlXPathObjectPtr result = NULL;
    char *type_string = NULL;
#endif

    if (!context) return IE_INVALID_CONTEXT;
    zfree(context->long_message);
    if (sender_type) zfree(*sender_type);
    if (raw_sender_type) zfree(*raw_sender_type);
    if (sender_name) zfree(*sender_name);
    if (!message_id) return IE_INVAL;

#if HAVE_LIBCURL
    /* Do request and check for success */
    err = build_send_check_message_request(context, SERVICE_DM_INFO,
            BAD_CAST "GetMessageAuthor",
            message_id, &response, NULL, NULL, &code, &status_message);
    if (err) goto leave;

    /* Extract data */
    xpath_ctx = xmlXPathNewContext(response);
    if (!xpath_ctx) {
        err = IE_ERROR;
        goto leave;
    }
    if (_isds_register_namespaces(xpath_ctx, MESSAGE_NS_UNSIGNED)) {
        err = IE_ERROR;
        goto leave;
    }
    result = xmlXPathEvalExpression(
                BAD_CAST "/isds:GetMessageAuthorResponse", xpath_ctx);
    if (!result) {
        err = IE_ERROR;
        goto leave;
    }
    if (xmlXPathNodeSetIsEmpty(result->nodesetval)) {
        isds_log_message(context,
                _("Missing GetMessageAuthorResponse element"));
        err = IE_ISDS;
        goto leave;
    }
    if (result->nodesetval->nodeNr > 1) {
        isds_log_message(context,
                _("Multiple GetMessageAuthorResponse element"));
        err = IE_ISDS;
        goto leave;
    }
    xpath_ctx->node = result->nodesetval->nodeTab[0];
    xmlXPathFreeObject(result); result = NULL;

    /* Fill output arguments in */
    EXTRACT_STRING("isds:userType", type_string);
    if (NULL != type_string) {
        if (NULL != sender_type) {
            *sender_type = calloc(1, sizeof(**sender_type));
            if (NULL == *sender_type) {
                err = IE_NOMEM;
                goto leave;
            }

            err = string2isds_sender_type((xmlChar *)type_string,
                    *sender_type);
            if (err) {
                zfree(*sender_type);
                if (err == IE_ENUM) {
                    err = IE_SUCCESS;
                    char *type_string_locale = _isds_utf82locale(type_string);
                    isds_log(ILF_ISDS, ILL_WARNING,
                            _("Unknown isds:userType value: %s"),
                            type_string_locale);
                    free(type_string_locale);
                }
            }
        }
    }
    if (NULL == raw_sender_type)
        zfree(type_string);
    if (NULL != sender_name)
        EXTRACT_STRING("isds:authorName", *sender_name);

leave:
    if (err) {
        if (NULL != sender_type) zfree(*sender_type);
        zfree(type_string);
        if (NULL != sender_name) zfree(*sender_name);
    }
    if (NULL != raw_sender_type) *raw_sender_type = type_string;

    xmlXPathFreeObject(result);
    xmlXPathFreeContext(xpath_ctx);

    free(code);
    free(status_message);
    xmlFreeDoc(response);

    if (!err)
        isds_log(ILF_ISDS, ILL_DEBUG,
                _("GetMessageAuthor request processed by server "
                    "successfully.\n"));
#else /* not HAVE_LIBCURL */
    err = IE_NOTSUP;
#endif
    return err;
}


/* Retrieve hash of message identified by ID stored in ISDS.
 * @context is session context
 * @message_id is message identifier
 * @hash is automatically reallocated message hash downloaded from ISDS.
 * Message must exist in system and must not be deleted. */
isds_error isds_download_message_hash(struct isds_ctx *context,
        const char *message_id, struct isds_hash **hash) {

    isds_error err = IE_SUCCESS;
#if HAVE_LIBCURL
    xmlDocPtr response = NULL;
    xmlChar *code = NULL, *status_message = NULL;
    xmlXPathContextPtr xpath_ctx = NULL;
    xmlXPathObjectPtr result = NULL;
#endif

    if (!context) return IE_INVALID_CONTEXT;
    zfree(context->long_message);
   
    isds_hash_free(hash);

#if HAVE_LIBCURL
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
    if (_isds_register_namespaces(xpath_ctx, MESSAGE_NS_UNSIGNED)) {
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
        char *message_id_locale = _isds_utf82locale((char*) message_id);
        isds_printf_message(context,
                _("Server did not return any response for ID `%s' "
                    "on VerifyMessage request"), message_id_locale);
        free(message_id_locale);
        err = IE_ISDS;
        goto leave;
    }
    /* More responses */
    if (result->nodesetval->nodeNr > 1) {
        char *message_id_locale = _isds_utf82locale((char*) message_id);
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
#else /* not HAVE_LIBCURL */
    err = IE_NOTSUP;
#endif
    return err;
}


/* Erase message specified by @message_id from long term storage. Other
 * message cannot be erased on user request.
 * @context is session context
 * @message_id is message identifier.
 * @incoming is true for incoming message, false for outgoing message.
 * @return
 *  IE_SUCCESS  if message has ben removed
 *  IE_INVAL    if message does not exist in long term storage or message
 *              belongs to different box
 * TODO: IE_NOEPRM  if user has no permission to erase a message */
isds_error isds_delete_message_from_storage(struct isds_ctx *context,
        const char *message_id, _Bool incoming) {
    isds_error err = IE_SUCCESS;
#if HAVE_LIBCURL
    xmlNodePtr request = NULL, node;
    xmlNsPtr isds_ns = NULL;
    xmlDocPtr response = NULL;
    xmlChar *code = NULL, *status_message = NULL;
#endif

    if (!context) return IE_INVALID_CONTEXT;
    zfree(context->long_message);
    if (NULL == message_id) return IE_INVAL;
   
#if HAVE_LIBCURL
    /* Check if connection is established
     * TODO: This check should be done downstairs. */
    if (!context->curl) return IE_CONNECTION_CLOSED;

    /* Build request */
    request = xmlNewNode(NULL, BAD_CAST "EraseMessage");
    if (!request) {
        isds_log_message(context,
                _("Could build EraseMessage request"));
        return IE_ERROR;
    }
    isds_ns = xmlNewNs(request, BAD_CAST ISDS_NS, NULL);
    if(!isds_ns) {
        isds_log_message(context, _("Could not create ISDS name space"));
        xmlFreeNode(request);
        return IE_ERROR;
    }
    xmlSetNs(request, isds_ns);

    err = validate_message_id_length(context, (xmlChar *) message_id);
    if (err) goto leave;
    INSERT_STRING(request, "dmID", message_id);

    INSERT_SCALAR_BOOLEAN(request, "dmIncoming", incoming);


    /* Send request */
    isds_log(ILF_ISDS, ILL_DEBUG, _("Sending EraseMessage request for "
                "message ID %s to ISDS\n"), message_id);
    err = _isds(context, SERVICE_DM_INFO, request, &response, NULL, NULL);
    xmlFreeNode(request); request = NULL;
    
    if (err) {
        isds_log(ILF_ISDS, ILL_DEBUG,
                    _("Processing ISDS response on EraseMessage request "
                        "failed\n"));
        goto leave;
    }

    /* Check for response status */
    err = isds_response_status(context, SERVICE_DM_INFO, response,
            &code, &status_message, NULL);
    if (err) {
        isds_log(ILF_ISDS, ILL_DEBUG,
                    _("ISDS response on EraseMessage request is missing "
                        "status\n"));
        goto leave;
    }

    /* Check server status code */
    if (!xmlStrcmp(code, BAD_CAST "1211")) {
        isds_log_message(context, _("Message to erase belongs to other box"));
        err = IE_INVAL;
    } else if (!xmlStrcmp(code, BAD_CAST "1219")) {
        isds_log_message(context, _("Message to erase is not saved in "
                    "long term storage or the direction does not match"));
        err = IE_INVAL;
    } else if (xmlStrcmp(code, BAD_CAST "0000")) {
        char *code_locale = _isds_utf82locale((char*) code);
        char *message_locale = _isds_utf82locale((char*) status_message);
        isds_log(ILF_ISDS, ILL_DEBUG,
                    _("Server refused EraseMessage request "
                        "(code=%s, message=%s)\n"),
                code_locale, message_locale);
        isds_log_message(context, message_locale);
        free(code_locale);
        free(message_locale);
        err = IE_ISDS;
        goto leave;
    }

leave:
    free(code);
    free(status_message);
    xmlFreeDoc(response);
    xmlFreeNode(request);

    if (!err)
        isds_log(ILF_ISDS, ILL_DEBUG,
                    _("EraseMessage request processed by server "
                        "successfully.\n")
                );
#else /* not HAVE_LIBCURL */
    err = IE_NOTSUP;
#endif
    return err;
}


/* Mark message as read. This is a transactional commit function to acknowledge
 * to ISDS the message has been downloaded and processed by client properly.
 * @context is session context
 * @message_id is message identifier. */
isds_error isds_mark_message_read(struct isds_ctx *context,
        const char *message_id) {

    isds_error err = IE_SUCCESS;
#if HAVE_LIBCURL
    xmlDocPtr response = NULL;
    xmlChar *code = NULL, *status_message = NULL;
#endif

    if (!context) return IE_INVALID_CONTEXT;
    zfree(context->long_message);
   
#if HAVE_LIBCURL
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
#else /* not HAVE_LIBCURL */
    err = IE_NOTSUP;
#endif
    return err;
}


/* Mark message as received by recipient. This is applicable only to
 * commercial message. Use envelope->dmType message member to distinguish
 * commercial message from government message. Government message is
 * received automatically (by law), commercial message on recipient request.
 * @context is session context
 * @message_id is message identifier. */
isds_error isds_mark_message_received(struct isds_ctx *context,
        const char *message_id) {

    isds_error err = IE_SUCCESS;
#if HAVE_LIBCURL
    xmlDocPtr response = NULL;
    xmlChar *code = NULL, *status_message = NULL;
#endif

    if (!context) return IE_INVALID_CONTEXT;
    zfree(context->long_message);
   
#if HAVE_LIBCURL
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
#else /* not HAVE_LIBCURL */
    err = IE_NOTSUP;
#endif
    return err;
}


/* Send document for authorized conversion into Czech POINT system.
 * This is public anonymous service, no log-in necessary. Special context is
 * used to reuse keep-a-live HTTPS connection.
 * @context is Czech POINT session context. DO NOT use context connected to
 * ISDS server. Use new context or context used by this function previously.
 * @document is document to convert. Only data, data_length, dmFileDescr and
 * is_xml members are significant. Be ware that not all document formats can be
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
#if HAVE_LIBCURL
    xmlNsPtr deposit_ns = NULL, empty_ns = NULL;
    xmlNodePtr request = NULL, node;
    xmlDocPtr response = NULL;

    xmlXPathContextPtr xpath_ctx = NULL;
    xmlXPathObjectPtr result = NULL;
    long int status = -1;
    long int *status_ptr = &status;
    char *string = NULL;
#endif


    if (!context) return IE_INVALID_CONTEXT;
    zfree(context->long_message);
    if (!document || !id || !date) return IE_INVAL;

    if (document->is_xml) {
        isds_log_message(context,
                _("XML documents cannot be submitted to conversion"));
        return IE_NOTSUP;
    }

    /* Free output arguments */
    zfree(*id);
    zfree(*date);

#if HAVE_LIBCURL
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

    /* Insert children. They are in empty namespace! */
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
    err = insert_base64_encoded_string(context, request, empty_ns, "document",
            document->data, document->data_length);
    if (err) goto leave;

    isds_log(ILF_ISDS, ILL_DEBUG,
            _("Submitting document for conversion into Czech POINT deposit"));

    /* Send conversion request */
    err = _czp_czpdeposit(context, request, &response);
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
    if (_isds_register_namespaces(xpath_ctx, MESSAGE_NS_UNSIGNED)) {
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
        char *string_locale = _isds_utf82locale(string);
        isds_printf_message(context,
                _("Czech POINT deposit refused document for conversion "
                    "(code=%ld, message=%s)"),
                status, string_locale);
        free(string_locale);
        err = IE_ISDS;
        goto leave;
    }

    /* Get document ID */
    EXTRACT_STRING("documentID", *id);

    /* Get submit date */
    EXTRACT_STRING("dateInserted", string);
    if (string) {
        *date = calloc(1, sizeof(**date));
        if (!*date) {
            err = IE_NOMEM;
            goto leave;
        }
        err = _isds_datestring2tm((xmlChar *)string, *date);
        if (err) {
            if (err == IE_NOTSUP) {
                err = IE_ISDS;
                char *string_locale = _isds_utf82locale(string);
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
    xmlFreeNode(request);

    if (!err) {
        char *id_locale = _isds_utf82locale((char *) *id); 
        isds_log(ILF_ISDS, ILL_DEBUG,
                _("Document %s has been submitted for conversion "
                    "to server successfully\n"), id_locale);
        free(id_locale);
    }
#else /* not HAVE_LIBCURL */
    err = IE_NOTSUP;
#endif
    return err;
}


/* Close possibly opened connection to Czech POINT document deposit.
 * @context is Czech POINT session context. */
isds_error czp_close_connection(struct isds_ctx *context) {
    if (!context) return IE_INVALID_CONTEXT;
    zfree(context->long_message);
#if HAVE_LIBCURL
    return czp_do_close_connection(context);
#else
    return IE_NOTSUP;
#endif
}


/* Send request for new box creation in testing ISDS instance.
 * It's not possible to request for a production box currently, as it
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
 * @former_names is former name of box owner. Pass NULL if you don't care.
 * @approval is optional external approval of box manipulation
 * @refnumber is reallocated serial number of request assigned by ISDS. Use
 * NULL, if you don't care.*/
isds_error isds_request_new_testing_box(struct isds_ctx *context,
        struct isds_DbOwnerInfo *box, const struct isds_list *users,
        const char *former_names, const struct isds_approval *approval,
        char **refnumber) {
    isds_error err = IE_SUCCESS;
#if HAVE_LIBCURL
    xmlNodePtr request = NULL;
    xmlDocPtr response = NULL;
    xmlXPathContextPtr xpath_ctx = NULL;
    xmlXPathObjectPtr result = NULL;
#endif


    if (!context) return IE_INVALID_CONTEXT;
    zfree(context->long_message);
    if (!box) return IE_INVAL;

#if HAVE_LIBCURL
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
            box, users, (xmlChar *) former_names, NULL, NULL, NULL, approval);
    if (err) goto leave;

    /* Send it to server and process response */
    err = send_destroy_request_check_response(context,
            SERVICE_DB_MANIPULATION, BAD_CAST "CreateDataBox", &request,
            &response, (xmlChar **) refnumber, NULL);
    if (err) goto leave;

    /* Extract box ID */
    xpath_ctx = xmlXPathNewContext(response);
    if (!xpath_ctx) {
        err = IE_ERROR;
        goto leave;
    }
    if (_isds_register_namespaces(xpath_ctx, MESSAGE_NS_UNSIGNED)) {
        err = IE_ERROR;
        goto leave;
    }
    EXTRACT_STRING("/isds:CreateDataBoxResponse/isds:dbID", box->dbID);

leave:
    xmlXPathFreeObject(result);
    xmlXPathFreeContext(xpath_ctx);
    xmlFreeDoc(response);
    xmlFreeNode(request);

    if (!err) {
        isds_log(ILF_ISDS, ILL_DEBUG,
                _("CreateDataBox request processed by server successfully.\n"));
    }
#else /* not HAVE_LIBCURL */
    err = IE_NOTSUP;
#endif

    return err;
}


/* Submit CMS signed message to ISDS to verify its originality. This is
 * stronger form of isds_verify_message_hash() because ISDS does more checks
 * than simple one (potentialy old weak) hash comparison.
 * @context is session context
 * @message is memory with raw CMS signed message bit stream
 * @length is @message size in bytes
 * @return
 *  IE_SUCCESS  if message originates in ISDS
 *  IE_NOTEQUAL if message is unknown to ISDS
 *  other code  for other errors */
isds_error isds_authenticate_message(struct isds_ctx *context,
        const void *message, size_t length) {
    isds_error err = IE_SUCCESS;
#if HAVE_LIBCURL
    xmlNsPtr isds_ns = NULL;
    xmlNodePtr request = NULL;
    xmlDocPtr response = NULL;
    xmlXPathContextPtr xpath_ctx = NULL;
    xmlXPathObjectPtr result = NULL;
    _Bool *authentic = NULL;
#endif

    if (!context) return IE_INVALID_CONTEXT;
    zfree(context->long_message);
    if (!message || length == 0) return IE_INVAL;

#if HAVE_LIBCURL
    /* Check if connection is established
     * TODO: This check should be done downstairs. */
    if (!context->curl) return IE_CONNECTION_CLOSED;


    /* Build AuthenticateMessage request */
    request = xmlNewNode(NULL, BAD_CAST "AuthenticateMessage");
    if (!request) {
        isds_log_message(context,
                _("Could not build AuthenticateMessage request"));
        return IE_ERROR;
    }
    isds_ns = xmlNewNs(request, BAD_CAST ISDS_NS, NULL);
    if(!isds_ns) {
        isds_log_message(context, _("Could not create ISDS name space"));
        xmlFreeNode(request);
        return IE_ERROR;
    }
    xmlSetNs(request, isds_ns);

    /* Insert Base64 encoded message */
    err = insert_base64_encoded_string(context, request, NULL, "dmMessage",
            message, length);
    if (err) goto leave;

    /* Send request to server and process response */
    err = send_destroy_request_check_response(context,
            SERVICE_DM_OPERATIONS, BAD_CAST "AuthenticateMessage", &request,
            &response, NULL, NULL);
    if (err) goto leave;


    /* ISDS has decided */
    xpath_ctx = xmlXPathNewContext(response);
    if (!xpath_ctx) {
        err = IE_ERROR;
        goto leave;
    }
    if (_isds_register_namespaces(xpath_ctx, MESSAGE_NS_UNSIGNED)) {
        err = IE_ERROR;
        goto leave;
    }

    EXTRACT_BOOLEAN("/isds:AuthenticateMessageResponse/isds:dmAuthResult", authentic);

    if (!authentic) { 
        isds_log_message(context,
                _("Server did not return any response on "
                    "AuthenticateMessage request"));
        err = IE_ISDS;
        goto leave;
    }
    if (*authentic) {
        isds_log(ILF_ISDS, ILL_DEBUG,
                _("ISDS authenticated the message successfully\n"));
    } else {
        isds_log_message(context, _("ISDS does not know the message"));
        err = IE_NOTEQUAL;
    }


leave:
    free(authentic);
    xmlXPathFreeObject(result);
    xmlXPathFreeContext(xpath_ctx);

    xmlFreeDoc(response);
    xmlFreeNode(request);
#else /* not HAVE_LIBCURL */
    err = IE_NOTSUP;
#endif

    return err;
}


/* Submit CMS signed message or delivery info to ISDS to re-sign the content
 * including adding new CMS time stamp. Only CMS blobs without time stamp can
 * be re-signed.
 * @context is session context
 * @input_data is memory with raw CMS signed message or delivery info bit
 * stream to re-sign
 * @input_length is @input_data size in bytes
 * @output_data is pointer to auto-allocated memory where to store re-signed
 * input data blob. Caller must free it.
 * @output_data is pointer where to store @output_data size in bytes
 * @valid_to is pointer to auto-allocated date of time stamp expiration.
 * Only tm_year, tm_mon and tm_mday will be set. Pass NULL, if you don't care.
 * @return
 *  IE_SUCCESS  if CMS blob has been re-signed successfully
 *  other code  for other errors */
isds_error isds_resign_message(struct isds_ctx *context,
        const void *input_data, size_t input_length,
        void **output_data, size_t *output_length, struct tm **valid_to) {
    isds_error err = IE_SUCCESS;
#if HAVE_LIBCURL
    xmlNsPtr isds_ns = NULL;
    xmlNodePtr request = NULL;
    xmlDocPtr response = NULL;
    xmlXPathContextPtr xpath_ctx = NULL;
    xmlXPathObjectPtr result = NULL;
    char *string = NULL;
    const xmlChar *codes[] = {
        BAD_CAST "2200",
        BAD_CAST "2201",
        BAD_CAST "2204",
        BAD_CAST "2207",
        NULL
    };
    const char *meanings[] = {
        "Message is bad",
        "Message is not original",
        "Message already contains time stamp in CAdES-EPES or CAdES-T CMS structure",
        "Time stamp could not been generated in time"
    };
    const isds_error errors[] = {
        IE_INVAL,
        IE_NOTUNIQ,
        IE_INVAL,
        IE_ISDS,
    };
    struct code_map_isds_error map = {
        .codes = codes,
        .meanings = meanings,
        .errors = errors
    };
#endif

    if (NULL != output_data) *output_data = NULL;
    if (NULL != output_length) *output_length = 0;
    if (NULL != valid_to) *valid_to = NULL;

    if (NULL == context) return IE_INVALID_CONTEXT;
    zfree(context->long_message);
    if (NULL == input_data || 0 == input_length) {
        isds_log_message(context, _("Empty CMS blob on input"));
        return IE_INVAL;
    }
    if (NULL == output_data || NULL == output_length) {
        isds_log_message(context,
                _("NULL pointer provided for output CMS blob"));
        return IE_INVAL;
    }

#if HAVE_LIBCURL
    /* Check if connection is established
     * TODO: This check should be done downstairs. */
    if (!context->curl) return IE_CONNECTION_CLOSED;


    /* Build Re-signISDSDocument request */
    request = xmlNewNode(NULL, BAD_CAST "Re-signISDSDocument");
    if (!request) {
        isds_log_message(context,
                _("Could not build Re-signISDSDocument request"));
        return IE_ERROR;
    }
    isds_ns = xmlNewNs(request, BAD_CAST ISDS_NS, NULL);
    if(!isds_ns) {
        isds_log_message(context, _("Could not create ISDS name space"));
        xmlFreeNode(request);
        return IE_ERROR;
    }
    xmlSetNs(request, isds_ns);

    /* Insert Base64 encoded CMS blob */
    err = insert_base64_encoded_string(context, request, NULL, "dmDoc",
            input_data, input_length);
    if (err) goto leave;

    /* Send request to server and process response */
    err = send_destroy_request_check_response(context,
            SERVICE_DM_OPERATIONS, BAD_CAST "Re-signISDSDocument", &request,
            &response, NULL, &map);
    if (err) goto leave;


    /* Extract re-signed data */
    xpath_ctx = xmlXPathNewContext(response);
    if (!xpath_ctx) {
        err = IE_ERROR;
        goto leave;
    }
    if (_isds_register_namespaces(xpath_ctx, MESSAGE_NS_UNSIGNED)) {
        err = IE_ERROR;
        goto leave;
    }
    result = xmlXPathEvalExpression(
            BAD_CAST "/isds:Re-signISDSDocumentResponse", xpath_ctx);
    if (!result) {
        err = IE_ERROR;
        goto leave;
    }
    if (xmlXPathNodeSetIsEmpty(result->nodesetval)) {
        isds_log_message(context,
                _("Missing Re-signISDSDocumentResponse element"));
        err = IE_ISDS;
        goto leave;
    }
    if (result->nodesetval->nodeNr > 1) {
        isds_log_message(context,
                _("Multiple Re-signISDSDocumentResponse element"));
        err = IE_ISDS;
        goto leave;
    }
    xpath_ctx->node = result->nodesetval->nodeTab[0];
    xmlXPathFreeObject(result); result = NULL;

    EXTRACT_STRING("isds:dmResultDoc", string);
    /* Decode non-empty data */
    if (NULL != string && string[0] != '\0') {
        *output_length = _isds_b64decode(string, output_data);
        if (*output_length == (size_t) -1) {
            isds_log_message(context,
                    _("Error while Base64-decoding re-signed data"));
            err = IE_ERROR;
            goto leave;
        }
    } else {
        isds_log_message(context, _("Server did not send re-signed data"));
        err = IE_ISDS;
        goto leave;
    }
    zfree(string);

    if (NULL != valid_to) {
        /* Get time stamp expiration date */
        EXTRACT_STRING("isds:dmValidTo", string);
        if (NULL != string) {
            *valid_to = calloc(1, sizeof(**valid_to));
            if (!*valid_to) {
                err = IE_NOMEM;
                goto leave;
            }
            err = _isds_datestring2tm((xmlChar *)string, *valid_to);
            if (err) {
                if (err == IE_NOTSUP) {
                    err = IE_ISDS;
                    char *string_locale = _isds_utf82locale(string);
                    isds_printf_message(context,
                            _("Invalid dmValidTo value: %s"), string_locale);
                    free(string_locale);
                }
                goto leave;
            }
        }
    }

leave:
    free(string);

    xmlXPathFreeObject(result);
    xmlXPathFreeContext(xpath_ctx);

    xmlFreeDoc(response);
    xmlFreeNode(request);
#else /* not HAVE_LIBCURL */
    err = IE_NOTSUP;
#endif

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
            err = _isds_extract_cms_data(context,
                    message->raw, message->raw_length,
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
            err = _isds_extract_cms_data(context,
                    message->raw, message->raw_length,
                    &xml_stream, &xml_stream_length);
            if (err) goto leave;
            break;

        default:
            isds_log_message(context, _("Bad raw representation type"));
            return IE_INVAL;
            break;
    }


    /* XXX: Hash is computed from original string representing isds:dmDm
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
    err = _isds_find_element_boundary(xml_stream, xml_stream_length,
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
    err = _isds_compute_hash(xml_stream + phys_start, phys_end - phys_start + 1,
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

    for (size_t i = 0; i < h1->length; i++) {
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
 * member will be changed during function run. Use envelope on heap only.
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


/* Normalize @mime_type to be proper MIME type.
 * ISDS servers pass invalid MIME types (e.g. "pdf"). This function tries to
 * guess regular MIME type (e.g. "application/pdf").
 * @mime_type is UTF-8 encoded MIME type to fix
 * @return original @mime_type if no better interpretation exists, or 
 * constant static UTF-8 encoded string with proper MIME type. */
const char *isds_normalize_mime_type(const char *mime_type) {
    if (!mime_type) return NULL;

    for (size_t offset = 0;
            offset < sizeof(extension_map_mime)/sizeof(extension_map_mime[0]);
            offset += 2) {
        if (!xmlStrcasecmp((const xmlChar*) mime_type,
                extension_map_mime[offset]))
            return (const char *) extension_map_mime[offset + 1];
    }

    return mime_type;
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
 * @xpath_ctx is XPath context
 * @message_ns selects proper message name space. Unsigned and signed
 * messages and delivery info's differ in prefix and URI. */
_hidden isds_error _isds_register_namespaces(xmlXPathContextPtr xpath_ctx,
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
    if (xmlXPathRegisterNs(xpath_ctx, BAD_CAST "oisds", BAD_CAST OISDS_NS))
        return IE_ERROR;
    if (xmlXPathRegisterNs(xpath_ctx, BAD_CAST "sisds", message_namespace))
        return IE_ERROR;
    if (xmlXPathRegisterNs(xpath_ctx, BAD_CAST "xs", BAD_CAST SCHEMA_NS))
        return IE_ERROR;
    if (xmlXPathRegisterNs(xpath_ctx, BAD_CAST "deposit", BAD_CAST DEPOSIT_NS))
        return IE_ERROR;
    return IE_SUCCESS;
}

