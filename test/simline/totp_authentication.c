#ifndef _POSIX_SOURCE
#define _POSIX_SOURCE   /* For getaddrinfo(3) */
#endif

#ifndef _BSD_SOURCE
#define _BSD_SOURCE   /* For NI_MAXHOST */
#endif

#include "../test.h"
#include "server.h"
#include "isds.h"

static const char *username = "douglas";
static const char *password = "42";
static const char *otp_code = "314";


static int test_login(const isds_error error, struct isds_ctx *context,
        const char *url, const char *username, const char *password,
        const struct isds_pki_credentials *pki_credentials,
        struct isds_otp *otp) {
    isds_error err;

    err = isds_login(context, url, username, password, pki_credentials, otp);
    if (error != err)
        FAIL_TEST("Wrong return code: expected=%s, returned=%s (%s)",
                isds_strerror(error), isds_strerror(err),
                isds_long_message(context));


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

int main(int argc, char **argv) {
    int error;
    pid_t server_process;
    char *server_address = NULL;
    struct isds_ctx *context = NULL;
    char *url = NULL;

    struct isds_otp otp_credentials = {
        .method = OTP_TIME
    };

    INIT_TEST("TOTP authentication");

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
        const struct arguments_otp_authentication server_arguments = {
            .method = AUTH_OTP_TIME,
            .username = username,
            .password = password,
            .otp = (char *) otp_code,
            .isds_deviations = 1
        };
        error = start_server(&server_process, &server_address,
                server_otp_authentication, &server_arguments);
        if (error == -1) {
            isds_ctx_free(&context);
            isds_cleanup();
            ABORT_UNIT(server_error);
        }
        if (-1 == test_asprintf(&url, "http://%s/", server_address)) {
            free(server_address);
            stop_server(server_process);
            isds_ctx_free(&context);
            isds_cleanup();
            ABORT_UNIT("Could not format ISDS URL");
        }
        free(server_address);

        otp_credentials.otp_code = NULL;
        TEST("First phase with invalid password", test_login,
                IE_NOT_LOGGED_IN, context,
                url, "7777777", "nbuusr1", NULL, &otp_credentials);
        isds_logout(context);

        otp_credentials.otp_code = NULL;
        TEST("First phase with valid password", test_login,
                IE_PARTIAL_SUCCESS, context,
                url, username, password, NULL, &otp_credentials);
        isds_logout(context);

        otp_credentials.otp_code = (char *) otp_code;
        TEST("Second phase with invalid password", test_login,
                IE_NOT_LOGGED_IN, context,
                url, "7777777", "nbuusr1", NULL, &otp_credentials);
        isds_logout(context);

        otp_credentials.otp_code = "666";
        TEST("Second phase with valid password but invalid OTP code", test_login,
                IE_NOT_LOGGED_IN, context,
                url, username, password, NULL, &otp_credentials);
        isds_logout(context);

        otp_credentials.otp_code = (char *) otp_code;
        TEST("Second phase with valid password and valid OTP code", test_login,
                IE_SUCCESS, context,
                url, username, password, NULL, &otp_credentials);
        TEST("Ping after succesfull OTP log-in", test_ping,
                IE_SUCCESS, context);
        TEST("Log-out after successfull log-in", test_logout,
                IE_SUCCESS, context);

        TEST("Ping after log-out after succesfull OTP log-in", test_ping,
                IE_CONNECTION_CLOSED, context);

        if (-1 == stop_server(server_process)) {
            ABORT_UNIT(server_error);
        }
    
        free(url);
        url = NULL;
    }

    {
        error = start_server(&server_process, &server_address,
                server_out_of_order, NULL);
        if (error == -1) {
            isds_ctx_free(&context);
            isds_cleanup();
            ABORT_UNIT(server_error);
        }
        if (-1 == test_asprintf(&url, "http://%s/", server_address)) {
            free(server_address);
            stop_server(server_process);
            isds_ctx_free(&context);
            isds_cleanup();
            ABORT_UNIT("Could not format ISDS URL");
        }
        free(server_address);

        otp_credentials.otp_code = "666";
        TEST("log into out-of-order server", test_login, IE_SOAP, context,
                url, username, password, NULL, &otp_credentials);
        isds_logout(context);

        if (-1 == stop_server(server_process)) {
            ABORT_UNIT(server_error);
        }
    
        free(url);
        url = NULL;
    }

    isds_ctx_free(&context);
    isds_cleanup();
    SUM_TEST();
}
