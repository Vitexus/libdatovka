#ifndef __ISDS_ISDS_H__
#define __ISDS_ISDS_H__

/* Public interface for libisds.
 * Private declarations in isds_priv.h. */

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
    IE_ISDS
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

/* Box type */
typedef enum {
    DBTYPE_FO,
    DBTYPE_PFO,
    DBTYPE_PFO_ADVOK,
    DBTYPE_PFO_DANPOR,
    DBTYPE_PFO_INSSPR,
    DBTYPE_PO,
    DBTYPE_PO_ZAK,
    DBTYPE_PO_REQ,
    DBTYPE_OVM,
    DBTYPE_OVM_NOTAR,
    DBTYPE_OVM_EXEKUT,
    DBTYPE_OVM_REQ
} isds_DbType;

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

/* Data about box and his owner */
struct isds_DbOwnerInfo {
    char *dbID;                     /* Box ID */
    isds_DbType dbType;             /* Box Type */
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
    int dbState;                    /* Box state; 1 <=> active box;
                                       TODO: enum? */
    _Bool dbEffectiveOVM;           /* Box has OVM role (§ 5a) */
};

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
 * called. */
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

/* Deallocate structure isds_DbOwnerInfo recursively and NULL it */
void isds_DbOwnerInfo_free(struct isds_DbOwnerInfo **db_owner_info);

#endif
