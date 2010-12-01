#ifndef __ISDS_ISDS_H__
#define __ISDS_ISDS_H__

/* Public interface for libisds.
 * Private declarations in isds_priv.h. */

#include <stdlib.h> /* For size_t */
#include <time.h>   /* For struct tm */
#include <sys/time.h> /* For struct timeval */
#include <libxml/tree.h>    /* Gor xmlDoc and xmlNodePtr */

#ifdef __cplusplus  /* For C++ linker sake */
extern "C" {
#endif

/* _deprecated macro marks library symbols as deprecated. Application should
 * avoid using such function as soon as possible. */
#if defined(__GNUC__) 
#define _deprecated __attribute__((deprecated))
#else
#define _deprecated
#endif

/* Service locators */
/* Base URL of production ISDS instance */
extern const char isds_locator[];       /* Without client certificate auth. */
extern const char isds_cert_locator[];  /* With client certificate auth. */
/* Base URL of testing ISDS instance */
extern const char isds_testing_locator[]; /* Without client certificate auth. */
extern const char isds_testing_locator[]; /* With client certificate auth. */


struct isds_ctx;    /* Context for specific ISDS box */

typedef enum {
    IE_SUCCESS = 0, /* No error, just for C convenience (0 means Ok) */
    IE_ERROR,       /* Unspecified error */
    IE_NOTSUP,
    IE_INVAL,
    IE_INVALID_CONTEXT,
    IE_NOT_LOGGED_IN,
    IE_CONNECTION_CLOSED,
    IE_TIMED_OUT,
    IE_NOEXIST,
    IE_NOMEM,
    IE_NETWORK,
    IE_HTTP,
    IE_SOAP,
    IE_XML,
    IE_ISDS,
    IE_ENUM,
    IE_DATE,
    IE_2BIG,
    IE_2SMALL,
    IE_NOTUNIQ,
    IE_NOTEQUAL,
    IE_PARTIAL_SUCCESS,
    IE_ABORTED
} isds_error;

typedef enum {
    ILL_NONE = 0,
    ILL_CRIT = 10,
    ILL_ERR = 20,
    ILL_WARNING = 30,
    ILL_INFO = 40,
    ILL_DEBUG = 50,
    ILL_ALL = 100
} isds_log_level;

typedef enum {
    ILF_NONE = 0x0,
    ILF_HTTP = 0x1,
    ILF_SOAP = 0x2,
    ILF_ISDS = 0x4,
    ILF_FILE = 0x8,
    ILF_SEC  = 0x10,
    ILF_XML  = 0x20,
    ILF_ALL  = 0xFF
} isds_log_facility;

/* Return text description of ISDS error */
const char *isds_strerror(const isds_error error);

/* libisds options */
typedef enum {
    IOPT_TLS_VERIFY_SERVER,     /*  _Bool: Verify server identity?
                                   Default value is true. */
    IOPT_TLS_CA_FILE,           /* char *: File name with CA certificates.
                                   Default value depends on cryptographic
                                   library. */
    IOPT_TLS_CA_DIRECTORY,      /* char *: Directory name with CA certificates.
                                   Default value depends on cryptographic
                                   library. */
    IOPT_TLS_CRL_FILE,          /* char *: File  name with CRL in PEM format.
                                   Default value depends on cryptographic
                                   library. */
    IOPT_NORMALIZE_MIME_TYPE,   /*  _Bool: Normalize MIME type values?
                                   Default value is false. */
} isds_option;

/* TLS libisds options */
typedef enum {
    ITLS_VERIFY_SERVER,     /*  _Bool: Verify server identity? */
    ITLS_CA_FILE,           /* char *: File name with CA certificates */
    ITLS_CA_DIRECTORY,      /* char *: Directory name with CA certificates */
    ITLS_CRL_FILE           /* char *: File  name with CRL in PEM format */
} isds_tls_option;

/* Cryptographic material encoding */
typedef enum {
    PKI_FORMAT_PEM,         /* PEM format */
    PKI_FORMAT_DER,         /* DER format */
    PKI_FORMAT_ENG          /* Stored in crypto engine */
} isds_pki_format;

/* Public key crypto material to authenticate client */
struct isds_pki_credentials {
    char *engine;           /* String identifier of crypto engine to use
                               (where key is stored). Use NULL for no engine */
    isds_pki_format certificate_format;     /* Certificate format */
    char *certificate;      /* Path to client certificate, or certificate
                               nickname in case of NSS as curl back-end,
                               or key slot identifier inside crypto engine.
                               Some crypto engines can pair certificate with
                               key automatically (NULL value) */
    isds_pki_format key_format;     /* Private key format */
    char *key;              /* Path to client private key, or key identifier
                               in case of used engine */
    char *passphrase;       /* Zero terminated string with password for
                               decrypting private key, or engine PIN.
                               Use NULL for no pass-phrase or question by
                               engine. */
};

/* Box type */
typedef enum {
    DBTYPE_SYSTEM = 0,          /* This is special sender value for messages
                                   sent by ISDS. */
    DBTYPE_OVM = 10,
    DBTYPE_OVM_NOTAR = 11,
    DBTYPE_OVM_EXEKUT = 12,
    DBTYPE_OVM_REQ = 13,
    DBTYPE_PO = 20,
    DBTYPE_PO_ZAK = 21,
    DBTYPE_PO_REQ = 22,
    DBTYPE_PFO = 30,
    DBTYPE_PFO_ADVOK = 31,
    DBTYPE_PFO_DANPOR = 32,
    DBTYPE_PFO_INSSPR = 33,
    DBTYPE_FO = 40
} isds_DbType;

/* Box status from point of view of accessibility */
typedef enum {
    DBSTATE_ACCESSIBLE = 1,
    DBSTATE_TEMP_UNACCESSIBLE = 2,
    DBSTATE_NOT_YET_ACCESSIBLE = 3,
    DBSTATE_PERM_UNACCESSIBLE = 4,
    DBSTATE_REMOVED = 5
} isds_DbState;

/* User permissions from point of view of ISDS.
 * Instances can be bitmaps of any discrete values. */
typedef enum {
    PRIVIL_READ_NON_PERSONAL = 0x1, /* Can download and read messages with
                                       dmPersonalDelivery == false */
    PRIVIL_READ_ALL = 0x2,          /* Can download and read messages with
                                       dmPersonalDelivery == true */
    PRIVIL_CREATE_DM = 0x4,         /* Can create and sent messages,
                                       can download outgoing (sent) messages */
    PRIVIL_VIEW_INFO = 0x8,         /* Can list messages and data about
                                       post and delivery */
    PRIVIL_SEARCH_DB = 0x10,        /* Can search for boxes */
    PRIVIL_OWNER_ADM = 0x20,        /* Can administer his box (add/remove
                                       permitted users and theirs
                                       permissions) */
    PRIVIL_READ_VAULT = 0x40,       /* Can read message stored in data safe */
    PRIVIL_ERASE_VAULT = 0x80       /* Can delete messages from data safe */ 
} isds_priviledges;

/* Message status */
typedef enum {
    MESSAGESTATE_SENT = 0x2,            /* Message has been put into ISDS */
    MESSAGESTATE_STAMPED = 0x4,         /* Message stamped by TSA */
    MESSAGESTATE_INFECTED = 0x8,        /* Message included viruses,
                                           infected document has been removed */
    MESSAGESTATE_DELIVERED = 0x10,      /* Message delivered
                                           (dmDeliveryTime stored) */
    MESSAGESTATE_SUBSTITUTED = 0x20,    /* Message delivered through fiction,
                                           dmAcceptanceTime stored */
    MESSAGESTATE_RECEIVED = 0x40,       /* Message accepted (by user log-in or
                                           user explicit request),
                                           dmAcceptanceTime stored */
    MESSAGESTATE_READ = 0x80,           /* Message has been read by user */
    MESSAGESTATE_UNDELIVERABLE = 0x100, /* Message could not been delivered
                                           (e.g. recipient box has been made
                                           inaccessible meantime) */
    MESSAGESTATE_REMOVED = 0x200,       /* Message content deleted */
    MESSAGESTATE_IN_SAFE = 0x400        /* Message stored in data safe */

} isds_message_status;
#define MESSAGESTATE_ANY 0x7FE          /* Union of all isds_message_status
                                           values */

/* Hash algorithm types */
typedef enum {
    HASH_ALGORITHM_MD5,
    HASH_ALGORITHM_SHA_1,
    HASH_ALGORITHM_SHA_224,
    HASH_ALGORITHM_SHA_256,
    HASH_ALGORITHM_SHA_384,
    HASH_ALGORITHM_SHA_512,
} isds_hash_algorithm;

/* Buffer storage strategy.
 * How function should embed application provided buffer into raw element of
 * output structure. */
typedef enum {
    BUFFER_DONT_STORE,      /* Don't fill raw member */
    BUFFER_COPY,            /* Copy buffer content into newly allocated raw */
    BUFFER_MOVE             /* Just copy pointer.
                               But leave deallocation to isds_*_free(). */
} isds_buffer_strategy;

/* Hash value storage */
struct isds_hash {
    isds_hash_algorithm algorithm;      /* Hash algorithm */
    size_t length;                      /* Hash value length in bytes */
    void *value;                        /* Hash value */
};

/* Name of person */
struct isds_PersonName {
    char *pnFirstName;
    char *pnMiddleName;
    char *pnLastName;
    char *pnLastNameAtBirth;
};

/* Date and place of birth */
struct isds_BirthInfo {
    struct tm *biDate;      /* Date of Birth in local time at birth place,
                               only tm_year, tm_mon and tm_mday carry sane
                               value */
    char *biCity;
    char *biCounty;         /* German: Bezirk, Czech: okres */
    char *biState;
};

/* Post address */
struct isds_Address {
    char *adCity;
    char *adStreet;
    char *adNumberInStreet;
    char *adNumberInMunicipality;
    char *adZipCode;
    char *adState;
};

/* Data about box and his owner.
 * NULL pointer means undefined value */
struct isds_DbOwnerInfo {
    char *dbID;                     /* Box ID [Max. 7 chars] */
    isds_DbType *dbType;            /* Box Type */
    char *ic;                       /* ID */
    struct isds_PersonName *personName;     /* Name of person */
    char *firmName;                 /* Name of firm */
    struct isds_BirthInfo *birthInfo;       /* Birth of person */
    struct isds_Address *address;   /* Post address */
    char *nationality;
    char *email;
    char *telNumber;
    char *identifier;               /* External box identifier for data
                                       provider (OVM, PO, maybe PFO)
                                       [Max. 20 chars] */
    char *registryCode;             /* PFO External registry code
                                       [Max. 5 chars] */
    long int *dbState;              /* Box state; 1 <=> active box;
                                       long int because xsd:integer
                                       TODO: enum? */
    _Bool *dbEffectiveOVM;          /* Box has OVM role (§ 5a) */
    _Bool *dbOpenAddressing;        /* Non-OVM Box is free to receive
                                       messages from anybody */  
};

/* User type */
typedef enum {
    USERTYPE_PRIMARY,               /* Owner of the box */
    USERTYPE_ENTRUSTED,             /* User with limited access to the box */
    USERTYPE_ADMINISTRATOR,         /* User to manage ENTRUSTED_USERs */
    USERTYPE_OFFICIAL               /* ??? */
} isds_UserType;

/* Data about user.
 * NULL pointer means undefined value */
struct isds_DbUserInfo {
    char *userID;               /* User ID [Min. 6, max. 12 characters] */
    isds_UserType *userType;    /* User type */
    long int *userPrivils;      /* Set of user permissions */
    struct isds_PersonName *personName;     /* Name of the person */
    struct isds_Address *address;   /* Post address */
    struct tm *biDate;          /* Date of birth in local time,
                                   only tm_year, tm_mon and tm_mday carry sane
                                   value */
    char *ic;                   /* ID of a supervising firm [Max. 8 chars] */
    char *firmName;             /* Name of a supervising firm
                                   [Max. 100 chars] */
    char *caStreet;             /* Street and number of contact address */
    char *caCity;               /* Czech City of contact address */
    char *caZipCode;            /* Post office code of contact address */
    char *caState;              /* Abbreviated country of contact address;
                                   Implicit value is "CZ"; Optional. */
};

/* Message event type */
typedef enum {
    EVENT_UKNOWN,                   /* Event unknown to this library */
    EVENT_ACCEPTED_BY_RECIPIENT,    /* Message has been delivered and accepted
                                       by recipient action */
    EVENT_ACCEPTED_BY_FICTION,      /* Message has been delivered, acceptance
                                       timed out, considered as accepted */
    EVENT_UNDELIVERABLE,            /* Recipient box made inaccessible,
                                       thus message is undeliverable */
    EVENT_COMMERCIAL_ACCEPTED       /* Recipient confirmed acceptance of
                                       commercial message */
} isds_event_type;

/* Message event
 * All members are optional as specification states so. */
struct isds_event {
    struct timeval *time;           /* When the event occurred */
    isds_event_type *type;          /* Type of the event */
    char *description;              /* Human readable event description
                                       generated by ISDS (Czech) */
};

/* Message envelope
 * Be ware that the string length constraints are forced only on output
 * members transmitted to ISDS. The other direction (downloaded from ISDS)
 * can break these rules. It should not happen, but nobody knows how much
 * incompatible new version of ISDS protocol will be. This is the gold
 * Internet rule: be strict on what you put, be tolerant on what you get. */
struct isds_envelope {
    /* Following members apply to incoming messages only: */
    char *dmID;                     /* Message ID.
                                       Maximal length is 20 characters. */
    char *dbIDSender;               /* Box ID of sender.
                                       Special value "aaaaaaa" means sent by
                                       ISDS.
                                       Maximal length is 7 characters. */
    char *dmSender;                 /* Sender name;
                                       Maximal length is 100 characters. */
    char *dmSenderAddress;          /* Postal address of sender;
                                       Maximal length is 100 characters. */
    long int *dmSenderType;         /* Gross Box type of sender
                                       TODO: isds_DbType ? */
    char *dmRecipient;              /* Recipient name;
                                       Maximal length is 100 characters. */
    char *dmRecipientAddress;       /* Postal address of recipient;
                                       Maximal length is 100 characters. */
    _Bool *dmAmbiguousRecipient;    /* Recipient has OVM role */

    /* Following members are assigned by ISDS in different phases of message
     * life cycle. */
    unsigned long int *dmOrdinal;   /* Ordinal number in list of
                                       incoming/outgoing messages */
    isds_message_status *dmMessageStatus;  /* Message state */
    long int *dmAttachmentSize;     /* Size of message documents in
                                       kilobytes (rounded). */
    struct timeval *dmDeliveryTime;     /* Time of delivery into a box
                                           NULL, if message has not been
                                           delivered yet */
    struct timeval *dmAcceptanceTime;   /* Time of acceptance of the message
                                           by an user. NULL if message has not
                                           been accepted yet. */
    struct isds_hash *hash;         /* Message hash.
                                       This is hash of isds:dmDM subtree. */
    void *timestamp;                /* Qualified time stamp; Optional. */
    size_t timestamp_length;        /* Length of timestamp in bytes */
    struct isds_list *events;       /* Events message passed trough;
                                       List of isds_event's. */


    /* Following members apply to both outgoing and incoming messages: */
    char *dmSenderOrgUnit;          /* Organisation unit of sender as string;
                                       Optional. */
    long int *dmSenderOrgUnitNum;   /* Organisation unit of sender as number;
                                       Optional. */
    char *dbIDRecipient;            /* Box ID of recipient; Mandatory.
                                       Maximal length is 7 characters. */
    char *dmRecipientOrgUnit;       /* Organisation unit of recipient as
                                       string; Optional. */
    long int *dmRecipientOrgUnitNum;    /* Organisation unit of recipient as
                                           number; Optional. */
    char *dmToHands;                /* Person in recipient organisation;
                                       Optional. */
    char *dmAnnotation;             /* Subject (title) of the message.
                                       Maximal length is 255 characters. */
    char *dmRecipientRefNumber;     /* Czech: číslo jednací příjemce; Optional.
                                       Maximal length is 50 characters. */
    char *dmSenderRefNumber;        /* Czech: číslo jednací odesílatele;
                                       Optional. Maximal length is 50 chars. */
    char *dmRecipientIdent;         /* Czech: spisová značka příjemce; Optional.
                                       Maximal length is 50 characters. */
    char *dmSenderIdent;            /* Czech: spisová značka odesílatele;
                                       Optional. Maximal length is 50 chars. */

    /* Act addressing in Czech Republic:
     * Point (Paragraph) § Section Law/Year Coll. */
    long int *dmLegalTitleLaw;      /* Number of act mandating authority */
    long int *dmLegalTitleYear;     /* Year of act issue mandating authority */
    char *dmLegalTitleSect;         /* Section of act mandating authority.
                                       Czech: paragraf */
    char *dmLegalTitlePar;          /* Paragraph of act mandating authority.
                                       Czech: odstavec */
    char *dmLegalTitlePoint;        /* Point of act mandating authority.
                                       Czech: písmeno */

    _Bool *dmPersonalDelivery;      /* If true, only person with higher
                                       privileges can read this message */
    _Bool *dmAllowSubstDelivery;    /* Allow delivery through fiction.
                                       I.e. Even if recipient did not read this
                                       message, message is considered as
                                       delivered after (currently) 10 days.
                                       This is delivery through fiction.
                                       Applies only to OVM dbType sender. */
    _Bool *dmOVM;                   /* OVM sending mode.
                                       Non-OVM dbType boxes that has
                                       dbEffectiveOVM == true MUST select
                                       between true (OVM mode) and
                                       false (non-OVM mode).
                                       Optional; Implicit value is true. */
    char *dmType;                   /* Message type:
                                       "V" is public message
                                       "K" is commercial message
                                       Default value for outgoing message is "V"
                                       Length: Exact 1 UTF-8 character if
                                       defined;
                                       As 2010-05-20, used as output only. */
};


/* Document type from point of hierarchy */
typedef enum {
    FILEMETATYPE_MAIN,              /* Main document */
    FILEMETATYPE_ENCLOSURE,         /* Appendix */
    FILEMETATYPE_SIGNATURE,         /* Digital signature of other document */
    FILEMETATYPE_META               /* XML document for ESS (electronic
                                       document information system) purposes */
} isds_FileMetaType;

/* Document */
struct isds_document {
    _Bool is_xml;                   /* True if document is ISDS XML document.
                                       False if document is ISDS binary
                                       document. */
    xmlNodePtr xml_node_list;       /* XML node-set presenting current XML
                                       document content. This is pointer to
                                       first node of the document in
                                       isds_message.xml tree. Use `children'
                                       and `next' members to iterate the
                                       document.
                                       It will be NULL if document is empty.
                                       Valid only if is_xml is true. */
    void *data;                     /* Document content.
                                       The encoding and interpretation depends
                                       on dmMimeType.
                                       Valid only if is_xml is false. */
    size_t data_length;             /* Length of the data in bytes.
                                       Valid only if is_xml is false. */

    char *dmMimeType;               /* MIME type of data; Mandatory. */
    isds_FileMetaType dmFileMetaType;   /* Document type to create hierarchy */
    char *dmFileGuid;               /* Message-local document identifier;
                                       Optional. */
    char *dmUpFileGuid;             /* Reference to upper document identifier
                                       (dmFileGuid); Optional. */
    char *dmFileDescr;              /* Document name (title). E.g. file name;
                                       Mandatory. */
    char *dmFormat;                 /* Reference to XML form definition;
                                       Defines how to interpret XML document;
                                       Optional. */
};

/* Raw message representation content type.
 * This is necessary to distinguish between different representations without
 * expensive repeated detection.
 * Infix explanation:
 *  PLAIN_SIGNED  data are XML with namespace mangled to signed alternative
 *  CMS_SIGNED    data are XML with signed namespace encapsulated in CMS */
typedef enum {
    RAWTYPE_INCOMING_MESSAGE,
    RAWTYPE_PLAIN_SIGNED_INCOMING_MESSAGE,
    RAWTYPE_CMS_SIGNED_INCOMING_MESSAGE,
    RAWTYPE_PLAIN_SIGNED_OUTGOING_MESSAGE,
    RAWTYPE_CMS_SIGNED_OUTGOING_MESSAGE,
    RAWTYPE_DELIVERYINFO,
    RAWTYPE_PLAIN_SIGNED_DELIVERYINFO,
    RAWTYPE_CMS_SIGNED_DELIVERYINFO
} isds_raw_type;

/* Message */
struct isds_message {
    void *raw;                      /* Raw message in XML format as send to or
                                       from the ISDS. You can use it to store
                                       local copy. This is binary buffer. */
    size_t raw_length;              /* Length of raw message in bytes */
    isds_raw_type raw_type;         /* Content type of raw representation
                                       Meaningful only with non-NULL raw
                                       member */
    xmlDocPtr xml;                  /* Parsed XML document with attached ISDS
                                       message XML documents.
                                       Can be NULL. May be freed AFTER deallocating
                                       documents member structure. */
    struct isds_envelope *envelope; /* Message envelope */
    struct isds_list *documents;    /* List of isds_document's.
                                       Valid message must contain exactly one
                                       document of type FILEMETATYPE_MAIN and
                                       can contain any number of other type
                                       documents. Total size of documents
                                       must not exceed 10 MB. */
};

/* Message copy recipient and assigned message ID */
struct isds_message_copy {
    /* Input members defined by application */
    char *dbIDRecipient;            /* Box ID of recipient; Mandatory.
                                       Maximal length is 7 characters. */
    char *dmRecipientOrgUnit;       /* Organisation unit of recipient as
                                       string; Optional. */
    long int *dmRecipientOrgUnitNum;    /* Organisation unit of recipient as
                                           number; Optional. */
    char *dmToHands;                /* Person in recipient organisation;
                                       Optional. */

    /* Output members returned from ISDS */
    isds_error error;               /* libisds compatible error of delivery to o                                       ne recipient */
    char *dmStatus;                 /* Error description returned by ISDS;
                                       Optional. */
    char *dmID;                     /* Assigned message ID; Meaningful only
                                       for error == IE_SUCCESS */
};

/* General linked list */
struct isds_list {
    struct isds_list *next;         /* Next list item,
                                       or NULL if current is last */
    void *data;                     /* Payload */
    void (*destructor) (void **);   /* Payload deallocator;
                                       Use NULL to have static data member. */
};

/* External box approval */
struct isds_approval {
    _Bool approved;                 /* True if request for box has been
                                       approved out of ISDS */
    char *refference;               /* Identifier of the approval */
};

/* Digital delivery of credentials */
struct isds_credentials_delivery {
    /* Input members */
    char *email;                    /* e-mail address where to send
                                       notification with link to service where
                                       user can get know his new credentials */
    /* Output members */
    char *token;                    /* token user needs to use to authorize on
                                       the web server to view his new
                                       credentials. */
    char *new_user_name;            /* user's log-in name that ISDS created/
                                       changed up on a call. */
};

/* Initialize ISDS library.
 * Global function, must be called before other functions.
 * If it fails you can not use ISDS library and must call isds_cleanup() to
 * free partially initialized global variables. */
isds_error isds_init(void);

/* Deinitialize ISDS library.
 * Global function, must be called as last library function. */
isds_error isds_cleanup(void);

/* Return version string of this library. Version of dependencies can be
 * embedded. Do no try to parse it. You must free it. */
char *isds_version(void);

/* Create ISDS context.
 * Each context can be used for different sessions to (possibly) different
 * ISDS server with different credentials.
 * Returns new context, or NULL */
struct isds_ctx *isds_ctx_create(void);

/* Destroy ISDS context and free memory.
 * @context will be NULLed on success. */
isds_error isds_ctx_free(struct isds_ctx **context);

/* Return long message text produced by library function, e.g. detailed error
 * message. Returned pointer is only valid until new library function is
 * called for the same context. Could be NULL, especially if NULL context is
 * supplied. Return string is locale encoded. */
char *isds_long_message(const struct isds_ctx *context);

/* Set logging up.
 * @facilities is bit mask of isds_log_facility values,
 * @level is verbosity level. */
void isds_set_logging(const unsigned int facilities,
        const isds_log_level level);

/* Function provided by application libisds will call to pass log message.
 * The message is usually locale encoded, but raw strings (UTF-8 usually) can
 * occur when logging raw communication with ISDS servers. Infixed zero byte
 * is not excluded, but should not present. Use @length argument to get real
 * length of the message.
 * TODO: We will try to fix the encoding issue
 * @facility is log message class
 * @level is log message severity
 * @message is string with zero byte terminator. This can be any arbitrary
 * chunk of a sentence with or without new line, a sentence can be splitted
 * into more messages. However it should not happen. If you discover message
 * without new line, report it as a bug.
 * @length is size of @message string in bytes excluding trailing zero
 * @data is pointer that will be passed unchanged to this function at run-time
 * */
typedef void (*isds_log_callback)(
        isds_log_facility facility, isds_log_level level,
        const char *message, int length, void *data);

/* Register callback function libisds calls when new global log message is
 * produced by library. Library logs to stderr by default.
 * @callback is function provided by application libisds will call. See type
 * definition for @callback argument explanation. Pass NULL to revert logging to
 * default behaviour.
 * @data is application specific data @callback gets as last argument */
void isds_set_log_callback(isds_log_callback callback, void *data);

/* Set timeout in milliseconds for each network job like connecting to server
 * or sending message. Use 0 to disable timeout limits. */
isds_error isds_set_timeout(struct isds_ctx *context,
        const unsigned int timeout);

/* Function provided by application libisds will call with
 * following five arguments. Value zero of any argument means the value is
 * unknown.
 * @upload_total is expected total upload,
 * @upload_current is cumulative current upload progress
 * @dowload_total is expected total download
 * @download_current is cumulative current download progress
 * @data is pointer that will be passed unchanged to this function at run-time
 * @return 0 to continue HTTP transfer, or non-zero to abort transfer */
typedef int (*isds_progress_callback)(
        double upload_total, double upload_current,
        double download_total, double download_current,
        void *data);

/* Register callback function libisds calls periodically during HTTP data
 * transfer.
 * @context is session context
 * @callback is function provided by application libisds will call. See type
 * definition for @callback argument explanation.
 * @data is application specific data @callback gets as last argument */
isds_error isds_set_progress_callback(struct isds_ctx *context,
        isds_progress_callback callback, void *data);

/* Change context settings.
 * @context is context which setting will be applied to
 * @option is name of option. It determines the type of last argument. See
 * isds_option definition for more info.
 * @... is value of new setting. Type is determined by @option
 * */
isds_error isds_set_opt(struct isds_ctx *context, const isds_option option,
        ...);

/* Deprecated: Use isds_set_opt() instead.
 * Change SSL/TLS settings.
 * @context is context which setting will be applied to
 * @option is name of option. It determines the type of last argument. See
 * isds_tls_option definition for more info.
 * @... is value of new setting. Type is determined by @option
 * */
isds_error isds_set_tls(struct isds_ctx *context, const isds_tls_option option,
        ...) _deprecated; 

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
 * Please note, that different cases requires different certificate type
 * (system qualified one or commercial non qualified one). This library does
 * not check such political issues. Please see ISDS Specification for more
 * details.
 * @url is base address of ISDS web service. Pass extern isds_locator
 * variable to use production ISDS instance without client certificate
 * authentication (or extern isds_cert_locator with client certificate
 * authentication). Passing NULL has the same effect, autoselection between
 * isds_locator and isds_cert_locator is performed in addition. You can pass
 * extern isds_testing_locator (or isds_cert_testing_locator) variable to
 * select testing instance. 
 * @username is user name of ISDS user or box ID
 * @password is user's secret password
 * @pki_credentials defines public key cryptographic material to use in client
 * authentication. */
isds_error isds_login(struct isds_ctx *context, const char *url,
        const char *username, const char *password,
        const struct isds_pki_credentials *pki_credentials);

/* Log out from ISDS server and close connection. */
isds_error isds_logout(struct isds_ctx *context);

/* Verify connection to ISDS is alive and server is responding.
 * Sent dummy request to ISDS and expect dummy response. */
isds_error isds_ping(struct isds_ctx *context);

/* Get data about logged in user and his box. */
isds_error isds_GetOwnerInfoFromLogin(struct isds_ctx *context,
        struct isds_DbOwnerInfo **db_owner_info);

/* Get data about logged in user. */
isds_error isds_GetUserInfoFromLogin(struct isds_ctx *context,
        struct isds_DbUserInfo **db_user_info);

/* Get expiration time of current password
 * @context is session context
 * @expiration is automatically reallocated time when password expires, In
 * case of error will be nulled. */
isds_error isds_get_password_expiration(struct isds_ctx *context,
        struct timeval **expiration);

/* Change user password in ISDS.
 * User must supply old password, new password will takes effect after some
 * time, current session can continue. Password must fulfill some constraints.
 * @context is session context
 * @old_password is current password.
 * @new_password is requested new password */
isds_error isds_change_password(struct isds_ctx *context,
        const char *old_password, const char *new_password);

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
 * @token is NULL if new password should be delivered off-line to the user.
 * It is valid pointer if user should obtain new password on-line on dedicated
 * web server. Then it outputs automatically reallocated token user needs to
 * use to authorize on the web server to view his new password. 
 * @approval is optional external approval of box manipulation
 * @refnumber is reallocated serial number of request assigned by ISDS. Use
 * NULL, if you don't care.*/
isds_error isds_add_box(struct isds_ctx *context,
        struct isds_DbOwnerInfo *box, const struct isds_list *users,
        const char *former_names, const char *upper_box_id,
        const char *ceo_label, char **token,
        const struct isds_approval *approval, char **refnumber);

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
        char **refnumber);

