#ifndef __ISDS_SERVER_TYPES_H
#define __ISDS_SERVER_TYPES_H

/* One-time password authentication method */
enum auth_otp_method {
    AUTH_OTP_HMAC = 0,  /* HMAC-based OTP */
    AUTH_OTP_TIME       /* Time-based OTP */
};

#endif
