#ifndef __ISDS_ISDS_H__
#define __ISDS_ISDS_H__

/* Public interface for libisds.
 * Private declarations in isds_priv.h. */

#include <stdlib.h> /* For size_t */
#include <sys/time.h> /* For struct timeval */

/* _deprecated macro marks library symbols as deprecated. Application should
 * avoid using such function as soon as possible. */
#if defined(__GNUC__) 
#define _deprecated __attribute__((deprecated))
#else
#define _deprecated
#endif


struct isds_ctx;    /* Context for specific ISDS box */

typedef enum {
    IE_SUCCESS = 0, /* No error, just for C conveniece (0 means Ok) */
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
    IE_PARTIAL_SUCCESS
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

/* TLS libisds options */
typedef enum {
    ITLS_VERIFY_SERVER,     /*  _Bool: Verify server idetity? */
    ITLS_CA_FILE,           /* char *: File name with CA certificates */
    ITLS_CA_DIRECTORY       /* char *: Directory name with CA certificates */
} isds_tls_option;

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

/* Box status from point of view of accesibilty */
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
                                       can dowload outgoing (sent) messages */
    PRIVIL_VIEW_INFO = 0x8,         /* Can list messages and data about
                                       post and delivery */
    PRIVIL_SEARCH_DB = 0x10,        /* Can search for boxes */
    PRIVIL_OWNER_ADM = 0x20,        /* Can administer his box (add/remove
                                       permitted users and theirs
                                       permissions) */
    PRIVIL_READ_VAULT = 0x40,       /* Cen read message stored in data safe */
    PRIVIL_ERASE_VAULT = 0x80       /* Can delete messages from data safe */ 
} isds_priviledges;

/* Message status */
typedef enum {
    MESSAGESTATE_SENT = 0x2,            /* Message has been put into ISDS */
    MESSAGESTATE_STAMPED = 0x4,         /* Message stamped by TSA */
    MESSAGESTATE_INFECTED = 0x8,        /* Message included virues,
                                           infected document has been removed */
    MESSAGESTATE_DELIVERED = 0x10,      /* Message delivered
                                           (dmDeliveryTime stored) */
    MESSAGESTATE_SUBSTITUTED = 0x20,    /* Message delivered through fiction,
                                           dmAcceptanceTime stored */
    MESSAGESTATE_RECEIVED = 0x40,       /* Message accepted (by user login or
                                           user explicit request),
                                           dmAcceptanceTime stored */
    MESSAGESTATE_READ = 0x80,           /* Message has been read by user */
    MESSAGESTATE_UNDELIVERABLE = 0x100, /* Message could not been delivered
                                           (e.g. recipent box has been made
                                           unaccessible meantime) */
    MESSAGESTATE_REMOVED = 0x200,       /* Message content deleted */
    MESSAGESTATE_IN_SAFE = 0x400        /* Message stored in data safe */

} isds_message_status;
#define MESSAGESTATE_ANY 0x7FE          /* Union of all isds_message_status
                                           values */

/* Hash algoritm types */
typedef enum {
    HASH_ALGORITHM_MD5,
    HASH_ALGORITHM_SHA_1,
    HASH_ALGORITHM_SHA_256,
    HASH_ALGORITHM_SHA_512,
} isds_hash_algorithm;

/* Buffer storage strategy.
 * How function should embed application provided buffer into raw element of
 * output structure. */
typedef enum {
    BUFFER_DONT_STORE,      /* Don't fill raw memeber */
    BUFFER_COPY,            /* Copy buffer content into newly allocated raw */
    BUFFER_MOVE             /* Just copy pointer.
                               But leave deallocation to isds_*_free(). */
} isds_buffer_strategy;