/* Remove given given box permanently.
 * @context is session context
 * @box is box description to delete
 * @since is date of box owner cancellation. Only tm_year, tm_mon and tm_mday
 * carry sane value.
 * @approval is optional external approval of box manipulation
 * @refnumber is reallocated serial number of request assigned by ISDS. Use
 * NULL, if you don't care.*/
isds_error isds_delete_box(struct isds_ctx *context,
        const struct isds_DbOwnerInfo *box, const struct tm *since,
        const struct isds_approval *approval, char **refnumber);

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
        const struct isds_approval *approval, char **refnumber);

/* Get data about all users assigned to given box.
 * @context is session context
 * @box_id is box ID
 * @users is automatically reallocated list of struct isds_DbUserInfo */
isds_error isds_GetDataBoxUsers(struct isds_ctx *context, const char *box_id,
        struct isds_list **users);

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
        char **refnumber);

/* Reset credentials of user assigned to given box.
 * @context is session context
 * @box is box identification
 * @user identifies user to reset password
 * @fee_paid is true if fee has been paid, false otherwise
 * @approval is optional external approval of box manipulation
 * @token is NULL if new password should be delivered off-line to the user.
 * It is valid pointer if user should obtain new password on-line on dedicated
 * web server. Then it outputs automatically reallocated token user needs to
 * use to authorize on the web server to view his new password. 
 * @email is user's e-mail address user must provide to dedicated web server
 * together with @token. Valid only if @token is not NULL.
 * @new_user_name is automatically reallocated user's log-in name that ISDS
 * changed up on this call. Valid only if @token is not NULL.
 * @refnumber is reallocated serial number of request assigned by ISDS. Use
 * NULL, if you don't care.*/
