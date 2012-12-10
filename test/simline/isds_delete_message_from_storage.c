#ifndef _POSIX_SOURCE
#define _POSIX_SOURCE   /* For getaddrinfo(3) */
#endif

#ifndef _BSD_SOURCE
#define _BSD_SOURCE   /* For NI_MAXHOST */
#endif

#ifndef _XOPEN_SOURCE
#define _XOPEN_SOURCE 600   /* For unsetenv(3) */
#endif

#include "../test.h"
#include "server.h"
#include "isds.h"

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

static int test_isds_delete_message_from_storage(const isds_error error,
        struct isds_ctx *context, const char *message_id, _Bool incoming) {
    isds_error err;

    err = isds_delete_message_from_storage(context, message_id, incoming);
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

    INIT_TEST("isds_delete_message_from_storage");

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
        const struct arguments_DS_Dx_EraseMessage service_arguments = {
            .message_id = "1234567",
            .incoming = 1
        };
        const struct service_configuration services[] = {
            { SERVICE_DS_Dz_DummyOperation, NULL },
            { SERVICE_DS_Dx_EraseMessage, &service_arguments },
            { SERVICE_END, NULL }
        };
        const struct arguments_basic_authentication server_arguments = {
            .username = username,
            .password = password,
            .isds_deviations = 1,
            .services = services
        };
        error = start_server(&server_process, &server_address,
                server_basic_authentication, &server_arguments, NULL);
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

        TEST("prior logging in", test_isds_delete_message_from_storage,
                IE_CONNECTION_CLOSED, context, "1234567", 1);
        TEST("login", test_login, IE_SUCCESS,
                context, url, username, password, NULL, NULL);
        TEST("bad message ID", test_isds_delete_message_from_storage, IE_INVAL,
                context, "1000000", 1);
        TEST("bad direction", test_isds_delete_message_from_storage, IE_INVAL,
                context, "1234567", 0);
        TEST("good ID and direction", test_isds_delete_message_from_storage,
                IE_SUCCESS, context, "1234567", 1);

        if (stop_server(server_process)) {
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
