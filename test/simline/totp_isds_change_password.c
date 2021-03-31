#ifndef _POSIX_SOURCE
#define _POSIX_SOURCE   /* For getaddrinfo(3) */
#endif

#ifndef _BSD_SOURCE
#define _BSD_SOURCE   /* For NI_MAXHOST up to glibc-2.19 */
#endif
#ifndef _DEFAULT_SOURCE
#define _DEFAULT_SOURCE   /* For NI_MAXHOST since glibc-2.20 */
#endif

#ifndef _XOPEN_SOURCE
#define _XOPEN_SOURCE 600   /* For unsetenv(3) */
#endif

#include "../test.h"
#include "server.h"
#include "libdatovka/isds.h"

static const char *username = "Doug1as$";
static const char *password = "42aA#bc8";
static const char *otp_code = "314";


static int test_login(const isds_error error,
        const isds_otp_resolution resolution, struct isds_ctx *context,
        const char *url, const char *username, const char *password,
        const struct isds_pki_credentials *pki_credentials,
        struct isds_otp *otp) {
    isds_error err;

    err = isds_login(context, url, username, password, pki_credentials, otp);
    if (error != err)
        FAIL_TEST("Wrong return code: expected=%s, returned=%s (%s)",
                isds_strerror(error), isds_strerror(err),
                isds_long_message(context));
    if (otp != NULL && resolution != otp->resolution)
        FAIL_TEST("Wrong OTP resolution: expected=%d, returned=%d (%s)",
                resolution, otp->resolution, isds_long_message(context));


    PASS_TEST;
}


static int test_isds_change_password(const isds_error error,
        const isds_otp_resolution resolution, const char *reference_number,
        struct isds_ctx *context, const char *old_password,
        const char *new_password, struct isds_otp *otp, char **refnum) {
    isds_error err;

    err = isds_change_password(context, old_password, new_password, otp,
            refnum);
    if (error != err)
        FAIL_TEST("Wrong return code: expected=%s, returned=%s (%s)",
                isds_strerror(error), isds_strerror(err),
                isds_long_message(context));
    if (otp != NULL && resolution != otp->resolution)
        FAIL_TEST("Wrong OTP resolution: expected=%d, returned=%d (%s)",
                resolution, otp->resolution, isds_long_message(context));
    if (NULL != refnum)
        TEST_STRING_DUPLICITY(reference_number, *refnum);

    PASS_TEST;
}