isds_error isds_reset_password(struct isds_ctx *context,
        const struct isds_DbOwnerInfo *box,
        const struct isds_DbUserInfo *user,
        const _Bool fee_paid, const struct isds_approval *approval,
        char **token, const char *email, char **new_user_name,
        char **refnumber);

/* Assign new user to given box.
 * @context is session context
 * @box is box identification
 * @user defines new user to add
 * @approval is optional external approval of box manipulation
 * @refnumber is reallocated serial number of request assigned by ISDS. Use
 * NULL, if you don't care.*/
isds_error isds_add_user(struct isds_ctx *context,
        const struct isds_DbOwnerInfo *box, const struct isds_DbUserInfo *user,
        const struct isds_approval *approval, char **refnumber);

/* Remove user assigned to given box.
 * @context is session context
 * @box is box identification
 * @user identifies user to remove
 * @approval is optional external approval of box manipulation
 * @refnumber is reallocated serial number of request assigned by ISDS. Use
 * NULL, if you don't care.*/
isds_error isds_delete_user(struct isds_ctx *context,
        const struct isds_DbOwnerInfo *box, const struct isds_DbUserInfo *user,
        const struct isds_approval *approval, char **refnumber);

/* Find boxes suiting given criteria.
 * @context is ISDS session context.
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
        struct isds_list **boxes);

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
        long int *box_status);

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
        const struct isds_approval *approval, char **refnumber);

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
        const struct isds_approval *approval, char **refnumber);

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
        char **refnumber);

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
        char **refnumber);

/* Send a message via ISDS to a recipient
 * @context is session context
 * @outgoing_message is message to send; Some members are mandatory (like
 * dbIDRecipient), some are optional and some are irrelevant (especially data
 * about sender). Included pointer to isds_list documents must contain at
 * least one document of FILEMETATYPE_MAIN. This is read-write structure, some
 * members will be filled with valid data from ISDS. Exact list of write
 * members is subject to change. Currently dmId is changed.
 * @return ISDS_SUCCESS, or other error code if something goes wrong. */
