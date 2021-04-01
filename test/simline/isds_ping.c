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

static int test_isds_ping(const isds_error error, struct isds_ctx *context) {
    isds_error err;

    err = isds_ping(context);

    if (error != err) {
        FAIL_TEST("Wrong return code: expected=%s, returned=%s (%s)",
                isds_strerror(error), isds_strerror(err),
                isds_long_message(context));
    }

    PASS_TEST;
}


int main(void) {
    int error;
    pid_t server_process;
    struct isds_ctx *context = NULL;
    char *url = NULL;

    INIT_TEST("isds_ping");

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
        const struct arguments_basic_authentication server_arguments = {
            .username = username,
            .password = password,
            .isds_deviations = 1,
            .services = services
        };

        error = start_server(&server_process, &url,
                server_basic_authentication, &server_arguments, NULL);
        if (error == -1) {
            isds_ctx_free(&context);
            isds_cleanup();
            ABORT_UNIT(server_error);
        }

        TEST("prior logging in", test_isds_ping,
                IE_CONNECTION_CLOSED, context);
        TEST("login", test_login, IE_SUCCESS,
                context, url, username, password, NULL, NULL);
        TEST("after logging in", test_isds_ping,
                IE_SUCCESS, context);

        /* Terminate server without logging out, we will exhibit calls on the
         * logged-in context later. */
        if (stop_server(server_process)) {
            isds_ctx_free(&context);
            isds_cleanup();
            ABORT_UNIT(server_error);
        }
        free(url);
        url = NULL;
    }

    /* Shorten network timeout to make tests faster */
    if (isds_set_timeout(context, 500) != IE_SUCCESS) {
        isds_ctx_free(&context);
        isds_cleanup();
        ABORT_UNIT("isds_set_timeout() failed");
    }
    TEST("when server disappears", test_isds_ping,
            IE_NETWORK, context);

    isds_logout(context);
    TEST("after logging out", test_isds_ping,
            IE_CONNECTION_CLOSED, context);

    isds_ctx_free(&context);
    isds_cleanup();
    SUM_TEST();
}
