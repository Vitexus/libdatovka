#ifndef __ISDS_SERVICES_H
#define __ISDS_SERVICES_H

#include "server_types.h"

typedef enum {
    SERVICE_END,
    SERVICE_DS_Dz_DummyOperation,
    SERVICE_DS_DsManage_ChangeISDSPassword,
    SERVICE_asws_changePassword_ChangePasswordOTP,
    SERVICE_asws_changePassword_SendSMSCode
} service_id;

struct service_configuration {
    service_id name;        /* Identifier of SOAP service */
    const void *arguments;  /* Configuration for the service */
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