isds_error isds_send_message(struct isds_ctx *context,
        struct isds_message *outgoing_message);

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
        struct isds_list *copies);

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
 * if you provide pointer to non-NULL, list will be freed automatically at first.
 * Also in case of error the list will be NULLed.
 * @return IE_SUCCESS or appropriate error code. */
isds_error isds_get_list_of_sent_messages(struct isds_ctx *context,
        const struct timeval *from_time, const struct timeval *to_time,
        const long int *dmSenderOrgUnitNum, const unsigned int status_filter,
        const unsigned long int offset, unsigned long int *number,
        struct isds_list **messages);

/* Get list of incoming (addressed to you) messages.
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
 * Use NULL if you don't care about the meta data (useful if you want to know
 * only the @number). If you provide &NULL, list will be allocated on heap,
 * if you provide pointer to non-NULL, list will be freed automatically at first.
 * Also in case of error the list will be NULLed.
 * @return IE_SUCCESS or appropriate error code. */
isds_error isds_get_list_of_received_messages(struct isds_ctx *context,
        const struct timeval *from_time, const struct timeval *to_time,
        const long int *dmSenderOrgUnitNum, const unsigned int status_filter,
        const unsigned long int offset, unsigned long int *number,
        struct isds_list **messages);

/* Download incoming message envelope identified by ID.
 * @context is session context
 * @message_id is message identifier (you can get them from
 * isds_get_list_of_received_messages())
 * @message is automatically reallocated message retrieved from ISDS.
 * It will miss documents per se. Use isds_get_received_message(), if you are
 * interested in documents (content) too.
 * Returned hash and timestamp require documents to be verifiable. */