/* Hash value storage */
struct isds_hash {
    isds_hash_algorithm algorithm;      /* Hash algoritgm */
    size_t length;                      /* Hash value lenght in bytes */
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
                                       long int beacause xsd:integer
                                       TODO: enum? */
    _Bool *dbEffectiveOVM;          /* Box has OVM role (§ 5a) */
    _Bool *dbOpenAddressing;        /* Non-OVM Box is free to recieve
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
};

/* Message event type */
typedef enum {
    EVENT_UKNOWN,                   /* Event unknown to this library */
    EVENT_ACCEPTED_BY_RECIPIENT,    /* Message has been delivered and accepted
                                       by recipeint action */
    EVENT_ACCEPTED_BY_FICTION,      /* Message has been delivered, acceptance
                                       timed out, considered as accepted */
    EVENT_UNDELIVERABLE,            /* Recipient box made unaccessible,
                                       thus message is undelivarable */
    EVENT_COMMERCIAL_ACCEPTED       /* Recipient confirmed acceptace of
                                       commercial message */
} isds_event_type;

/* Message event
 * Alle members are optional as specification states so. */
struct isds_event {
    struct timeval *time;           /* When the event occurred */
    isds_event_type *type;          /* Type of the event */
    char *description;              /* Human readable event description
                                       generated by ISDS (Czech) */
};

/* Message envelope
 * Be ware that the string length contraints are forced only on output
 * memebers transmitted to ISDS. The other direction (downloded from ISDS)
 * can break these rules. It should not happen, but nobody knows how much
 * incompatible new version of ISDS protocol will be. This is the gold
 * Internet rule: be strict on what you put, be tollerant on what you get. */
struct isds_envelope {
    /* Following memebers apply to incoming messages only: */
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
    char *dmType;                   /* Message type:
                                       "V" is public message
                                       "K" is commercial message */

    /* Following memebers are assigned by ISDS in different phases of message
     * life cycle. */
    unsigned long int *dmOrdinal;   /* Ordinal number in list of
                                       incoming/outgoing messages */
    isds_message_status *dmMessageStatus;  /* Message state */
    long int *dmAttachmentSize;     /* Size of message documents in
                                       kilobytes (rounded). */
    struct timeval *dmDeliveryTime;     /* Time of delivery into a box
                                           NULL, if message has not been
                                           delivered yet */
    struct timeval *dmAcceptanceTime;   /* Time of accpetance of the message
                                           by an user. NULL if message has not
                                           been accepted yet. */
    struct isds_hash *hash;         /* Message hash.
                                       This is hash of isds:dmDM subtree. */
    void *timestamp;                /* Qualified time stamp */
    size_t timestamp_length;        /* Lenght of timestamp in bytes */
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
                                       Optional. Maximal lenght is 50 chars. */
    char *dmRecipientIdent;         /* Czech: spisová značka příjemce; Optional.
                                       Maximal length is 50 characters. */
    char *dmSenderIdent;            /* Czech: spisová značka odesílatele;
                                       Optional. Maximal lenght is 50 chars. */

    /* Act addressing in Czech Republic:
     * Point (Parahraph) § Section Law/Year Coll. */
    long int *dmLegalTitleLaw;      /* Number of act mandating authority */
    long int *dmLegalTitleYear;     /* Year of act issue mandating authority */
    char *dmLegalTitleSect;         /* Section of act mandating authority.
                                       Czech: paragraf */
    char *dmLegalTitlePar;          /* Parahraph of act mandating authority.
                                       Czech: odstavec */
    char *dmLegalTitlePoint;        /* Point of act mandating authority.
                                       Czech: písmeno */

    _Bool *dmPersonalDelivery;      /* If true, only person with higher
                                       priviledges can read this message */
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
                                       Optionable; Implicit value is true. */
};


/* Document type from point of hiearchy */
typedef enum {
    FILEMETATYPE_MAIN,              /* Main document */
    FILEMETATYPE_ENCLOSURE,         /* Appendix */
    FILEMETATYPE_SIGNATURE,         /* Digital signature of other document */
    FILEMETATYPE_META               /* XML document for ESS (electronic
                                       document information system) purposes */
} isds_FileMetaType;

/* Document */
struct isds_document {
    void *data;                     /* Document content.
                                       The encoding and interpretation depends
                                       on dmMimeType.
                                       TODO: inline XML */
    size_t data_length;             /* Length of the data in bytes */
    char *dmMimeType;               /* MIME type of data; Mandatory. */
    isds_FileMetaType dmFileMetaType;   /* Document type to create hierarchy */
    char *dmFileGuid;               /* Message-local document identifier;
                                       Optional. */
    char *dmUpFileGuid;             /* Reference to upper document identifier
                                       (dmFileGuid); Optional. */
    char *dmFileDescr;              /* Document name (title). E.g. file name;
                                       Mandatory. */
    char *dmFormat;                 /* Reference to XML form definition;
                                       Defines howto interpret XML document;
                                       Optional. */
};

/* Raw message representation content type.
 * This is necessary to distinguish between different representations without
 * expensive repated detection.
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
    size_t raw_length;              /* Lenght of raw message in bytes */
    isds_raw_type raw_type;         /* Content type of raw representation
                                       Meaningfull only with non-NULL raw
                                       member */
    struct isds_envelope *envelope; /* Message envelope */
    struct isds_list *documents;    /* List of isds_document's.
                                       Valid message must contain exactly one
                                       document of type FILEMETATYPE_MAIN and
                                       can contain any number of other type
                                       documents. Totol size of documents
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
    char *dmID;                     /* Assigned message ID; Meaningfull only
                                       for error == IE_SUCCESS */
};

