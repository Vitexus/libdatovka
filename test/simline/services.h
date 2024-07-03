#ifndef __ISDS_SERVICES_H
#define __ISDS_SERVICES_H

#include <time.h> /* struct tm */
#include "server_types.h"

typedef enum {
    SERVICE_END,
    SERVICE_asws_changePassword_ChangePasswordOTP,
    SERVICE_asws_changePassword_SendSMSCode,
    SERVICE_DS_df_DataBoxCreditInfo,
    SERVICE_DS_df_FindDataBox,
    SERVICE_DS_df_GetDataBoxActivityStatus,
    SERVICE_DS_df_ISDSSearch2,
    SERVICE_DS_DsManage_ChangeISDSPassword,
    SERVICE_DS_Dx_EraseMessage,
    SERVICE_DS_Dz_DummyOperation,
    SERVICE_DS_Dz_ResignISDSDocument,
} service_id;

struct service_configuration {
    service_id name;        /* Identifier of SOAP service */
    const void *arguments;  /* Configuration for the service */
};

/* Type of credit change event */
typedef enum {
    SERVER_CREDIT_CHARGED = 1, /* Credit has been charged. */
    SERVER_CREDIT_DISCHARGED = 2, /* Credit has been discharged. */
    SERVER_CREDIT_MESSAGE_SENT = 3, /*
                                     * Credit has been spent for sending
                                     * a commercial message.
                                     */
    SERVER_CREDIT_STORAGE_SET = 4, /*
                                    * Credit has been spent for setting
                                    * a long-term storage.
                                    */
    SERVER_CREDIT_EXPIRED = 5, /* Credit has expired. */
    SERVER_CREDIT_DELETED_MESSAGE_RECOVERED = 7 /*
                                                 * Message previously deleted
                                                 * from long-term storage
                                                 * has been recovered.
                                                 */
} server_credit_event_type;

/* Data specific for SERVER_CREDIT_CHARGED server_credit_event_type */
struct server_credit_event_charged {
    char *transaction;              /* Transaction identified;
                                       NULL-terminated string. */
};

/* Data specific for SERVER_CREDIT_DISCHARGED server_credit_event_type */
struct server_credit_event_discharged {
    char *transaction;              /* Transaction identified;
                                       NULL-terminated string. */
};

/* Data specific for SERVER_CREDIT_MESSAGE_SENT server_credit_event_type */
struct server_credit_event_message_sent {
    char *recipient;                /* Recipient's box ID of the sent message */
    char *message_id;               /* ID of the sent message */
};

/* Data specific for SERVER_CREDIT_STORAGE_SET server_credit_event_type */
struct server_credit_event_storage_set {
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

/* Data specific for SERVER_CREDIT_DELETED_MESSAGE_RECOVERED server_credit_event_type */
struct server_credit_event_deleted_message_recovered {
    char *initiator; /* Name of a user who initiated this change; Optional. */
};

/* Event about change of credit for sending commercial services */
struct server_credit_event {
    /* Common fields */
    struct isds_timeval *time;           /* When the credit was changed. */
    long int credit_change;         /* Difference in credit value caused by
                                       this event. The unit is 1/100 CZK. */
    long int new_credit;            /* Credit value after this event.
                                       The unit is 1/100 CZK. */
    server_credit_event_type type;    /* Type of the event */