isds_error isds_get_received_envelope(struct isds_ctx *context,
        const char *message_id, struct isds_message **message);

/* Download signed delivery info-sheet of given message identified by ID.
 * @context is session context
 * @message_id is message identifier (you can get them from
 * isds_get_list_of_{sent,received}_messages())
 * @message is automatically reallocated message retrieved from ISDS.
 * It will miss documents per se. Use isds_get_signed_received_message(),
 * if you are interested in documents (content). OTOH, only this function
 * can get list events message has gone through. */
isds_error isds_get_signed_delivery_info(struct isds_ctx *context,
        const char *message_id, struct isds_message **message);

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
        struct isds_message **message, const isds_buffer_strategy strategy);

/* Download delivery info-sheet of given message identified by ID.
 * @context is session context
 * @message_id is message identifier (you can get them from
 * isds_get_list_of_{sent,received}_messages())
 * @message is automatically reallocated message retrieved from ISDS.
 * It will miss documents per se. Use isds_get_received_message(), if you are
 * interested in documents (content). OTOH, only this function can get list
 * of events message has gone through. */
isds_error isds_get_delivery_info(struct isds_ctx *context,
        const char *message_id, struct isds_message **message);

/* Download incoming message identified by ID.
 * @context is session context
 * @message_id is message identifier (you can get them from
 * isds_get_list_of_received_messages())
 * @message is automatically reallocated message retrieved from ISDS */
