#ifndef __ISDS_ISDS_H__
#define __ISDS_ISDS_H__

/* Public interface for libisds.
 * Private declarations in isds_priv.h. */

#include <stdlib.h> /* For size_t */
#include <sys/time.h> /* For struct timeval */

struct isds_ctx;    /* Context for specific ISDS box */

typedef enum {
    IE_SUCCESS = 0, /* No error, just for C conveniece (0 means Ok) */
    IE_ERROR,       /* unsepcified error */
    IE_NOTSUP,
    IE_INVAL,
    IE_INVALID_CONTEXT,
    IE_NOT_LOGGED_IN,
    IE_CONNECTION_CLOSED,
    IE_TIMED_OUT,
    IE_NOEXIST,
    IE_NOMEM,
    IE_NETWORK,
    IE_SOAP,
    IE_XML,
    IE_ISDS,
    IE_ENUM,
    IE_DATE,
    IE_2BIG,
    IE_NOTUNIQ
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
    ILF_ALL  = 0xFF
} isds_log_facility;

/* Return text description of ISDS error */
char *isds_strerror(const isds_error error);

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
 * Instances can be bitmas of any discrete values. */
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
    PRIVIL_OWNER_ADM = 0x20         /* Can administer his box (add/remove
                                       permitted users and theirs
                                       permissions) */
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
    MESSAGESTATE_RECIEVED = 0x40,       /* Message devlivered by user login
                                           dmAcceptanceTime stored */
    MESSAGESTATE_READ = 0x80,           /* Message has been read by user */
    MESSAGESTATE_UNDELIVERABLE = 0x100, /* Message could not been delivered
                                           (e.g. recipent box has been made
                                           unaccessible meantime) */
    MESSAGESTATE_REMOVED = 0x200        /* Message content deleted */

} isds_message_status;
#define MESSAGESTATE_ANY 0x3FE          /* Union of all isds_message_status
                                           values */

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
    char *dbID;                     /* Box ID */
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

/* Message */
struct isds_message {
    void *raw;                      /* Raw message in XML format as send to or
                                       from the ISDS. You can use it to store
                                       local copy. This is binary buffer. */
    size_t *raw_length;             /* Lenght of raw message in bytes */
    struct isds_envelope *envelope; /* Message envelope */
    struct isds_list *documents;    /* List of isds_document's.
                                       Valid message must contain exactly one
                                       document of type FILEMETATYPE_MAIN and
                                       can contain any number of other type
                                       documents. Totol size of documents
                                       must not exceed 10 MB. */
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

/*struct isds_address {
    struct isds_address *next;
    char *box_id;
    char *name;
};

struct isds_message {
    struct isds_message *next;
    struct isds_address *sender;
    struct isds_address *recipient;
    char *subject;
};*/


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
void isds_set_logging(const unsigned int facilities, const isds_log_level level);

/* Connect to given url.
 * It just makes TCP connection to ISDS server found in @url hostname part. */
/*int isds_connect(struct isds_ctx *context, const char *url);*/

/* Set timeout in miliseconds for each network job like connecting to server
 * or sending message. Use 0 to disable timeout limits. */
isds_error isds_set_timeout(struct isds_ctx *context, const unsigned int timeout);


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
isds_error isds_login(struct isds_ctx *context, const char *url, const char *username,
        const char *password, const char *certificate, const char* key);

/* Log out from ISDS server and close connection. */
isds_error isds_logout(struct isds_ctx *context);

/* Verify connection to ISDS is alive and server is responding.
 * Sent dumy request to ISDS and expect dummy response. */
isds_error isds_ping(struct isds_ctx *context);

/* Get data about logged in user and his box. */
isds_error isds_GetOwnerInfoFromLogin(struct isds_ctx *context,
        struct isds_DbOwnerInfo **db_owner_info);

/* Find boxes suiting given criteria.
 * @context is ISDS session context.
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

/* Dwwnload incomping message identified by ID.
 * @context is session context
 * @message_id is message identifier (you can get them from
 * isds_get_list_of_received_messages())
 * @message is automatically reallocated message retrieved from ISDS */
isds_error isds_get_received_message(struct isds_ctx *context,
        const char *message_id, struct isds_message **message);

/* Send bogus request to ISDS.
 * Just for test purposes */
isds_error isds_bogus_request(struct isds_ctx *context);

/*int isds_get_message(struct isds_ctx *context, const unsigned int id,
        struct isds_message **message);
int isds_send_message(struct isds_ctx *context, struct isds_message *message);
int isds_list_messages(struct isds_ctx *context, struct isds_message **message);
int isds_find_recipient(struct isds_ctx *context, const struct address *pattern,
        struct isds_address **address);

int isds_message_free(struct isds_message **message);
int isds_address_free(struct isds_address **address);
*/

/* Search for document by document ID in list of documents. IDs are compared
 * as UTF-8 string.
 * @documents is list of isds_documents
 * @id is document identifier
 * @return first matching document or NULL. */
const struct isds_document *isds_find_document_by_id(
        const struct isds_list *documents, const char *id);

/* Deallocate structure isds_DbOwnerInfo recursively and NULL it */
void isds_DbOwnerInfo_free(struct isds_DbOwnerInfo **db_owner_info);

/* Deallocate struct isds_envelope recurisvely and NULL it */
void isds_envelope_free(struct isds_envelope **envelope);

/* Deallocate struct isds_document recurisvely and NULL it */
void isds_document_free(struct isds_document **document);

/* Deallocate struct isds_message recurisvely and NULL it */
void isds_message_free(struct isds_message **message);

#endif