/* General linked list */
struct isds_list {
    struct isds_list *next;         /* Next list item,
                                       or NULL if current is last */
    void *data;                     /* Payload */
    void (*destructor) (void **);   /* Payload deallocator */
};

/* Free isds_list with all member data.
 * @list list to free, on return will be NULL */
void isds_list_free(struct isds_list **list);


/* Initialize ISDS library.
 * Global function, must be called before other functions.
 * If it failes you can not use ISDS library and must call isds_cleanup() to
 * free partially inititialized global variables. */
isds_error isds_init(void);

/* Deinicialize ISDS library.
 * Global function, must be called as last library function. */
isds_error isds_cleanup(void);

/* Create ISDS context.
 * Each context can be used for different sessions to (possibly) different
 * ISDS server with different credentials.
 * Returns new context, or NULL */
struct isds_ctx *isds_ctx_create(void);

/* Destroy ISDS context and free memmory.
 * @context will be NULLed on success. */
isds_error isds_ctx_free(struct isds_ctx **context);

/* Return long message text produced by library fucntion, e.g. detailed error
 * mesage. Returned pointer is only valid until new library function is
 * called for the same context. Could be NULL, especially if NULL context is
 * supplied. Return string is locale encoded. */
char *isds_long_message(const struct isds_ctx *context);

/* Set logging up.
 * @facilities is bitmask of isds_log_facility values,
 * @level is verbosity level. */
void isds_set_logging(const unsigned int facilities,
        const isds_log_level level);


/* Set timeout in miliseconds for each network job like connecting to server
 * or sending message. Use 0 to disable timeout limits. */
isds_error isds_set_timeout(struct isds_ctx *context,
        const unsigned int timeout);

/* Function provided by application libsds will call with
 * following five arguments. Value zero of any argument means the value is
 * unknown.
 * @upload_total is expected total upload,
 * @upload_current is cumulative current upload progress
 * @dowload_total is expected total download
 * @download_current is cumulative current download progress
 * @data is pointer that will be passed unchanged to this function at run-time
 * @return 0 to continue HTTP transfaer, or non-zero to abort transfer */
typedef int (*isds_progress_callback)(
        double upload_total, double upload_current,
        double download_total, double download_current,
        void *data);

/* Register callback function libisds calls periodocally during HTTP data
 * transfer.
 * @context is session context
 * @callback is function provided by application libsds will call. See type
 * defition for @callback argument explanation.
 * @data is application specific data @callback gets as last argument */
isds_error isds_set_progress_callback(struct isds_ctx *context,
        isds_progress_callback callback, void *data);

/* Change SSL/TLS settings.
 * @context is context which setting vill be applied to
 * @option is name of option. It determines the type of last argument. See
 * isds_tls_option definition for more info.
 * @... is value of new setting. Type is determined by @option
 * */
isds_error isds_set_tls(struct isds_ctx *context, const isds_tls_option option,
        ...);

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
        const char *certificate, const char* key);

/* Log out from ISDS server and close connection. */
isds_error isds_logout(struct isds_ctx *context);

/* Verify connection to ISDS is alive and server is responding.
 * Sent dumy request to ISDS and expect dummy response. */
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
 * @refnumber is reallocated serial number of request assigned by ISDS. Use
 * NULL, if you don't care.*/
isds_error isds_add_box(struct isds_ctx *context,
        struct isds_DbOwnerInfo *box, const struct isds_list *users,
        const char *former_names, const char *upper_box_id,
        const char *ceo_label, char **refnumber);

/* Notify ISDS about new PFO entity.
 * This function has no real effect.
 * @context is session context
 * @box is PFO description including single primary user.
 * @users is list of struct isds_DbUserInfo (contact address of PFO box owner)
 * @former_names is optional undocumented string. Pass NULL if you don't care.
 * @upper_box_id is optional ID of supper box if currently created box is
 * subordinated.
 * @ceo_label is optional title of OVM box owner (e.g. mayor)
 * @refnumber is reallocated serial number of request assigned by ISDS. Use
 * NULL, if you don't care.*/
isds_error isds_add_pfoinfo(struct isds_ctx *context,
        const struct isds_DbOwnerInfo *box, const struct isds_list *users,
        const char *former_names, const char *upper_box_id,
        const char *ceo_label, char **refnumber);

