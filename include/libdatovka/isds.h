#ifndef __ISDS_ISDS_H__
#define __ISDS_ISDS_H__

/* Public interface for libdatovka.
 * Private declarations in isds_priv.h. */

#include <stdint.h> /* int32_t, int64_t */
#include <stdlib.h> /* For size_t */
#include <time.h>   /* For struct tm */
#include <libxml/tree.h>    /* For xmlDoc and xmlNodePtr */

#ifdef __cplusplus  /* For C++ linker sake */
extern "C" {
#endif

/*
 * Numeric release version identifier:
 * MMNNRR0S: MM = major, NN == minor, RR == release (patch), S = status
 * The status nibble has holds the value 0 for release other values identify
 * developmental builds.
 * For example:
 * 0.1.0 release             0x00010000
 * 0.1.0 further development 0x00010001
 * 0.1.2                     0x00010200
 * 1.2.3                     0x01020300
 */
#define ISDS_LIB_VER_NUM 0x00030000L
#define ISDS_LIB_VER_STR "libdatovka 0.3.0"

/*
 * Can be used like:
 * #if (ISDS_LIB_VER_NUM >= ISDS_LIB_VER_CHECK(0, 1, 0))
 */
#define ISDS_LIB_VER_CHECK(major, minor, release) \
	(((major) << 24) | ((minor) << 16) | (release << 8))

/*
 * Return version number of this library.
 * This may a different version than the version the application has been
 * compiled against.
 */
unsigned long isds_lib_ver_num(void);

/*
 * Return version of this library.
 * This may a different version than the version the application has been
 * compiled against.
 */
const char *isds_lib_ver_str(void);

/* _deprecated macro marks library symbols as deprecated. Application should
 * avoid using such function as soon as possible. */
#if defined(__GNUC__)
#define _deprecated __attribute__((deprecated))
#else
#define _deprecated
#endif

/* Service locators */
/* Base URL of production ISDS instance */
extern const char isds_locator[]; /* Without client certificate authentication. */
extern const char isds_cert_locator[]; /* With client certificate authentication. */
extern const char isds_vodz_locator[]; /* High-volume data message without client certificate. */
extern const char isds_vodz_cert_locator[]; /* High-volume data message without client certificate. */
extern const char isds_otp_locator[]; /* With OTP authentication. */
extern const char isds_mep_locator[]; /* Mobile key application. */
/* Base URL of testing ISDS instance */
extern const char isds_testing_locator[]; /* Without client certificate */
extern const char isds_cert_testing_locator[]; /* With client certificate */
extern const char isds_vodz_testing_locator[]; /* High-volume data message without client certificate. */
extern const char isds_vodz_cert_testing_locator[]; /* High-volume data message without client certificate. */
extern const char isds_otp_testing_locator[]; /* With OTP authentication */
extern const char isds_mep_testing_locator[]; /* Mobile key application. */


struct isds_ctx;    /* Context for specific ISDS box */

/*
 * Struct timeval doesn't guarantee a 64-bit-wide second counter across all
 * systems.
 * Mingw uses 'long' which however is only 32-bit wide even on 64-bit Windows.
 * FreeBSD 13.0 on amd64 uses 64-bit-wide time_t but mktime() doesn't work past
 * year 2038.
 */
struct isds_timeval {
	int64_t tv_sec;
	int32_t tv_usec;
};

typedef enum isds_error {
    IE_SUCCESS = 0, /* No error, just for C convenience (0 means Ok) */
    IE_ERROR,       /* Unspecified error */
    IE_NOTSUP,
    IE_INVAL,
    IE_INVALID_CONTEXT,
    IE_NOT_LOGGED_IN,
    IE_CONNECTION_CLOSED,
    IE_TIMED_OUT,
    IE_NONEXIST,
    IE_NOMEM,
    IE_NETWORK,
    IE_HTTP,
    IE_SOAP,
    IE_XML,
    IE_ISDS,
    IE_ENUM,
    IE_DATE,
    IE_TOO_BIG,
    IE_TOO_SMALL,
    IE_NOTUNIQ,
    IE_NOTEQUAL,
    IE_PARTIAL_SUCCESS,
    IE_ABORTED,
    IE_SECURITY
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

/* library options */
typedef enum isds_option {
    IOPT_TLS_VERIFY_SERVER = 1, /* _Bool: Verify server identity?
                                   Default value is true. */
    IOPT_TLS_CA_FILE = 2, /* char *: File name with CA certificates.
                             Default value depends on cryptographic library. */
    IOPT_TLS_CA_DIRECTORY = 3, /* char *: Directory name with CA certificates.
                                  Default value depends on cryptographic
                                   library. */
    IOPT_TLS_CRL_FILE = 4, /* char *: File  name with CRL in PEM format.
                              Default value depends on cryptographic library. */
    IOPT_NORMALIZE_MIME_TYPE = 5 /* _Bool: Normalize MIME type values?
                                    Default value is false. */
} isds_option;

/* library TLS options */
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

/* One-time password authentication method */
typedef enum {
    OTP_HMAC = 0,           /* HMAC-based OTP method */
    OTP_TIME                /* Time-based OTP method */
} isds_otp_method;

/* One-time password authentication resolution */
typedef enum {
    OTP_RESOLUTION_SUCCESS = 0,         /* Authentication succeeded */
    OTP_RESOLUTION_UNKNOWN,             /* Status is unknown */
    OTP_RESOLUTION_BAD_AUTHENTICATION,  /* Bad log-in, retry */
    OTP_RESOLUTION_ACCESS_BLOCKED,      /* Access blocked for 60 minutes
                                           (brute force attack detected) */
    OTP_RESOLUTION_PASSWORD_EXPIRED,    /* Password has expired.
                                           ???: OTP or regular password
                                           expired? */
    OTP_RESOLUTION_TOO_FAST,            /* OTP cannot be sent repeatedly
                                           at this rate (minimal delay
                                           depends on TOTP window setting) */
    OTP_RESOLUTION_UNAUTHORIZED,        /* User name is not allowed to
                                           access requested URI */
    OTP_RESOLUTION_TOTP_SENT,           /* OTP has been generated and sent by
                                           ISDS */
    OTP_RESOLUTION_TOTP_NOT_SENT,       /* OTP could not been sent.
                                           Retry later. */
} isds_otp_resolution;

/* One-time password to authenticate client */
struct isds_otp {
    /* Input members */
    isds_otp_method method;         /* Select OTP method to use */
    char *otp_code;                 /* One-time password to use. Pass NULL,
                                       if you do not know it yet (e.g. in case
                                       of first phase of time-based OTP to
                                       request new code from ISDS.) */
    /* Output members */
    isds_otp_resolution resolution; /* Fine-grade resolution of OTP
                                       authentication attempt. */
};

/* Mobile key authentication resolution. */
typedef enum {
    MEP_RESOLUTION_SUCCESS = 0,    /* Authentication succeeded */
    MEP_RESOLUTION_UNKNOWN,        /* Status is unknown */
    MEP_RESOLUTION_UNRECOGNISED,   /* Authentication request not recognised. */
    MEP_RESOLUTION_ACK_REQUESTED,  /* Waiting for acknowledgement. */
    MEP_RESOLUTION_ACK,            /* Acknowledged. */
    MEP_RESOLUTION_ACK_EXPIRED     /* Acknowledgement request expired. */
} isds_mep_resolution;

/* Mobile key context to authenticate client */
struct isds_mep {
    /* Input members. */
    char *app_name;                 /* Client application name. This name is
                                       displayed in the mobile key authentication
                                       application to provide the user brief
                                       information about which application is
                                       requiring access to the data box. */
    /* Intermediate members. */
    char *intermediate_uri;         /* Intermediate authentication URI. */
    /* Output members. */
    isds_mep_resolution resolution; /* Fine-grade resolution of mobile key
                                       authentication attempt. */
};

/* Type of status message. Can refer to dbStatus or dmStatus. */
typedef enum isds_status_type {
    STAT_DB = 0, /* Status returned inside the dbStatus element. */
    STAT_DM      /* Status returned inside the dmStatus element. */
} isds_status_type;

/* Status as returned by many ISDS operations. All strings are UTF-8 encoded. */
struct isds_status {
    enum isds_status_type type; /* Type of the status description. */
    char *code;                 /* Value of the *StatusCode element. */
    char *message;              /* Value of the *StatusMessage element. */
    char *ref_number;           /* Value of the dbStatusRefNumber element if available. */
};

/* Box type */
typedef enum {
    DBTYPE_OVM_MAIN = -1,       /* Special value for
                                   isds_find_box_by_fulltext(). */
    DBTYPE_SYSTEM = 0,          /* This is special sender value for messages
                                   sent by ISDS. */
    DBTYPE_OVM = 10,
    DBTYPE_OVM_NOTAR = 11,      /* This type has been replaced with OVM_PFO. */
    DBTYPE_OVM_EXEKUT = 12,     /* This type has been replaced with OVM_PFO. */
    DBTYPE_OVM_REQ = 13,
    DBTYPE_OVM_FO = 14,
    DBTYPE_OVM_PFO = 15,
    DBTYPE_OVM_PO = 16,
    DBTYPE_PO = 20,
    DBTYPE_PO_ZAK = 21,         /* This type has been replaced with PO. */
    DBTYPE_PO_REQ = 22,
    DBTYPE_PFO = 30,
    DBTYPE_PFO_ADVOK = 31,
    DBTYPE_PFO_DANPOR = 32,
    DBTYPE_PFO_INSSPR = 33,
    DBTYPE_PFO_AUDITOR = 34,
    DBTYPE_PFO_ZNALEC = 35,
    DBTYPE_PFO_TLUMOCNIK = 36,
    DBTYPE_PFO_REQ = 50,
    DBTYPE_FO = 40
} isds_DbType;

/* Box status from point of view of accessibility */
typedef enum {
    DBSTATE_ACCESSIBLE = 1,
    DBSTATE_TEMP_INACCESSIBLE = 2,
    DBSTATE_NOT_YET_ACCESSIBLE = 3,
    DBSTATE_PERM_INACCESSIBLE = 4,
    DBSTATE_REMOVED = 5,
    DBSTATE_TEMP_INACCESSIBLE_LAW = 6
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
    PRIVIL_READ_VAULT = 0x40,       /* Can read message stored in long term
                                       storage (does not exists since
                                       2012-05) */
    PRIVIL_ERASE_VAULT = 0x80       /* Can delete messages from long term
                                       storage */
} isds_privileges;

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
    MESSAGESTATE_IN_VAULT = 0x400       /* Message stored in long term storage */
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

/* Name of person version 2. Since WSDL 2.31. */
struct isds_PersonName2 {
    char *pnGivenNames;    /* First name and other given (middle) names */
    char *pnLastName;      /* Family name */
};

/* Date and place of birth */
struct isds_BirthInfo {
    struct tm *biDate;      /* Date of birth in local time at birth place,
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

/* Post address version 3. Since WSDL 2.31. */
struct isds_AddressExt2 {
    char *adCode;                    /* RUIAN address code */
    char *adCity;
    char *adDistrict;                /* Part of the municipality */
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

/* Data about box and its respective owner version 3. Since WSDL 2.31.
 * NULL pointer means undefined value */
struct isds_DbOwnerInfoExt2 {
    char *dbID;                     /* Box ID [Max. 7 chars] */
    _Bool *aifoIsds;                /* Set if box owner data are held and
                                       synchronised with the Person registry
                                       (Regist osob/ROB) */
    isds_DbType *dbType;            /* Box type */
    char *ic;                       /* ID */
    struct isds_PersonName2 *personName; /* Name of person */
    char *firmName;                 /* Name of firm */
    struct isds_BirthInfo *birthInfo;    /* Birth of person */
    struct isds_AddressExt2 *address;    /* Post address */
    char *nationality;
    char *dbIdOVM;                  /* ID from the OVM registry */
    long int *dbState;              /* Box state; 1 <=> active box;
                                       long int because xsd:integer
                                       TODO: enum? */
    _Bool *dbOpenAddressing;        /* Non-OVM Box is free to receive
                                       messages from anybody */
    char *dbUpperID;                /* ID of superordinate OVM data box */
};

/* User type */
typedef enum {
    USERTYPE_PRIMARY,               /* Owner of the box */
    USERTYPE_ENTRUSTED,             /* User with limited access to the box */
    USERTYPE_ADMINISTRATOR,         /* User to manage ENTRUSTED_USERs */
    USERTYPE_OFFICIAL,              /* ??? */
    USERTYPE_OFFICIAL_CERT,         /* ??? */
    USERTYPE_LIQUIDATOR,            /* Company liquidator */
    USERTYPE_RECEIVER,              /* Company receiver */
    USERTYPE_GUARDIAN               /* Legal guardian */
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

/* Data about user version 3. Since WSDL 2.31.
 * NULL pointer means undefined value */
struct isds_DbUserInfoExt2 {
    _Bool *aifoIsds;            /* Set if user data are held within
                                   the Person registry (Registr obyvatel/ROB) */
    struct isds_PersonName2 *personName; /* Name of the person */
    struct isds_AddressExt2 *address;    /* Post address */
    struct tm *biDate;          /* Date of birth in local time,
                                   only tm_year, tm_mon and tm_mday carry sane
                                   value */
    char *isdsID;               /* Identifier without significance, unique for
                                   each user. It does not change when new
                                   login credentials are handed out. */
    isds_UserType *userType;    /* User type */
    long int *userPrivils;      /* Set of user permissions */
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
    EVENT_UNKNOWN,                  /* Event unknown to this library */
    EVENT_ACCEPTED_BY_RECIPIENT,    /* Message has been delivered and accepted
                                       by recipient action */
    EVENT_ACCEPTED_BY_FICTION,      /* Message has been delivered, acceptance
                                       timed out, considered accepted */
    EVENT_ACCEPTED_BY_FICTION_NO_USER, /* Message has been delivered, acceptance
                                          timed out because there was no user
                                          who could accept the message */
    EVENT_UNDELIVERABLE,            /* Recipient box made inaccessible,
                                       thus message is undeliverable */
    EVENT_COMMERCIAL_ACCEPTED,      /* Recipient confirmed acceptance of
                                       commercial message */
    EVENT_ENTERED_SYSTEM,           /* Message entered ISDS, i.e. has been just
                                       sent by sender */
    EVENT_DELIVERED,                /* Message has been delivered */
    EVENT_PRIMARY_LOGIN,            /* Primary user has logged in */
    EVENT_ENTRUSTED_LOGIN,          /* Entrusted user with capability to read
                                       has logged in */
    EVENT_SYSCERT_LOGIN,            /* Application authenticated by `system'
                                       certificate has logged in */
    EVENT_UNDELIVERED_AV_CHECK      /* An attachment didn't pass the antivirus
                                       check, message has not been delivered */
} isds_event_type;

/* Message event
 * All members are optional as specification states so. */
struct isds_event {
    struct isds_timeval *time; /* When the event occurred */
    isds_event_type *type; /* Type of the event */
    char *description; /*
                        * Human readable event description generated by ISDS
                        * (in Czech).
                        */
};

/*
 * Allows the sender to publish specified information about his person when
 * sending a message.
 */
typedef enum isds_IdLevel_value {
    PUBLISH_USERTYPE = 0, /* The sender user type isds_UserType is always enabled. */
    PUBLISH_PERSONNAME = 1, /* Publish name. Comprises pnGivenNames and pnLastName. */
    PUBLISH_BIDATE = 2, /* Publish biDate. */
    PUBLISH_BICITY = 4, /* Publish biCity - only when sender is SENDERTYPE_ENTRUSTED of a FO or PFO box. */
    PUBLISH_BICOUNTY = 8, /* Publish biCounty - only when sender is SENDERTYPE_ENTRUSTED of a FO or PFO box. */
    PUBLISH_ADCODE = 16, /* Publish adCode (RUIAN address code). */
    PUBLISH_FULLADDRESS = 32, /* Publish fullAddress. */
    PUBLISH_ROBIDENT = 64 /* Publish robIdent - flag whether the person is identified within the ROB (Registr obyvatel/citizen registry). */
} isds_IdLevel_value;

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
    struct isds_timeval *dmDeliveryTime; /* Time of delivery into a box
                                            NULL, if message has not been
                                            delivered yet */
    struct isds_timeval *dmAcceptanceTime;/* Time of acceptance of the message
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
    char *dmType;                   /* Message type (commercial subtypes or
                                       government message):
                                       Input values (when sending a message):
                                       "I" is commercial message offering
                                           paying the response (initiatory
                                           message);
                                           it's necessary to define
                                           dmSenderRefNumber
                                       "K" is commercial message paid by sender
                                           if this message
                                       "O" is commercial response paid by
                                           sender of initiatory message; it's
                                           necessary to copy value from
                                           dmSenderRefNumber of initiatory
                                           message to dmRecipientRefNumber
                                           of this message
                                       "V" is non-commercial government message
                                       Default value while sending is undefined
                                       which has the same meaning as "V".
                                       Output values (when retrieving
                                       a message):
                                       "A" is subsidized initiatory commercial
                                           message which can pay a response
                                       "B" is subsidized initiatory commercial
                                           message which has already paid the
                                           response
                                       "C" is subsidized initiatory commercial
                                           message where the response offer has
                                           expired
                                       "D" is externally subsidized commercial
                                           message
                                       "E" is commercial message prepaid by
                                           a stamp
                                       "G" is commercial message paid by
                                           a sponsor
                                       "I"
                                       "K"
                                       "O"
                                       "V"
                                       "X" is initiatory commercial message
                                           where the response offer has expired
                                       "Y" initiatory commercial message which
                                           has already paid the response
                                       "Z" is limitedly subsidized commercial
                                           message
                                       Length: Exactly 1 UTF-8 character if
                                       defined; */
    _Bool *dmVODZ;                  /* True when receiving high-volume data
                                       messages. */

    /* Following members apply to outgoing messages only: */
    _Bool *dmOVM;                   /* OVM sending mode.
                                       Non-OVM dbType boxes that has
                                       dbEffectiveOVM == true MUST select
                                       between true (OVM mode) and
                                       false (non-OVM mode).
                                       Optional; Implicit value is true. */
    _Bool *dmPublishOwnID;          /* Allow sender to express his name shall
                                       be available to recipient by
                                       isds_get_message_sender(). Sender type
                                       will be always available.
                                       Optional; Default value is false. */
    int *idLevel;                   /* Sum of isds_IdLevel_value. Additionally
                                       specifies which personal information
                                       the user wants to publish when sending
                                       a message. */
};


/* Document type from point of hierarchy */
typedef enum isds_FileMetaType {
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

/* Attachment, used for high-volume data messages. */
struct isds_dmFile {
	void *data; /*
	             * Document content.
	             * The encoding and interpretation depends on dmMimeType.
	             */
	size_t data_length; /* Length of the data in bytes. */
	isds_FileMetaType dmFileMetaType; /* Document type to create hierarchy. */
	char *dmMimeType; /* MIME type of the data; Mandatory. */
	char *dmFileDescr; /* Document name (title). E.g. file name; Mandatory. */
};

/*
 * Response for UploadAttachment.
 * Complete attachment identification.
 * Encapsulates the attachment identifier and hashes for high-volume data messages.
 */
struct isds_dmAtt {
	char *dmAttID; /* Attachment identifier, nothing to do with message identifier. */
	char *dmAttHash1; /* Hash1 value. */
	char *dmAttHash1Alg; /* Hash1 algorithm identifier. */
	char *dmAttHash2; /* Hash2 value. */
	char *dmAttHash2Alg; /* Hash2 algorithm identifier. */
};

/* Attachment, used for high-volume data messages. */
struct isds_dmExtFile {
	enum isds_FileMetaType dmFileMetaType; /* Document type to create hierarchy. */
	struct isds_dmAtt dmAtt; /* Complete attachment identification. */
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
                                       must not exceed 20 MB. */
    struct isds_list *ext_files; /*
                                  * List of isds_dmExtFile entries.
                                  * This list if only used when sending
                                  * high-volume data messages. Then sending
                                  * ordinary data messages the list must be empty.
                                  * Valid message must contain exactly one
                                  * document or extended file entry of the type
                                  * FILEMETATYPE_MAIN.
                                  * Total size of documents must not exceed 2 GB
                                  * for high-volume data messages.
                                  */
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
    isds_error error;               /* libdatovka compatible error of delivery
                                       to one recipient */
    char *dmStatus;                 /* Error description returned by ISDS;
                                       Optional. */
    char *dmID;                     /* Assigned message ID; Meaningful only
                                       for error == IE_SUCCESS */
};

/* Message state change event */
struct isds_message_status_change {
    char *dmID; /* Message ID. */
    isds_message_status *dmMessageStatus; /* Message state */
    struct isds_timeval *time; /* When the state changed */
};

/* How outgoing commercial message gets paid */
typedef enum {
    PAYMENT_SENDER,             /* Paid by sender */
    PAYMENT_STAMP,              /* Pre-paid by a sender */
    PAYMENT_SPONSOR,            /* A sponsor pays all messages */
    PAYMENT_RESPONSE,           /* Recipient pays a response */
    PAYMENT_SPONSOR_LIMITED,    /* Undocumented */
    PAYMENT_SPONSOR_EXTERNAL    /* Undocumented */
} isds_payment_type;

/* Permission to send commercial message */
struct isds_commercial_permission {
    isds_payment_type type; /* Payment method */
    char *recipient; /* Send to this box ID only; NULL means to anybody. */
    char *payer; /* Owner of this box ID pays */
    struct isds_timeval *expiration; /*
                                      * This permissions is valid until;
                                      * NULL means indefinitely.
                                      */
    unsigned long int *count; /*
                               * Number of messages that can be sent on this
                               * permission; NULL means unlimited.
                               */
    char *reply_identifier; /*
                             * Identifier to pair request and response message.
                             * Meaningful only with type PAYMENT_RESPONSE.
                             */
};

/* Type of commercial message. */
typedef enum isds_commercial_message_type {
    COMMERCIAL_NORMAL = 0, /* Normal commercial message. */
    COMMERCIAL_INIT /* Initiatory commercial message. */
} isds_commercial_message_type;

/*
 * Data vault (long term storage) type.
 * Described in pril_2/WS_ISDS_vyhledavani_datovych_schranek.pdf
 *     section 2.8.
 */
typedef enum isds_vault_type {
	VAULT_NONE = 0, /* Long term storage is inactive. */
	VAULT_PREPAID = 1, /* Long term storage is active using a prepaid service. */
	VAULT_UNUSED_2 = 2, /* This type was used until 2013. No more used since then. */
	VAULT_CONTRACTUAL = 3, /* Long term storage is active based on a contract. */
	VAULT_TRIAL = 4, /* Long term storage is active to try it out. */
	VAULT_UNUSED_5 = 5, /* No more used. */
	VAULT_SPECIAL_OFFER = 6 /* Long term storage is active because of an special offer. */
} isds_vault_type;

/*
 * Data vault (long term storage) payment state.
 * Described in pril_2/WS_ISDS_vyhledavani_datovych_schranek.pdf
 *     section 2.8.
 */
typedef enum isds_vault_payment_status {
	VAULT_NOT_PAID_YET = 0, /* Long term storage has not been paid yet. */
	VAULT_PAID_ALREADY = 1 /* Long term storage has already been paid. */
} isds_vault_payment_status;

/*
 * Data vault (long term storage) information structure.
 * Described in pril_2/WS_ISDS_vyhledavani_datovych_schranek.pdf
 *     section 2.8.
 */
struct isds_DTInfoOutput {
	enum isds_vault_type *actDTType; /*
	                                 * Type of the active long term storage.
	                                 * It is suggested in dbTypes.xsd that this value may not be presented.
	                                 */
	unsigned long int *actDTCapacity; /* The capacity of the long term storage. */
	struct tm *actDTFrom; /* Inception date of the current active status. */
	struct tm *actDTTo; /* Termination date of the current active status. */
	unsigned long int *actDTCapUsed; /* Used capacity in units of messages. */
	enum isds_vault_type *futDTType; /*
	                                 * Type of the future long term storage.
	                                 * It is suggested in dbTypes.xsd that this value may not be presented.
	                                 */
	unsigned long int *futDTCapacity; /* Ordered capacity of the long term storage. */
	struct tm *futDTFrom; /* Ordered from. */
	struct tm *futDTTo; /* Ordered to. */
	enum isds_vault_payment_status *futDTPaid; /* Acknowledgement of payment. */
};

/* Type of credit change event */
typedef enum {
    ISDS_CREDIT_CHARGED,        /* Credit has been charged */
    ISDS_CREDIT_DISCHARGED,     /* Credit has been discharged */
    ISDS_CREDIT_MESSAGE_SENT,   /* Credit has been spent for sending
                                   a commercial message */
    ISDS_CREDIT_STORAGE_SET,    /* Credit has been spent for setting
                                   a long-term storage */
    ISDS_CREDIT_EXPIRED         /* Credit has expired */
} isds_credit_event_type;

/* Data specific for ISDS_CREDIT_CHARGED isds_credit_event_type */
struct isds_credit_event_charged {
    char *transaction;              /* Transaction identifier;
                                       NULL-terminated string. */
};

/* Data specific for ISDS_CREDIT_DISCHARGED isds_credit_event_type */
struct isds_credit_event_discharged {
    char *transaction;              /* Transaction identifier;
                                       NULL-terminated string. */
};

/* Data specific for ISDS_CREDIT_MESSAGE_SENT isds_credit_event_type */
struct isds_credit_event_message_sent {
    char *recipient;                /* Recipient's box ID of the sent message */
    char *message_id;               /* ID of the sent message */
};

/* Data specific for ISDS_CREDIT_STORAGE_SET isds_credit_event_type */
struct isds_credit_event_storage_set {
    long int new_capacity;          /* New storage capacity. The unit is
                                       a message. */
    struct tm *new_valid_from;      /* The new capacity is available since
                                       date. */
    struct tm *new_valid_to;        /* The new capacity expires on date. */
    long int *old_capacity;         /* Previous storage capacity; Optional.
                                       The unit is a message. */
    struct tm *old_valid_from;      /* Date; Optional; Only tm_year,
                                       tm_mon, and tm_mday carry sane value. */
    struct tm *old_valid_to;        /* Date; Optional. */
    char *initiator;                /* Name of a user who initiated this
                                       change; Optional. */
};

/* Event about change of credit for sending commercial services */
struct isds_credit_event {
    /* Common fields */
    struct isds_timeval *time; /* When the credit was changed. */
    long int credit_change; /*
                             * Difference in credit value caused by
                             * this event. The unit is 1/100 CZK.
                             */
    long int new_credit; /*
                          * Credit value after this event.
                          * The unit is 1/100 CZK.
                         */
    isds_credit_event_type type; /* Type of the event */

    /* Details specific for the type */
    union {
        struct isds_credit_event_charged charged;
                                                /* ISDS_CREDIT_CHARGED */
        struct isds_credit_event_discharged discharged;
                                                /* ISDS_CREDIT_DISCHARGED */
        struct isds_credit_event_message_sent message_sent;
                                                /* ISDS_CREDIT_MESSAGE_SENT */
        struct isds_credit_event_storage_set storage_set;
                                                /* ISDS_CREDIT_STORAGE_SET */
    } details;
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
    char *reference;                /* Identifier of the approval */
};

/* Message sender type.
 * Similar but not equivalent to isds_UserType. */
typedef enum {
    SENDERTYPE_PRIMARY,             /* Owner of the box */
    SENDERTYPE_ENTRUSTED,           /* User with limited access to the box */
    SENDERTYPE_ADMINISTRATOR,       /* User to manage ENTRUSTED_USERs */
    SENDERTYPE_OFFICIAL,            /* ISDS; sender of system message */
    SENDERTYPE_VIRTUAL,             /* An application (e.g. document
                                       information system) */
    SENDERTYPE_OFFICIAL_CERT,       /* ???; Non-normative */
    SENDERTYPE_LIQUIDATOR,          /* Liquidator of the company; Non-normative */
    SENDERTYPE_RECEIVER,            /* Receiver of the company */
    SENDERTYPE_GUARDIAN             /* Legal guardian */
} isds_sender_type;

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

/* Box attribute to search while performing full-text search */
typedef enum {
    FULLTEXT_ALL,       /* search in address, organization identifier, and
                           box id */
    FULLTEXT_ADDRESS,   /* search in address */
    FULLTEXT_IC,        /* search in organization identifier */
    FULLTEXT_BOX_ID     /* search in box ID */
} isds_fulltext_target;

/* A box matching full-text search */
struct isds_fulltext_result {
    char *dbID;                 /* Box ID */
    isds_DbType dbType;         /* Box Type */
    char *name;                 /* Subject owning the box */
    struct isds_list *name_match_start;     /* List of pointers into `name'
                                               field string. Point to first
                                               characters of a matched query
                                               word. */
    struct isds_list *name_match_end;       /* List of pointers into `name'
                                               field string. Point after last
                                               characters of a matched query
                                               word. */
    char *address;              /* Post address */
    struct isds_list *address_match_start;  /* List of pointers into `address'
                                               field string. Point to first
                                               characters of a matched query
                                               word. */
    struct isds_list *address_match_end;    /* List of pointers into `address'
                                               field string. Point after last
                                               characters of a matched query
                                               word. */
    char *ic;                   /* Organization identifier */
    struct tm *biDate;          /* Date of birth in local time at birth place,
                                   only tm_year, tm_mon and tm_mday carry sane
                                   value */
    _Bool dbEffectiveOVM;       /* Box has OVM role (§ 5a) */
    _Bool active;               /* Box is active */
    _Bool public_sending;       /* Current box can send non-commercial
                                   messages into this box */
    _Bool commercial_sending;   /* Current box can send commercial messages
                                   into this box */
};

/* A box state valid in the time range */
struct isds_box_state_period {
    struct isds_timeval from; /* Time range beginning */
    struct isds_timeval to; /* Time range end */
    long int dbState; /*
                       * Box state; 1 <=> active box, otherwise inaccessible;
                       * use isds_DbState enum to identify some states.
                       */
};

/*
 * Response for GetMessageAuthor2.
 * Described in pril_2/WS_manipulace_s_datovymi_zpravami.pdf,
 *     section 2.10.
 */
struct isds_dmMessageAuthor {
    isds_sender_type *userType; /* Message sender type. */
    struct isds_PersonName2 *personName; /* Contains pnGivenNames and pnLastName. */
    struct tm *biDate; /* Date of birth in local time at birth place,
                          only tm_year, tm_mon and tm_mday carry sane
                          value. */
    char *biCity;
    char *biCounty; /* German: Bezirk, Czech: okres */
    char *adCode; /* RUIAN address code */
    char *fullAddress;
    _Bool *robIdent; /* Flag whether the person is identifiers within the ROB. */
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

/*
 * Set a different function to replace the default timegm() function.
 * @f_gmtime_r is a function pointer to the replacement function which always
 * returns a 64-bit integer to represent a time value.
 */
void isds_set_func_timegm(int64_t (*f_timegm)(struct tm *));

/*
 * Check whether the timegm() function in use behaves as expected.
 */
isds_error isds_check_func_timegm(struct isds_ctx *context);

/*
 * Set a different function to replace the default gmtime_r() function.
 * @f_gmtime_r is a function pointer to the replacement function which always
 * takes a 64-bit integer to represent a time value.
 */
void isds_set_func_gmtime_r(struct tm *(*f_gmtime_r)(const int64_t *, struct tm *));

/*
 * Check whether the gmtime_r() function in use behaves as expected.
 */
isds_error isds_check_func_gmtime_r(struct isds_ctx *context);

/* Destroy ISDS context and free memory.
 * @context will be set to NULL on success. */
isds_error isds_ctx_free(struct isds_ctx **context);

/* Return long message text produced by library function, e.g. detailed error
 * message. Returned pointer is only valid until new library function is
 * called for the same context. Could be NULL, especially if NULL context is
 * supplied. Return string is locale encoded. */
char *isds_long_message(const struct isds_ctx *context);

/* Return status description of the last performed ISDS operation.
 * Returned pointer is only valid until new library function is called for
 * the supplied context. Can be NULL. */
const struct isds_status *isds_operation_status(const struct isds_ctx *context);

/* Set logging up.
 * @facilities is bit mask of isds_log_facility values,
 * @level is verbosity level. */
void isds_set_logging(const unsigned int facilities,
        const isds_log_level level);

/* Function provided by application which the library will call to pass a log
 * message. The message is usually locale encoded, but raw strings
 * (UTF-8 usually) can occur when logging raw communication with ISDS servers.
 * Infixed zero byte is not excluded, but should not present. Use @length
 * argument to get real length of the message.
 * TODO: We will try to fix the encoding issue
 * @facility is log message class
 * @level is log message severity
 * @message is string with zero byte terminator. This can be any arbitrary
 * chunk of a sentence with or without new line, a sentence can be split
 * into more messages. However it should not happen. If you discover message
 * without new line, report it as a bug.
 * @length is size of @message string in bytes excluding trailing zero
 * @data is pointer that will be passed unchanged to this function at run-time
 * */
typedef void (*isds_log_callback)(
        isds_log_facility facility, isds_log_level level,
        const char *message, int length, void *data);

/* Register a callback function which the library calls when new global log
 * message is produced by the library. Library logs to stderr by default.
 * @callback is function provided by application which the library will call.
 * See type definition for @callback argument explanation. Pass NULL to revert
 * logging to default behaviour.
 * @data is application specific data @callback gets as last argument */
void isds_set_log_callback(isds_log_callback callback, void *data);

/* Set timeout in milliseconds for each network job like connecting to server
 * or sending message. Use 0 to disable timeout limits. */
isds_error isds_set_timeout(struct isds_ctx *context,
        const unsigned int timeout);

/* Function provided by application which the library will call with
 * following five arguments. Value zero of any argument means the value is
 * unknown.
 * @upload_total is expected total upload,
 * @upload_current is cumulative current upload progress
 * @download_total is expected total download
 * @download_current is cumulative current download progress
 * @data is pointer that will be passed unchanged to this function at run-time
 * @return 0 to continue HTTP transfer, or non-zero to abort transfer */
typedef int (*isds_progress_callback)(
        double upload_total, double upload_current,
        double download_total, double download_current,
        void *data);

/* Register a callback function which the library calls periodically during
 * a HTTP data transfer.
 * @context is session context
 * @callback is function provided by application which the library will call.
 * See type definition for @callback argument explanation.
 * @data is application specific data @callback gets as last argument */
isds_error isds_set_progress_callback(struct isds_ctx *context,
        isds_progress_callback callback, void *data);

/*
 * Change context settings.
 * @context is the context which specified setting will be applied to
 * @option is the code of the option as specified by enum isds_option.
 *     It determines the type of last argument.
 *     See the definition of enum isds_option for more information.
 * @... is value of new setting. Type is determined by @option
 */
isds_error isds_set_opt(struct isds_ctx *context, int option, ...);

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
 * @mk selects mobile key authentication method to use, specifies the
 * communication code.
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
        struct isds_otp *otp);

/* Connect and log into ISDS server using the MEP login method.
 * All arguments are copied, you don't have to keep them after successful
 * return.
 * @url is base address of ISDS web service. Pass extern isds_mep_locator to use
 * the production ISDS environment (pass extern isds_mep_testing_locator to
 * access the testing environment). Passing null causes the production
 * environment locator to be used.
 * @username is the username of ISDS user or box ID
 * @code is the communication code. The code is generated when enabling
 * the mobile key authentication and can be found in the web-based portal
 * of the data-box service.
 * @return:
 *  IE_SUCCESS if authentication succeeds
 *  IE_NOT_LOGGED_IN if authentication fails
 *  IE_PARTIAL_SUCCESS if MEP authentication has been requested, fine-grade
 *  resolution is returned via @mep->resolution, keep arguments unchanged and
 *  repeat the function call as long as IE_PARTIAL_SUCCESS is being returned;
 *  or other appropriate error. */
isds_error isds_login_mep(struct isds_ctx *context, const char *url,
        const char *username, const char *code, struct isds_mep *mep);

/* Log out from ISDS server and close connection. */
isds_error isds_logout(struct isds_ctx *context);

/* Verify connection to ISDS is alive and server is responding.
 * Send dummy request to ISDS and expect dummy response. */
isds_error isds_ping(struct isds_ctx *context);

/* Get data about logged in user and his box.
 * @context is session context
 * @db_owner_info is reallocated box owner description. It will be freed on
 * error.
 * @return error code from lower layer, context message will be set up
 * appropriately. */
isds_error isds_GetOwnerInfoFromLogin(struct isds_ctx *context,
        struct isds_DbOwnerInfo **db_owner_info);

/* Get data about logged in user and his box version 2.
 * @context is session context
 * @db_owner_info is reallocated box owner description. It will be freed on
 * error.
 * @return error code from lower layer, context message will be set up
 * appropriately. */
isds_error isds_GetOwnerInfoFromLogin2(struct isds_ctx *context,
        struct isds_DbOwnerInfoExt2 **db_owner_info);

/* Get data about logged in user. */
isds_error isds_GetUserInfoFromLogin(struct isds_ctx *context,
        struct isds_DbUserInfo **db_user_info);

/* Get data about the logged-in user version 2.
 * @context is session context
 * @db_user_info is reallocated user description. It will be freed on
 * error.
 * @return error code from lower layer, context message will be set up
 * appropriately. */
isds_error isds_GetUserInfoFromLogin2(struct isds_ctx *context,
        struct isds_DbUserInfoExt2 **db_user_info);

/* Get expiration time of current password
 * @context is session context
 * @expiration is automatically reallocated time when password expires. If
 * password expiration is disabled, NULL will be returned. In case of error
 * it will be set to NULL too. */
isds_error isds_get_password_expiration(struct isds_ctx *context,
        struct isds_timeval **expiration);

/* Change user password in ISDS.
 * User must supply old password, new password will takes effect after some
 * time, current session can continue. Password must fulfil some constraints.
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
        struct isds_otp *otp, char **refnumber);

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
        const struct isds_approval *approval, char **refnumber);

/* Undocumented function.
 * @context is session context
 * @box is box description to delete
 * @approval is optional external approval of box manipulation
 * @refnumber is reallocated serial number of request assigned by ISDS. Use
 * NULL, if you don't care.*/
isds_error isds_delete_box_promptly(struct isds_ctx *context,
        const struct isds_DbOwnerInfo *box,
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

/* Update data about given box version 2.
 * @context is session context
 * @box_id is box ID
 * @new_box are updated data about the box
 * @approval is optional external approval of box manipulation
 * @refnumber is reallocated serial number of request assigned by ISDS. Use
 * NULL, if you don't care. */
isds_error isds_UpdateDataBoxDescr2(struct isds_ctx *context,
        const char *box_id, const struct isds_DbOwnerInfoExt2 *new_box,
        const struct isds_approval *approval, char **refnumber);

/* Get data about all users assigned to given box.
 * @context is session context
 * @box_id is box ID
 * @users is automatically reallocated list of struct isds_DbUserInfo */
isds_error isds_GetDataBoxUsers(struct isds_ctx *context, const char *box_id,
        struct isds_list **users);

/* Get data about all users assigned to given box version 2.
 * @context is session context
 * @box_id is box ID
 * @users is automatically reallocated list of struct isds_DbUserInfoExt2 */
isds_error isds_GetDataBoxUsers2(struct isds_ctx *context, const char *box_id,
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

/* Update data about user assigned to given box version 2.
 * @context is session context
 * @box_id is box ID
 * @isds_id is isds ID as used in isds_DbUserInfoExt2.isdsID
 * @new_user are updated data about @old_user
 * @refnumber is reallocated serial number of request assigned by ISDS. Use
 * NULL, if you don't care. */
isds_error isds_UpdateDataBoxUser2(struct isds_ctx *context,
        const char *box_id, const char *isds_id,
        const struct isds_DbUserInfoExt2 *new_user, char **refnumber);

/* Undocumented function.
 * @context is session context
 * @box_id is UTF-8 encoded box identifier
 * @token is UTF-8 encoded temporary password
 * @user_id outputs UTF-8 encoded reallocated user identifier
 * @password outputs UTF-8 encoded reallocated user password
 * Output arguments will be set to NULL in case of error */
isds_error isds_activate(struct isds_ctx *context,
        const char *box_id, const char *token,
        char **user_id, char **password);

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
        char **refnumber);

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
 * assigned up on this call.
 * @approval is optional external approval of box manipulation
 * @refnumber is reallocated serial number of request assigned by ISDS. Use
 * NULL, if you don't care.
 *
 * Always check the message from the status after calling this functions. Even
 * after a successful return. The message may contain login information,
 * especially when the user account has been created inside the testing
 * environment, or other useful data. */
isds_error isds_add_user(struct isds_ctx *context,
        const struct isds_DbOwnerInfo *box, const struct isds_DbUserInfo *user,
        struct isds_credentials_delivery *credentials_delivery,
        const struct isds_approval *approval, char **refnumber);

/* Assign new user to given box version 2.
 * @context is session context
 * @box_id is box ID
 * @user defines new user to add
 * @credentials_delivery is NULL if new user's password should be delivered
 * off-line to the user. It is valid pointer if user should obtain new
 * password on-line on dedicated web server. Then input
 * @credentials_delivery.email value is user's e-mail address user must
 * provide to dedicated web server together with @credentials_delivery.token.
 * The output reallocated token user needs to use to authorize on the web
 * server to view his new password. Output reallocated
 * @credentials_delivery.new_user_name is user's log-in name that ISDS
 * assigned up on this call.
 * @approval is optional external approval of box manipulation
 * @refnumber is reallocated serial number of request assigned by ISDS. Use
 * NULL, if you don't care.
 *
 * Always check the message from the status after calling this functions. Even
 * after a successful return. The message may contain login information,
 * especially when the user account has been created inside the testing
 * environment, or other useful data. */
isds_error isds_AddDataBoxUser2(struct isds_ctx *context, const char *box_id,
        const struct isds_DbUserInfoExt2 *user,
        struct isds_credentials_delivery *credentials_delivery,
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

/* Remove user assigned to given box version 2.
 * @context is session context
 * @box_id is box ID
 * @isds_id is isds ID as used in isds_DbUserInfoExt2.isdsID
 * @approval is optional external approval of box manipulation
 * @refnumber is reallocated serial number of request assigned by ISDS. Use
 * NULL, if you don't care. */
isds_error isds_DeleteDataBoxUser2(struct isds_ctx *context,
        const char *box_id, const char *isds_id,
        const struct isds_approval *approval, char **refnumber);

/* Get list of boxes in ZIP archive.
 * @context is session context
 * @list_identifier is UTF-8 encoded string identifying boxes of interest.
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
        const char *list_identifier, void **buffer, size_t *buffer_length);

/* Find boxes suiting given criteria.
 * @context is ISDS session context.
 * @criteria is filter. You should fill in at least some members.
 * @boxes is automatically reallocated list of isds_DbOwnerInfo structures,
 * possibly empty. Input NULL or valid old structure.
 * @return:
 *  IE_SUCCESS if search succeeded, @boxes contains useful data
 *  IE_NONEXIST if no such box exists, @boxes will be NULL
 *  IE_TOO_BIG if too much boxes exist and server truncated the results, @boxes
 *      contains still valid data
 *  other code if something bad happens. @boxes will be NULL. */
isds_error isds_FindDataBox(struct isds_ctx *context,
        const struct isds_DbOwnerInfo *criteria,
        struct isds_list **boxes);

/* Find boxes suiting given criteria version 2.
 * @context is ISDS session context.
 * @criteria is filter. You should fill in at least some members.
 * @boxes is automatically reallocated list of isds_DbOwnerInfoExt2 structures,
 * possibly empty. Input NULL or valid old structure.
 * @return:
 *  IE_SUCCESS if search succeeded, @boxes contains useful data
 *  IE_NONEXIST if no such box exists, @boxes will be NULL
 *  IE_TOO_BIG if too much boxes exist and server truncated the results, @boxes
 *      contains still valid data
 *  other code if something bad happens. @boxes will be NULL. */
isds_error isds_FindDataBox2(struct isds_ctx *context,
        const struct isds_DbOwnerInfoExt2 *criteria,
        struct isds_list **boxes);

/* Find boxes matching a given full-text criteria.
 * @context is a session context
 * @query is a non-empty string which consists of words to search
 * @target selects box attributes to search for @query words. Pass NULL if you
 * don't care.
 * @box_type restricts searching to given box type. Value DBTYPE_SYSTEM means
 * to search in all box types. Value DBTYPE_OVM_MAIN means to search in
 * non-subsidiary OVM box types. Pass NULL to let server to use default value
 * which is DBTYPE_SYSTEM.
 * @page_size defines count of boxes to constitute a response page. It counts
 * from zero. Pass NULL to let server to use a default value (50 now).
 * @page_number defines ordinary number of the response page to return. It
 * counts from zero. Pass NULL to let server to use a default value (0 now).
 * @track_matches points to true for marking @query words found in the box
 * attributes. It points to false for not marking. Pass NULL to let the server
 * to use default value (false now).
 * @total_matching_boxes outputs reallocated number of all boxes matching the
 * query. Will be pointer to NULL if server did not provide the value.
 * Pass NULL if you don't care.
 * @current_page_beginning outputs reallocated ordinary number of the first box
 * in this @boxes page. It counts from zero. It will be pointer to NULL if the
 * server did not provide the value. Pass NULL if you don't care.
 * @current_page_size outputs reallocated count of boxes in the this @boxes
 * page. It will be pointer to NULL if the server did not provide the value.
 * Pass NULL if you don't care.
 * @last_page outputs pointer to reallocated boolean. True if this @boxes page
 * is the last one, false if more boxes match, NULL if the server did not
 * provide the value. Pass NULL if you don't care.
 * @boxes outputs reallocated list of isds_fulltext_result structures,
 * possibly empty.
 * @return:
 *  IE_SUCCESS if search succeeded
 *  IE_TOO_BIG if @page_size is too large
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
        struct isds_list **boxes);

/* Get status of a box.
 * @context is ISDS session context.
 * @box_id is UTF-8 encoded box identifier as zero terminated string
 * @box_status is return value of box status.
 * @return:
 *  IE_SUCCESS if box has been found and its status retrieved
 *  IE_NONEXIST if box is not known to ISDS server
 *  or other appropriate error.
 *  You can use isds_DbState to enumerate box status. However out of enum
 *  range value can be returned too. This is feature because ISDS
 *  specification leaves the set of values open.
 *  Be ware that status DBSTATE_REMOVED is signalled as IE_SUCCESS. That means
 *  the box has been deleted, but ISDS still lists its former existence. */
isds_error isds_CheckDataBox(struct isds_ctx *context, const char *box_id,
        long int *box_status);

/* Get history of box state changes.
 * @context is ISDS session context.
 * @box_id is UTF-8 encoded sender box identifier as zero terminated string.
 * @from_time is first second of history to return in @history. Server ignores
 * subseconds. NULL means time of creating the box.
 * @to_time is last second of history to return in @history. Server ignores
 * subseconds. It's valid to have the @from_time equal to the @to_time. The
 * interval is closed from both ends. NULL means now.
 * @history outputs auto-reallocated list of pointers to struct
 * isds_box_state_period. Each item describes a continues time when the box
 * was in one state. The state is 1 for accessible box. Otherwise the box
 * is inaccessible (privileged users will get exact box state as enumerated
 * in isds_DbState, other users 0).
 * @return:
 *  IE_SUCCESS if the history has been obtained correctly,
 *  or other appropriate error. Please note that server allows to retrieve
 *  the history only to some users. */
isds_error isds_get_box_state_history(struct isds_ctx *context,
        const char *box_id,
        const struct isds_timeval *from_time, const struct isds_timeval *to_time,
        struct isds_list **history);

/* Get list of permissions to send commercial messages (ISDS operation PDZInfo).
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
        const char *box_id, struct isds_list **permissions);

/* Checks whether there can a commercial message be sent to the recipient
 * (ISDS operation PDZSendInfo).
 * @context is ISDS session context.
 * @box_id is UTF-8 encoded recipient box identifier as zero terminated string.
 * @type is a commercial message type value.
 * @can_send is return value of the operation.
 * @return:
 *  IE_SUCCESS if the result has been obtained correctly,
 *  or other appropriate error.
 */
isds_error isds_PDZSendInfo(struct isds_ctx *context, const char *box_id,
    enum isds_commercial_message_type type, _Bool *can_send);

/* Get details about credit for sending pre-paid commercial messages (ISDS operation DataBoxCreditInfo).
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
        long int *credit, char **email, struct isds_list **history);

/*
 * Get details about the data vault/long term storage (ISDS operation DTInfo).
 * @context is ISDS session context.
 * @box_id is UTF-8 encoded sender box identifier as zero terminated string.
 * @dt_info_response is an automatically reallocated structure containing
 *     information about the long term storage.
 * @return:
 *     IE_SUCCESS if the long term storage information has been obtained correctly,
 *     other error code else.
 */
isds_error isds_DTInfo(struct isds_ctx *context, const char *box_id,
    struct isds_DTInfoOutput **dt_info_response);

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
 * least one document of FILEMETATYPE_MAIN. List of ExtFiles must be empty.
 * This is read-write structure, some
 * members will be filled with valid data from ISDS. Exact list of write
 * members is subject to change. Currently dmID is changed.
 * @return ISDS_SUCCESS, or other error code if something goes wrong. */
isds_error isds_send_message(struct isds_ctx *context,
        struct isds_message *outgoing_message);

/* Send a message via ISDS to a multiple recipients
 * @context is session context
 * @outgoing_message is message to send; Some members are mandatory,
 * some are optional and some are irrelevant (especially data
 * about sender). Data about recipient will be substituted by ISDS from
 * @copies. Included pointer to isds_list documents must
 * contain at least one document of FILEMETATYPE_MAIN. List of ExtFiles must be empty.
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

/*
 * Send an attachment (file) into the ISDS attachment storage.
 * @context is session context
 * @dm_file attachment description, @dmFile->dmFileMetaType value is ignored here.
 * @dm_att automatically reallocated attachment description which can be used
 * to create a high-volume data message.
 * @return ISDS_SUCCESS, or other error codes if something goes wrong.
 */
enum isds_error isds_UploadAttachment(struct isds_ctx *context,
    const struct isds_dmFile *dm_file, struct isds_dmAtt **dm_att);

/*
 * Send an attachment (file) into the ISDS attachment storage. This
 * implementation uses the MOTOM/XOP.
 * @context is session context
 * @dm_file attachment description, @dmFile->dmFileMetaType value is ignored here.
 * @dm_att automatically reallocated attachment description which can be used
 * to create a high-volume data message.
 * @return ISDS_SUCCESS, or other error codes if something goes wrong.
 */
enum isds_error isds_UploadAttachment_mtomxop(struct isds_ctx *context,
    const struct isds_dmFile *dm_file, struct isds_dmAtt **dm_att);

/*
 * Send a high-volume data message to a recipient.
 * @context is session context
 * @outgoing_message is message to send; Some members are mandatory (like
 * dbIDRecipient), some are optional and some are irrelevant (especially data
 * about sender). Included pointer to isds_list of isds_document or isds_dmExtFile
 * entries or must contain one document of FILEMETATYPE_MAIN. This is read-write
 * structure, some members will be filled with valid data from ISDS. Exact
 * list of write members is subject to change. Currently dmID is changed.
 * @return ISDS_SUCCESS, or other error code if something goes wrong.
 */
enum isds_error isds_CreateBigMessage(struct isds_ctx *context,
    struct isds_message *outgoing_message);

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
 * first. Also in case of error the list will be set to NULL.
 * @return IE_SUCCESS or appropriate error code. */
isds_error isds_get_list_of_sent_messages(struct isds_ctx *context,
        const struct isds_timeval *from_time, const struct isds_timeval *to_time,
        const long int *dmSenderOrgUnitNum, const unsigned int status_filter,
        const unsigned long int offset, unsigned long int *number,
        struct isds_list **messages);

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
 * first. Also in case of error the list will be set to NULL.
 * @return IE_SUCCESS or appropriate error code. */
isds_error isds_get_list_of_received_messages(struct isds_ctx *context,
        const struct isds_timeval *from_time, const struct isds_timeval *to_time,
        const long int *dmRecipientOrgUnitNum,
        const unsigned int status_filter,
        const unsigned long int offset, unsigned long int *number,
        struct isds_list **messages);

/* Get list of sent message state changes.
 * Any criterion argument can be NULL, if you don't care about it.
 * @context is session context. Must not be NULL.
 * @from_time is minimal time and date of status changes inclusive
 * @to_time is maximal time and date of status changes inclusive
 * @changed_states is automatically reallocated list of
 * isds_message_status_change entries. If you provide &NULL, list will
 * be allocated on heap, if you provide pointer to non-NULL, list will be freed
 * automatically at first. Also in case of error the list will be set to NULL.
 * XXX: The list item ordering is not specified.
 * XXX: Server provides only `recent' changes.
 * @return IE_SUCCESS or appropriate error code. */
isds_error isds_get_list_of_sent_message_state_changes(
        struct isds_ctx *context,
        const struct isds_timeval *from_time, const struct isds_timeval *to_time,
        struct isds_list **changed_states);

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
        char **raw_sender_type, char **sender_name);

/*
 * Get information about the user who sent a message identified by ID.
 * @context is session context
 * @message_id is message identifier
 * @author is automatically reallocated author information retrieved from ISDS.
 */
isds_error isds_GetMessageAuthor2(struct isds_ctx *context,
    const char *message_id, struct isds_dmMessageAuthor **author);

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
 * than simple one (potentially old weak) hash comparison.
 * @context is session context
 * @message is memory with raw CMS signed message bit stream
 * @length is @message size in bytes
 * @return
 *  IE_SUCCESS  if message originates in ISDS
 *  IE_NOTEQUAL if message is unknown to ISDS
 *  other code  for other errors */
isds_error isds_authenticate_message(struct isds_ctx *context,
        const void *message, size_t length);

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
        void **output_data, size_t *output_length, struct tm **valid_to);

/* Erase message specified by @message_id from long term storage. Other
 * message cannot be erased on user request.
 * @context is session context
 * @message_id is message identifier.
 * @incoming is true for incoming message, false for outgoing message.
 * @return
 *  IE_SUCCESS  if message has been removed
 *  IE_INVAL    if message does not exist in long term storage or message
 *              belongs to different box
 * TODO: IE_NOEPRM  if user has no permission to erase a message */
isds_error isds_delete_message_from_storage(struct isds_ctx *context,
        const char *message_id, _Bool incoming);

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
 * ISDS servers pass invalid MIME types (e.g. "pdf"). This function tries to
 * guess regular MIME type (e.g. "application/pdf").
 * @mime_type is UTF-8 encoded MIME type to fix
 * @return original @mime_type if no better interpretation exists, or
 * constant static UTF-8 encoded string with proper MIME type. */
const char *isds_normalize_mime_type(const char *mime_type);

/* Deallocate structure isds_status and NULL it.
 * @status  status to be freed */
void isds_status_free(struct isds_status **status);

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

/* Deallocate structure isds_PersonName recursively and NULL it */
void isds_PersonName_free(struct isds_PersonName **person_name);

/* Deallocate structure isds_PersonName2 recursively and NULL it */
void isds_PersonName2_free(struct isds_PersonName2 **person_name);

/* Deallocate structure isds_BirthInfo recursively and NULL it */
void isds_BirthInfo_free(struct isds_BirthInfo **birth_info);

/* Deallocate structure isds_Address recursively and NULL it */
void isds_Address_free(struct isds_Address **address);

/* Deallocate structure isds_AddressExt2 recursively and NULL it */
void isds_AddressExt2_free(struct isds_AddressExt2 **address);

/* Deallocate structure isds_DbOwnerInfo recursively and NULL it */
void isds_DbOwnerInfo_free(struct isds_DbOwnerInfo **db_owner_info);

/* Deallocate structure isds_DbOwnerInfoExt2 recursively and NULL it */
void isds_DbOwnerInfoExt2_free(struct isds_DbOwnerInfoExt2 **db_owner_info);

/* Deallocate structure isds_DbUserInfo recursively and NULL it */
void isds_DbUserInfo_free(struct isds_DbUserInfo **db_user_info);

/* Deallocate structure isds_DbUserInfoExt2 recursively and NULL it */
void isds_DbUserInfoExt2_free(struct isds_DbUserInfoExt2 **db_user_info);

/* Deallocate struct isds_event recursively and NULL it */
void isds_event_free(struct isds_event **event);

/* Deallocate struct isds_envelope recursively and NULL it */
void isds_envelope_free(struct isds_envelope **envelope);

/* Deallocate struct isds_document recursively and NULL it */
void isds_document_free(struct isds_document **document);

/* Deallocate struct isds_dmAtt recursively and NULL it */
void isds_dmAtt_free(struct isds_dmAtt **att);

/* Deallocate struct isds_dmExtFile recursively and NULL it */
void isds_dmExtFile_free(struct isds_dmExtFile **ext_file);

/* Deallocate struct isds_message recursively and NULL it */
void isds_message_free(struct isds_message **message);

/* Deallocate struct isds_message_copy recursively and NULL it */
void isds_message_copy_free(struct isds_message_copy **copy);

/* Deallocate struct isds_message_status_change recursively and NULL it */
void isds_message_status_change_free(
        struct isds_message_status_change **message_status_change);

/* Deallocate struct isds_approval recursively and NULL it */
void isds_approval_free(struct isds_approval **approval);

/* Deallocate struct isds_commercial_permission recursively and NULL it */
void isds_commercial_permission_free(
        struct isds_commercial_permission **permission);

/* Deallocate struct isds_DTInfoOutput recursively and set it to NULL. */
void isds_DTInfoOutput_free(struct isds_DTInfoOutput **info);

/* Deallocate struct isds_credit_event recursively and NULL it */
void isds_credit_event_free(struct isds_credit_event **event);

/* Deallocate struct isds_credentials_delivery recursively and NULL it.
 * The email string is deallocated too. */
void isds_credentials_delivery_free(
        struct isds_credentials_delivery **credentials_delivery);

/* Deallocate struct isds_fulltext_result recursively and NULL it */
void isds_fulltext_result_free(
        struct isds_fulltext_result **result);

/* Deallocate struct isds_box_state_period recursively and NULL it */
void isds_box_state_period_free(struct isds_box_state_period **period);

/* Deallocate struct isds_dmMessageAuthor recursively and NULL it. */
void isds_dmMessageAuthor_free(struct isds_dmMessageAuthor **author);

/* Copy structure isds_status recursively */
struct isds_status *isds_status_duplicate(const struct isds_status *src);

/* Copy structure isds_PersonName recursively */
struct isds_PersonName *isds_PersonName_duplicate(
        const struct isds_PersonName *src);

/* Copy structure isds_PersonName2 recursively */
struct isds_PersonName2 *isds_PersonName2_duplicate(
        const struct isds_PersonName2 *src);

/* Copy structure isds_Address recursively */
struct isds_Address *isds_Address_duplicate(
        const struct isds_Address *src);

/* Copy structure isds_AddressExt2 recursively */
struct isds_AddressExt2 *isds_AddressExt2_duplicate(
        const struct isds_AddressExt2 *src);

/* Copy structure isds_DbOwnerInfo recursively */
struct isds_DbOwnerInfo *isds_DbOwnerInfo_duplicate(
        const struct isds_DbOwnerInfo *src);

/* Copy structure isds_DbOwnerInfoExt2 recursively */
struct isds_DbOwnerInfoExt2 *isds_DbOwnerInfoExt2_duplicate(
        const struct isds_DbOwnerInfoExt2 *src);

/* Copy structure isds_DbUserInfo recursively */
struct isds_DbUserInfo *isds_DbUserInfo_duplicate(
        const struct isds_DbUserInfo *src);

/* Copy structure isds_DbUserInfoExt2 recursively */
struct isds_DbUserInfoExt2 *isds_DbUserInfoExt2_duplicate(
        const struct isds_DbUserInfoExt2 *src);

/* Copy structure isds_box_state_period recursively */
struct isds_box_state_period *isds_box_state_period_duplicate(
        const struct isds_box_state_period *src);

#ifdef __cplusplus  /* For C++ linker sake */
}
#endif

#endif