isds_error isds_get_received_message(struct isds_ctx *context,
        const char *message_id, struct isds_message **message);

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
        struct isds_message **message, const isds_buffer_strategy strategy);

/* Determine type of raw message or delivery info according some heuristics.
 * It does not validate the raw blob.
 * @context is session context
 * @raw_type returns content type of @buffer. Valid only if exit code of this
 * function is IE_SUCCESS. The pointer must be valid. This is no automatically
 * reallocated memory.
 * @buffer is message raw representation.
 * @length is length of buffer in bytes. */
isds_error isds_guess_raw_type(struct isds_ctx *context,
        isds_raw_type *raw_type, const void *buffer, const size_t length);

/* Download signed incoming message identified by ID.
 * @context is session context
 * @message_id is message identifier (you can get them from
 * isds_get_list_of_received_messages())
 * @message is automatically reallocated message retrieved from ISDS. The raw
 * member will be filled with PKCS#7 structure in DER format. */
isds_error isds_get_signed_received_message(struct isds_ctx *context,
        const char *message_id, struct isds_message **message);

/* Download signed outgoing message identified by ID.
 * @context is session context
 * @message_id is message identifier (you can get them from
 * isds_get_list_of_sent_messages())
 * @message is automatically reallocated message retrieved from ISDS. The raw
 * member will be filled with PKCS#7 structure in DER format. */