int main(void) {
    int error;
    pid_t server_process;
    struct isds_ctx *context = NULL;
    char *url = NULL;
    char *refnum = NULL;
    struct isds_otp otp_credentials = {
        .method = OTP_TIME
    };
    const struct arguments_asws_changePassword_ChangePasswordOTP
        passwd_arguments = {
        .username = username,
        .current_password = password,
        .method = AUTH_OTP_TIME,
        .reference_number = "42"
    };
    struct arguments_asws_changePassword_SendSMSCode
        sendsmscode_arguments = {
        .status_code = "0000",
        .status_message = "One-time password sent via SMS gateway",
        .reference_number = "43"
    };
    const struct service_configuration services[] = {
        { SERVICE_DS_Dz_DummyOperation, NULL },
        { SERVICE_asws_changePassword_ChangePasswordOTP, &passwd_arguments },
        { SERVICE_asws_changePassword_SendSMSCode, &sendsmscode_arguments },
        { SERVICE_END, NULL }
    };
    const struct arguments_otp_authentication server_arguments = {
        .method = AUTH_OTP_TIME,
        .username = username,
        .password = password,
        .otp = (char *) otp_code,
        .isds_deviations = 1,
        .services = services
    };


    INIT_TEST("isds_change_password with TOTP");

    if (unsetenv("http_proxy")) {
        ABORT_UNIT("Could not remove http_proxy variable from environment\n");
    }
    if (isds_init()) {
        isds_cleanup();
        ABORT_UNIT("isds_init() failed\n");
    }
    context = isds_ctx_create();
    if (!context) {
        isds_cleanup();
        ABORT_UNIT("isds_ctx_create() failed\n");
    }

    {
        sendsmscode_arguments.status_code = "0000";
        sendsmscode_arguments.status_message =
            "One-time password sent via SMS gateway";
        error = start_server(&server_process, &url,
                server_otp_authentication, &server_arguments, NULL);
        if (error == -1) {
            isds_ctx_free(&context);
            isds_cleanup();
            ABORT_UNIT(server_error);
        }

        otp_credentials.otp_code = (char *) otp_code;
        TEST("login", test_login, IE_SUCCESS, OTP_RESOLUTION_SUCCESS,
                context, url, username, password, NULL, &otp_credentials);

        /* First phase of authentication */
        otp_credentials.otp_code = NULL;
        TEST("First phase with invalid password", test_isds_change_password,
                IE_NOT_LOGGED_IN, OTP_RESOLUTION_BAD_AUTHENTICATION, NULL,
                context, "nbuusr1", "h2k$Aana", &otp_credentials, &refnum);
        otp_credentials.otp_code = NULL;
        TEST("First phase with valid password", test_isds_change_password,
                IE_PARTIAL_SUCCESS, OTP_RESOLUTION_TOTP_SENT, "43",
                context, password, "h2k$Aana", &otp_credentials, &refnum);

        /* Second phase of authentication */
        otp_credentials.otp_code = (char *) otp_code;
        TEST("Second phase with invalid password", test_isds_change_password,
                IE_NOT_LOGGED_IN, OTP_RESOLUTION_BAD_AUTHENTICATION, NULL,
                context, "nbuusr1", "h2k$Aana", &otp_credentials, &refnum);
        /* XXX: There is bug in curl < 7.28.0 when authorization header is not
         * sent on second attempt after 401 response. Fixed by upstream commit
         * ce8311c7e49eca93c136b58efa6763853541ec97. The only work-around is
         * to use new CURL handle. */
        TEST("Second phase with invalid password 2", test_isds_change_password,
                IE_NOT_LOGGED_IN, OTP_RESOLUTION_BAD_AUTHENTICATION, NULL,
                context, "nbuusr2", "h2k$Aana", &otp_credentials, &refnum);
        otp_credentials.otp_code = "666";
        TEST("Second phase with valid password but invalid OTP code",
                test_isds_change_password,
                IE_NOT_LOGGED_IN, OTP_RESOLUTION_BAD_AUTHENTICATION, NULL,
                context, password, "h2k$Aana", &otp_credentials, &refnum);

        /* Checks for new password */
        otp_credentials.otp_code = (char *) otp_code;
        TEST("too short (7 characters)", test_isds_change_password, IE_INVAL,
                OTP_RESOLUTION_SUCCESS, "42",
                context, password, "aB34567", &otp_credentials, &refnum);
        TEST("too long (33 characters)", test_isds_change_password, IE_INVAL,
                OTP_RESOLUTION_SUCCESS, "42",
                context, password, "aB3456789112345678921234567893123",
                &otp_credentials, &refnum);
        TEST("no upper case letter", test_isds_change_password, IE_INVAL,
                OTP_RESOLUTION_SUCCESS, "42",
                context, password, "1bcdefgh", &otp_credentials, &refnum);
        TEST("no lower case letter", test_isds_change_password, IE_INVAL,
                OTP_RESOLUTION_SUCCESS, "42",
                context, password, "1BCDEFGH", &otp_credentials, &refnum);
        TEST("no digit", test_isds_change_password, IE_INVAL,
                OTP_RESOLUTION_SUCCESS, "42",
                context, password, "aBCDEFGH", &otp_credentials, &refnum);
        TEST("forbidden space", test_isds_change_password, IE_INVAL,
                OTP_RESOLUTION_SUCCESS, "42",
                context, password, " h2k$Aan", &otp_credentials, &refnum);
        TEST("reused password", test_isds_change_password, IE_INVAL,
                OTP_RESOLUTION_SUCCESS, "42",
                context, password, password, &otp_credentials, &refnum);
        TEST("password contains user ID", test_isds_change_password, IE_INVAL,
                OTP_RESOLUTION_SUCCESS, "42",
                context, password, username, &otp_credentials, &refnum);
        TEST("sequence of the same characters", test_isds_change_password,
                IE_INVAL, OTP_RESOLUTION_SUCCESS, "42",
                context, password, "h222k$Aa", &otp_credentials, &refnum);
        TEST("forbiden prefix qwert", test_isds_change_password,
                IE_INVAL, OTP_RESOLUTION_SUCCESS, "42",
                context, password, "qwert$A8", &otp_credentials, &refnum);
        TEST("forbiden prefix asdgf", test_isds_change_password,
                IE_INVAL, OTP_RESOLUTION_SUCCESS, "42",
                context, password, "asdgf$A8", &otp_credentials, &refnum);
        TEST("forbiden prefix 12345", test_isds_change_password,
                IE_INVAL, OTP_RESOLUTION_SUCCESS, "42",
                context, password, "12345$Aa", &otp_credentials, &refnum);
        TEST("valid request", test_isds_change_password, IE_SUCCESS,
                OTP_RESOLUTION_SUCCESS, "42",
                context, password, "h2k$Aana", &otp_credentials, &refnum);

        free(refnum);
        refnum = NULL;
        isds_logout(context);
        if (stop_server(server_process)) {
            isds_ctx_free(&context);
            isds_cleanup();
            ABORT_UNIT(server_error);
        }

        free(url);
        url = NULL;
    }

    {
        sendsmscode_arguments.status_code = "2301";
        sendsmscode_arguments.status_message =
            "One-time code cannot be re-send faster than once a 30 seconds";
        error = start_server(&server_process, &url,
                server_otp_authentication, &server_arguments, NULL);
        if (error == -1) {
            isds_ctx_free(&context);
            isds_cleanup();
            ABORT_UNIT(server_error);
        }

        otp_credentials.otp_code = (char *) otp_code;
        TEST("login", test_login, IE_SUCCESS, OTP_RESOLUTION_SUCCESS,
                context, url, username, password, NULL, &otp_credentials);

        /* First phase of authentication */
        otp_credentials.otp_code = NULL;
        TEST("SendSMSCode cannot send so fast", test_isds_change_password,
                IE_ISDS, OTP_RESOLUTION_TOO_FAST, "43",
                context, password, "h2k$Aana", &otp_credentials, &refnum);

        free(refnum);
        refnum = NULL;
        isds_logout(context);
        if (stop_server(server_process)) {
            isds_ctx_free(&context);
            isds_cleanup();
            ABORT_UNIT(server_error);
        }
        free(url);
        url = NULL;
    }

    {
        sendsmscode_arguments.status_code = "2302";
        sendsmscode_arguments.status_message =
            "One-time code could not been sent. Try later again.";
        error = start_server(&server_process, &url,
                server_otp_authentication, &server_arguments, NULL);
        if (error == -1) {
            isds_ctx_free(&context);
            isds_cleanup();
            ABORT_UNIT(server_error);
        }

        otp_credentials.otp_code = (char *) otp_code;
        TEST("login", test_login, IE_SUCCESS, OTP_RESOLUTION_SUCCESS,
                context, url, username, password, NULL, &otp_credentials);

        /* First phase of authentication */
        otp_credentials.otp_code = NULL;
        TEST("SendSMSCode cannot send", test_isds_change_password,
                IE_ISDS, OTP_RESOLUTION_TOTP_NOT_SENT, "43",
                context, password, "h2k$Aana", &otp_credentials, &refnum);

        free(refnum);
        refnum = NULL;
        isds_logout(context);
        if (stop_server(server_process)) {
            isds_ctx_free(&context);
            isds_cleanup();
            ABORT_UNIT(server_error);
        }
        free(url);
        url = NULL;
    }

    isds_logout(context);
    isds_ctx_free(&context);
    isds_cleanup();
    SUM_TEST();
}