/* Remove given given box permanetly.
 * @context is session context
 * @box is box description to delete
 * @since is date of box owner cancalation. Only tm_year, tm_mon and tm_mday
 * carry sane value.
 * @refnumber is reallocated serial number of request assigned by ISDS. Use
 * NULL, if you don't care.*/
isds_error isds_delete_box(struct isds_ctx *context,
        const struct isds_DbOwnerInfo *box, const struct tm *since,
        char **refnumber);

/* Update data about given box.
 * @context is session context
 * @old_box current box description
 * @new_box are updated data about @old_box
 * @refnumber is reallocated serial number of request assigned by ISDS. Use
 * NULL, if you don't care.*/
isds_error isds_UpdateDataBoxDescr(struct isds_ctx *context,
        const struct isds_DbOwnerInfo *old_box,
        const struct isds_DbOwnerInfo *new_box,
        char **refnumber);

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
 * @token is NULL if new password should be delivered off-line to the user.
 * It is valid pointer if user should obtain new password on-line on dedicated
 * web server. Then it output automatically reallocated token user needs to
 * use to athtorize on the web server to view his new password. 
 * @refnumber is reallocated serial number of request assigned by ISDS. Use
 * NULL, if you don't care.*/
isds_error isds_reset_password(struct isds_ctx *context,
        const struct isds_DbOwnerInfo *box,
        const struct isds_DbUserInfo *user,
        const _Bool fee_paid,
        char **token, char **refnumber);

/* Assign new user to given box.
 * @context is session context
 * @box is box identification
 * @user defines new user to add
 * @refnumber is reallocated serial number of request assigned by ISDS. Use
 * NULL, if you don't care.*/
isds_error isds_add_user(struct isds_ctx *context,
        const struct isds_DbOwnerInfo *box, const struct isds_DbUserInfo *user,
        char **refnumber);

/* Remove user assigned to given box.
 * @context is session context
 * @box is box identification
 * @user identifies user to removve
 * @refnumber is reallocated serial number of request assigned by ISDS. Use
 * NULL, if you don't care.*/
isds_error isds_delete_user(struct isds_ctx *context,
        const struct isds_DbOwnerInfo *box, const struct isds_DbUserInfo *user,
        char **refnumber);

/* Find boxes suiting given criteria.
 * @context is ISDS session context.
 * @criteria is filter. You should fill in at least some members.
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
 * @refnumber is reallocated serial number of request assigned by ISDS. Use
 * NULL, if you don't care. */
isds_error isds_switch_commercial_receiving(struct isds_ctx *context,
        const char *box_id, const _Bool allow, char **refnumber);

/* Switch box into / out of state where non-OVM box can act as OVM (e.g. force
 * message acceptance). This is just a box permission. Sender must apply
 * such role by sending each message.
 * @context is ISDS session context.
 * @box_id is UTF-8 encoded box identifier as zero terminated string
 * @allow is true for enable, false for disable OVM role permission
 * @refnumber is reallocated serial number of request assigned by ISDS. Use
 * NULL, if you don't care. */
isds_error isds_switch_effective_ovm(struct isds_ctx *context,
        const char *box_id, const _Bool allow, char **refnumber);

/* Switch box accessibility state on request of box owner.
 * Despite the name, owner must do the requst off-line. This function is
 * designed for such off-line meeting points (e.g. Czech POINT).
 * @context is ISDS session context.
 * @box identifies box to swith accesibilty state.
 * @allow is true for making accesibale, false to disallow access.
 * @refnumber is reallocated serial number of request assigned by ISDS. Use
 * NULL, if you don't care. */
isds_error isds_switch_box_accessibility_on_owner_request(
        struct isds_ctx *context, const struct isds_DbOwnerInfo *box,
        const _Bool allow, char **refnumber);

/* Disable box accessibility on law enforcement (e.g. by prison) since exact
 * date.
 * @context is ISDS session context.
 * @box identifies box to swith accesibilty state.
 * @since is date since accesseibility has been denied. This can be past too.
 * Only tm_year, tm_mon and tm_mday carry sane value.
 * @refnumber is reallocated serial number of request assigned by ISDS. Use
 * NULL, if you don't care. */
isds_error isds_disable_box_accessibility_externaly(
        struct isds_ctx *context, const struct isds_DbOwnerInfo *box,
        const struct tm *since,  char **refnumber);

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
        struct isds_message *outgoing_message);

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
        struct isds_list *copies);

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
        struct isds_list **messages);