isds_error isds_get_signed_sent_message(struct isds_ctx *context,
        const char *message_id, struct isds_message **message);

/* Retrieve hash of message identified by ID stored in ISDS.
 * @context is session context
 * @message_id is message identifier
 * @hash is automatically reallocated message hash downloaded from ISDS.
 * Message must exist in system and must not be deleted. */
isds_error isds_download_message_hash(struct isds_ctx *context,
        const char *message_id, struct isds_hash **hash);

/* Compute hash of message from raw representation and store it into envelope.
 * Original hash structure will be destroyed in envelope.
 * @context is session context
 * @message is message carrying raw XML message blob
 * @algorithm is desired hash algorithm to use */
isds_error isds_compute_message_hash(struct isds_ctx *context,
        struct isds_message *message, const isds_hash_algorithm algorithm);

/* Compare two hashes.
 * @h1 is first hash
 * @h2 is another hash
 * @return
 *  IE_SUCCESS  if hashes equal
 *  IE_NOTUNIQ  if hashes are comparable, but they don't equal
 *  IE_ENUM     if not comparable, but both structures defined
 *  IE_INVAL    if some of the structures are undefined (NULL)
 *  IE_ERROR    if internal error occurs */
isds_error isds_hash_cmp(const struct isds_hash *h1,
        const struct isds_hash *h2);

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
        struct isds_message *message);

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
        const void *message, size_t length);

