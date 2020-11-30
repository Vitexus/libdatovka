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

static const char *username = "douglas";
static const char *password = "42";
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

static int test_logout(const isds_error error, struct isds_ctx *context) {
    isds_error err;

    err = isds_logout(context);
    if (error != err)
        FAIL_TEST("Wrong return code: expected=%s, returned=%s (%s)",
                isds_strerror(error), isds_strerror(err),
                isds_long_message(context));

    PASS_TEST;
}

static int test_ping(const isds_error error, struct isds_ctx *context) {
    isds_error err;

    err = isds_ping(context);
    if (error != err)
        FAIL_TEST("Wrong return code: expected=%s, returned=%s (%s)",
                isds_strerror(error), isds_strerror(err),
                isds_long_message(context));

    PASS_TEST;
}

int main(void) {
    int error;
    pid_t server_process;
    struct isds_ctx *context = NULL;
    char *url = NULL;

    struct isds_otp otp_credentials = {
        .method = OTP_HMAC
    };

    INIT_TEST("HOTP authentication");

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
        const struct service_configuration services[] = {
            { SERVICE_DS_Dz_DummyOperation, NULL },
            { SERVICE_END, NULL }
        };
        const struct arguments_otp_authentication server_arguments = {
            .method = AUTH_OTP_HMAC,
            .username = username,
            .password = password,
            .otp = otp_code,
            .isds_deviations = 1,
            .services = services
        };
        error = start_server(&server_process, &url,
                server_otp_authentication, &server_arguments, NULL);
        if (error == -1) {
            isds_ctx_free(&context);
            isds_cleanup();
            ABORT_UNIT(server_error);
        }

        otp_credentials.otp_code = NULL;
        TEST("Invalid password and missing OTP code", test_login,
                IE_NOT_LOGGED_IN, OTP_RESOLUTION_BAD_AUTHENTICATION, context,
                url, "7777777", "nbuusr1", NULL, &otp_credentials);
        isds_logout(context);

        otp_credentials.otp_code = (char *) otp_code;
        TEST("Invalid password and valid OTP code", test_login,
                IE_NOT_LOGGED_IN, OTP_RESOLUTION_BAD_AUTHENTICATION, context,
                url, "7777777", "nbuusr1", NULL, &otp_credentials);
        isds_logout(context);

        otp_credentials.otp_code = NULL;
        TEST("Valid password but missing OTP code", test_login,
                IE_NOT_LOGGED_IN, OTP_RESOLUTION_BAD_AUTHENTICATION, context,
                url, username, password, NULL, &otp_credentials);
        isds_logout(context);

        otp_credentials.otp_code = "666";
        TEST("Valid password but invalid OTP code", test_login,
                IE_NOT_LOGGED_IN, OTP_RESOLUTION_BAD_AUTHENTICATION, context,
                url, username, password, NULL, &otp_credentials);
        isds_logout(context);

        otp_credentials.otp_code = (char *) otp_code;
        TEST("Valid password and valid OTP code", test_login,
                IE_SUCCESS, OTP_RESOLUTION_SUCCESS, context,
                url, username, password, NULL, &otp_credentials);
        TEST("Ping after succesfull OTP log-in", test_ping,
                IE_SUCCESS, context);
        TEST("Log-out after successfull log-in", test_logout,
                IE_SUCCESS, context);

        TEST("Ping after log-out after succesfull OTP log-in", test_ping,
                IE_CONNECTION_CLOSED, context);

        if (stop_server(server_process)) {
            isds_ctx_free(&context);
            isds_cleanup();
            ABORT_UNIT(server_error);
        }
    
        free(url);
        url = NULL;
    }

    {
        error = start_server(&server_process, &url,
                server_out_of_order, NULL, NULL);
        if (error == -1) {
            isds_ctx_free(&context);
            isds_cleanup();
            ABORT_UNIT(server_error);
        }

        otp_credentials.otp_code = "666";
        TEST("log into out-of-order server", test_login,
                IE_SOAP, OTP_RESOLUTION_UNKNOWN, context,
                url, username, password, NULL, &otp_credentials);
        isds_logout(context);

        if (stop_server(server_process)) {
            isds_ctx_free(&context);
            isds_cleanup();
            ABORT_UNIT(server_error);
        }
    
        free(url);
        url = NULL;
    }

    isds_ctx_free(&context);
    isds_cleanup();
    SUM_TEST();
}