/* Get list of incoming (addressed to you) messages.
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
 * Use NULL if you don't care about the metadata (useful if you want to know
 * only the @number). If you provide &NULL, list will be allocated on heap,
 * if you provide pointer to non-NULL, list will be freed automacally at first.
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
 * interrested in documents (content) too.
 * Returned hash and timestamp require documents to be verifiable. */
isds_error isds_get_received_envelope(struct isds_ctx *context,
        const char *message_id, struct isds_message **message);

/* Download signed delivery infosheet of given message identified by ID.
 * @context is session context
 * @message_id is message identifier (you can get them from
 * isds_get_list_of_{sent,received}_messages())
 * @message is automatically reallocated message retrieved from ISDS.
 * It will miss documents per se. Use isds_get_signed_received_message(),
 * if you are interrested in documents (content). OTOH, only this function
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

/* Download delivery infosheet of given message identified by ID.
 * @context is session context
 * @message_id is message identifier (you can get them from
 * isds_get_list_of_{sent,received}_messages())
 * @message is automatically reallocated message retrieved from ISDS.
 * It will miss documents per se. Use isds_get_received_message(), if you are
 * interrested in documents (content). OTOH, only this function can get list
 * events message has gone through. */
isds_error isds_get_delivery_info(struct isds_ctx *context,
        const char *message_id, struct isds_message **message);

/* Deprecated: Use isds_load_message() instead. */
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
        struct isds_message **message, const isds_buffer_strategy strategy)
    _deprecated;

/* Download incoming message identified by ID.
 * @context is session context
 * @message_id is message identifier (you can get them from
 * isds_get_list_of_received_messages())
 * @message is automatically reallocated message retrieved from ISDS */
isds_error isds_get_received_message(struct isds_ctx *context,
        const char *message_id, struct isds_message **message);

/* Deprecated: Use isds_load_message() instead. */
/* Load signed message from buffer.
 * @context is session context
 * @outgoing is true if message is outgoing, false if message is incoming
 * @buffer is DER encoded PKCS#7 structure with signed message. You can
 * retrieve such data from message->raw after calling
 * isds_get_signed{received,sent}_message().
 * @length is length of buffer in bytes.
 * @message is automatically reallocated message parsed from @buffer.
 * @strategy selects how buffer will be attached into raw isds_message member.
 * */
isds_error isds_load_signed_message(struct isds_ctx *context,
        const _Bool outgoing, const void *buffer, const size_t length,
        struct isds_message **message, const isds_buffer_strategy strategy)
    _deprecated;

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

/* Download signed incoming message identified by ID.
 * @context is session context
 * @message_id is message identifier (you can get them from
 * isds_get_list_of_received_messages())
 * @message is automatically reallocated message retrieved from ISDS. The raw
 * memeber will be filled with PKCS#7 structure in DER format. */
isds_error isds_get_signed_received_message(struct isds_ctx *context,
        const char *message_id, struct isds_message **message);

/* Download signed outgoing message identified by ID.
 * @context is session context
 * @message_id is message identifier (you can get them from
 * isds_get_list_of_sent_messages())
 * @message is automatically reallocated message retrieved from ISDS. The raw
 * memeber will be filled with PKCS#7 structure in DER format. */
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
 * member will be changed during funcion run. Use envelope on heap only.
 * @return
 *  IE_SUCCESS  if message originates in ISDS
 *  IE_NOTEQUAL if message is unknown to ISDS
 *  other code  for other errors */
isds_error isds_verify_message_hash(struct isds_ctx *context,
        struct isds_message *message);

/* Mark message as read. This is a transactional commit function to acknoledge
 * to ISDS the message has been downloaded and processed by client properly.
 * @context is session context
 * @message_id is message identifier. */
isds_error isds_mark_message_read(struct isds_ctx *context,
        const char *message_id);

/* Mark message as received by recipient. This is applicable only to
 * commercial message. There is no specified way how to distinguishe
 * commercial message from government message yet. Government message is
 * received automatically (by law), commenrcial message on recipient request.
 * @context is session context
 * @message_id is message identifier. */
isds_error isds_mark_message_received(struct isds_ctx *context,
        const char *message_id);

/* Send bogus request to ISDS.
 * Just for test purposes */
isds_error isds_bogus_request(struct isds_ctx *context);

/* Search for document by document ID in list of documents. IDs are compared
 * as UTF-8 string.
 * @documents is list of isds_documents
 * @id is document identifier
 * @return first matching document or NULL. */
const struct isds_document *isds_find_document_by_id(
        const struct isds_list *documents, const char *id);

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

#endif