/* Mark message as read. This is a transactional commit function to acknowledge
 * to ISDS the message has been downloaded and processed by client properly.
 * @context is session context
 * @message_id is message identifier. */
isds_error isds_mark_message_read(struct isds_ctx *context,
        const char *message_id);

/* Mark message as received by recipient. This is applicable only to
 * commercial message. Use envelope->dmType message member to distinguish
 * commercial message from government message. Government message is
 * received automatically (by law), commercial message on recipient request.
 * @context is session context
 * @message_id is message identifier. */
isds_error isds_mark_message_received(struct isds_ctx *context,
        const char *message_id);

/* Send bogus request to ISDS.
 * Just for test purposes */
isds_error isds_bogus_request(struct isds_ctx *context);

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
        char **id, struct tm **date);

/* Close possibly opened connection to Czech POINT document deposit.
 * @context is Czech POINT session context. */
isds_error czp_close_connection(struct isds_ctx *context);

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
 * @former_names is optional undocumented string. Pass NULL if you don't care.
 * @approval is optional external approval of box manipulation
 * @refnumber is reallocated serial number of request assigned by ISDS. Use
 * NULL, if you don't care.*/
isds_error isds_request_new_testing_box(struct isds_ctx *context,
        struct isds_DbOwnerInfo *box, const struct isds_list *users,
        const char *former_names, const struct isds_approval *approval,
        char **refnumber);

/* Search for document by document ID in list of documents. IDs are compared
 * as UTF-8 string.
 * @documents is list of isds_documents
 * @id is document identifier
 * @return first matching document or NULL. */
const struct isds_document *isds_find_document_by_id(
        const struct isds_list *documents, const char *id);

/* Normalize @mime_type to be proper MIME type.
 * ISDS servers passes invalid MIME types (e.g. "pdf"). This function tries to
 * guess regular MIME type (e.g. "application/pdf").
 * @mime_type is UTF-8 encoded MIME type to fix
 * @return original @mime_type if no better interpretation exists, or array to
 * constant static UTF-8 encoded string with proper MIME type. */
char *isds_normalize_mime_type(const char* mime_type);

/* XXX: Deprecated: Use isds_set_opt() instead.
 * Switch MIME type normalization while message loading. Default state for new
 * context is no normalization.
 * @normalize use true to switch normalization on, false to switch off */
isds_error isds_set_mime_type_normalization(struct isds_ctx *context,
        _Bool normalize) _deprecated;

/* Deallocate structure isds_pki_credentials and NULL it.
 * Pass-phrase is discarded.
 * @pki  credentials to to free */
void isds_pki_credentials_free(struct isds_pki_credentials **pki);

/* Free isds_list with all member data.
 * @list list to free, on return will be NULL */
void isds_list_free(struct isds_list **list);

/* Deallocate structure isds_hash and NULL it.
 * @hash  hash to to free */
void isds_hash_free(struct isds_hash **hash);

/* Deallocate structure isds_DbOwnerInfo recursively and NULL it */
void isds_DbOwnerInfo_free(struct isds_DbOwnerInfo **db_owner_info);

/* Deallocate structure isds_DbUserInfo recursively and NULL it */
void isds_DbUserInfo_free(struct isds_DbUserInfo **db_user_info);

/* Deallocate struct isds_event recursively and NULL it */
void isds_event_free(struct isds_event **event);

/* Deallocate struct isds_envelope recursively and NULL it */
void isds_envelope_free(struct isds_envelope **envelope);

/* Deallocate struct isds_document recursively and NULL it */
void isds_document_free(struct isds_document **document);

/* Deallocate struct isds_message recursively and NULL it */
void isds_message_free(struct isds_message **message);

/* Deallocate struct isds_message_copy recursively and NULL it */
void isds_message_copy_free(struct isds_message_copy **copy);

/* Deallocate struct isds_approval recursively and NULL it */
void isds_approval_free(struct isds_approval **approval);

/* Deallocate struct isds_credentials_delivery recursively and NULL it.
 * The email string is deallocated too. */
void isds_credentials_delivery_free(
        struct isds_credentials_delivery **credentials_delivery);

/* Copy structure isds_PersonName recursively */
struct isds_PersonName *isds_PersonName_duplicate(
        const struct isds_PersonName *template);

/* Copy structure isds_Address recursively */
struct isds_Address *isds_Address_duplicate(
        const struct isds_Address *template);

/* Copy structure isds_DbOwnerInfo recursively */
struct isds_DbOwnerInfo *isds_DbOwnerInfo_duplicate(
        const struct isds_DbOwnerInfo *template);

/* Copy structure isds_DbUserInfo recursively */
struct isds_DbUserInfo *isds_DbUserInfo_duplicate(
        const struct isds_DbUserInfo *template);

#ifdef __cplusplus  /* For C++ linker sake */
}
#endif

#endif
