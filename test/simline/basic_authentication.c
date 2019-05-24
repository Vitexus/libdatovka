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
#include "isds.h"

static const char *username = "douglas";
static const char *password = "42";


static int test_login(const isds_error error, struct isds_ctx *context,
        const char *url, const char *username, const char *password,
        const struct isds_pki_credentials *pki_credentials,
        struct isds_otp *otp) {
    isds_error err;

    err = isds_login(context, url, username, password, pki_credentials, otp,
            NULL);
    if (error != err)
        FAIL_TEST("Wrong return code: expected=%s, returned=%s (%s)",
                isds_strerror(error), isds_strerror(err),
                isds_long_message(context));

    isds_logout(context);
    PASS_TEST;
}

int main(void) {
    int error;
    pid_t server_process;
    struct isds_ctx *context = NULL;
    char *url = NULL;

    INIT_TEST("basic authentication");

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

        TEST("invalid credentials", test_login, IE_NOT_LOGGED_IN, context,
                url, "7777777", "nbuusr1", NULL, NULL);

        TEST("valid login", test_login, IE_SUCCESS, context,
                url, username, password, NULL, NULL);

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

        TEST("log into out-of-order server", test_login, IE_SOAP, context,
                url, username, password, NULL, NULL);

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
