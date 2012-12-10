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

static int test_isds_change_password(const isds_error error,
        struct isds_ctx *context, const char *old_password,
        const char *new_password, struct isds_otp *otp, char **refnum) {
    isds_error err;

    err = isds_change_password(context, old_password, new_password, otp,
            refnum);
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

    INIT_TEST("isds_change_password");

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
        const struct arguments_DS_DsManage_ChangeISDSPassword service_arguments = {
            .username = username,
            .current_password = password
        };
        const struct service_configuration services[] = {
            { SERVICE_DS_Dz_DummyOperation, NULL },
            { SERVICE_DS_DsManage_ChangeISDSPassword, &service_arguments },
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

        TEST("login", test_login, IE_SUCCESS,
                context, url, username, password, NULL, NULL);
        TEST("bad old password", test_isds_change_password, IE_INVAL,
                context, "bad old password", "h2k$Aana", NULL, NULL);
        TEST("too short (7 characters)", test_isds_change_password, IE_INVAL,
                context, password, "aB34567", NULL, NULL);
        TEST("too long (33 characters)", test_isds_change_password, IE_INVAL,
                context, password, "aB3456789112345678921234567893123", NULL, NULL);
        TEST("no upper case letter", test_isds_change_password, IE_INVAL,
                context, password, "1bcdefgh", NULL, NULL);
        TEST("no lower case letter", test_isds_change_password, IE_INVAL,
                context, password, "1BCDEFGH", NULL, NULL);
        TEST("no digit", test_isds_change_password, IE_INVAL,
                context, password, "aBCDEFGH", NULL, NULL);
        TEST("forbidden space", test_isds_change_password, IE_INVAL,
                context, password, " h2k$Aan", NULL, NULL);
        TEST("reused password", test_isds_change_password, IE_INVAL,
                context, password, password, NULL, NULL);
        TEST("password contains user ID", test_isds_change_password, IE_INVAL,
                context, password, username, NULL, NULL);
        TEST("sequence of the same characters", test_isds_change_password,
                IE_INVAL, context, password, "h222k$Aa", NULL, NULL);
        TEST("forbiden prefix qwert", test_isds_change_password,
                IE_INVAL, context, password, "qwert$A8", NULL, NULL);
        TEST("forbiden prefix asdgf", test_isds_change_password,
                IE_INVAL, context, password, "asdgf$A8", NULL, NULL);
        TEST("forbiden prefix 12345", test_isds_change_password,
                IE_INVAL, context, password, "12345$Aa", NULL, NULL);
        TEST("valid request", test_isds_change_password, IE_SUCCESS,
                context, password, "h2k$Aana", NULL, NULL);

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