    /* Details specific for the type */
    union {
        /* SERVER_CREDIT_CHARGED */
        struct server_credit_event_charged charged;
        /* SERVER_CREDIT_DISCHAGED */
        struct server_credit_event_discharged discharged;
        /* SERVER_CREDIT_MESSAGE_SENT */
        struct server_credit_event_message_sent message_sent;
        /* SERVER_CREDIT_STORAGE_SET */
        struct server_credit_event_storage_set storage_set;
        /* SERVER_CREDIT_DELETED_MESSAGE_RECOVERED */
        struct server_credit_event_deleted_message_recovered deleted_message_recovered;
    } details;
};

/* An ISDSSearch2 result */
struct server_db_result {
    char *id;                   /* dbID value */
    char *type;                 /* dbType value */
    char *name;                 /* dbName value */
    char *address;              /* dbAddress value */
    struct tm *birth_date;      /* dbBiDate value */
    char *ic;                   /* dbICO value */
    _Bool ovm;                  /* dbEffectiveOVM value */
    char *send_options;         /* dbSendOptions value */
};

/* Union of tdbOwnerInfo and tdbPersonalOwnerInfo XSD types */
struct server_owner_info {
    char *dbID;
    _Bool *aifoIsds;
    char *dbType;
    char *ic;
    char *pnFirstName;
    char *pnMiddleName;
    char *pnLastName;
    char *pnLastNameAtBirth;
    char *firmName;
    struct tm *biDate;
    char *biCity;
    char *biCounty;
    char *biState;
    long int *adCode;
    char *adCity;
    char *adDistrict;
    char *adStreet;
    char *adNumberInStreet;
    char *adNumberInMunicipality;
    char *adZipCode;
    char *adState;
    char *nationality;
    _Bool email_exists; /* Return empty email element */
    char *email;
    _Bool telNumber_exists; /* Return empty telNumber element */
    char *telNumber;
    char *identifier;
    char *registryCode;
    long int *dbState;
    _Bool *dbEffectiveOVM;
    _Bool *dbOpenAddressing;
};

/* Box state period */
struct server_box_state_period {
    struct isds_timeval *from;
    struct isds_timeval *to;
    long int dbState;
};

/* General linked list */
struct server_list {
    struct server_list *next;       /* Next list item,
                                       or NULL if current is last */
    void *data;                     /* Payload */
    void (*destructor) (void **);   /* Payload deallocator;
                                       Use NULL to have static data member. */
};

struct arguments_DS_df_DataBoxCreditInfo {
    const char *status_code;
    const char *status_message;
    const char *box_id;             /* Require this dbID in request */
    const struct tm *from_date;     /* Require this ciFromDate in request */
    const struct tm *to_date;       /* Require this ciTodate in request */
    const long int current_credit;  /* Return this currentCredit */
    const char *email;              /* Return this notifEmail */
    const struct server_list *history;  /* Return this ciRecords */
};

struct arguments_DS_df_FindDataBox {
    const char *status_code;
    const char *status_message;
    const struct server_owner_info *criteria;  /* generilized tDbOwnerInfo */
    const _Bool results_exists;     /* Return dbResults element */
    const struct server_list *results;  /* Return list of
                                           struct server_owner_info * as
                                           dbResults */
};

struct arguments_DS_df_GetDataBoxActivityStatus {
    const char *status_code;
    const char *status_message;
    const char *box_id;             /* Input */
    const struct isds_timeval *from;     /* Input */
    const struct isds_timeval *to;       /* Input */
    const char *result_box_id;    /* Output */
    const _Bool results_exists;     /* Return Periods element */
    const struct server_list *results;  /* Return list of
                                           struct server_box_state_period * as
                                           Periods */
};

struct arguments_DS_df_ISDSSearch2 {
    const char *status_code;
    const char *status_message;
    const char *search_text;        /* Require this searchText in a request */
    const char *search_type;        /* Require this searchType in a request */
    const char *search_scope;       /* Require this searchScope in a request */
    const long int *search_page_number; /* Require this page in a request */
    const long int *search_page_size;   /* Require this pageSize in a request */
    const _Bool *search_highlighting_value;     /* Require this highlighting
                                                   value in a request */
    const unsigned long int *total_count;    /* Return this totalCount */
    const unsigned long int *current_count;  /* Return this currentCount */
    const unsigned long int *position;       /* Return this position */
    const _Bool *last_page;         /* Return this lastPage */
    const _Bool results_exists;     /* Return dbResults element */
    const struct server_list *results;  /* Return list of
                                           struct server_db_result* as dbResults */
};

struct arguments_DS_Dz_ResignISDSDocument {
    const char *status_code;
    const char *status_message;
    const struct tm *valid_to;   /* Return this date if not NULL */
};

struct arguments_DS_Dx_EraseMessage {
    const char *message_id;    /* Expected message ID */
    _Bool incoming;             /* Expected message direction,
                                   true for incoming */
};

struct arguments_DS_DsManage_ChangeISDSPassword {
    const char *username;           /* User ID */
    const char *current_password;   /* User password */
};

struct arguments_asws_changePassword_ChangePasswordOTP {
    const char *username;           /* User ID */
    const char *current_password;   /* User password */
    enum auth_otp_method method;    /* OTP method */
    const char *reference_number;   /* Return this string if not NULL */
};

struct arguments_asws_changePassword_SendSMSCode {
    const char *status_code;
    const char *status_message;
    const char *reference_number;   /* Return this string if not NULL */
};

#endif
