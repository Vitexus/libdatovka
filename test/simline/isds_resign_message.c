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

static int test_isds_resign_message(const isds_error error,
        struct isds_ctx *context, const void *data, size_t length,
        _Bool request_valid_to, const struct tm *expected_valid_to) {
    isds_error err;
    void *output = NULL;
    size_t output_length = 0;
    struct tm *valid_to = NULL;

    err = isds_resign_message(context, data, length, &output, &output_length,
            (request_valid_to) ? &valid_to : NULL);
    if (error != err) {
        free(output);
        free(valid_to);
        FAIL_TEST("Wrong return code: expected=%s, returned=%s (%s)",
                isds_strerror(error), isds_strerror(err),
                isds_long_message(context));
    }
    if (!err) {
        if (NULL == output) {
            free(output);
            free(valid_to);
            FAIL_TEST("No message returned");
        }
        if (length != output_length) {
            free(output);
            free(valid_to);
            FAIL_TEST("Wrong size of returned message: "
                    "expected=%zd, returned=%zd", length, output_length);
        }
        if (strcmp(data, output)) {
            free(output);
            free(valid_to);
            FAIL_TEST("Returned message does not match input message: "
                    "expected=`%s', returned=`%s'", data, output);
        }
    } else {
        if (0 != output_length) {
            free(output);
            free(valid_to);
            FAIL_TEST("Output size is not 0 on error");
        }
        if (NULL != output) {
            free(output);
            free(valid_to);
            FAIL_TEST("Output data pointer is not NULL on error");
        }
    }
    free(output);

    if (NULL == expected_valid_to) {
        if (NULL != valid_to) {
            free(valid_to);
            FAIL_TEST("Valid_to pointer is not NULL");
        }
    } else {
        if (NULL == valid_to) {
            free(valid_to);
            FAIL_TEST("Valid_to pointer should not be NULL");
        }
        if (
                expected_valid_to->tm_year != valid_to->tm_year ||
                expected_valid_to->tm_mon != valid_to->tm_mon ||
                expected_valid_to->tm_mday != valid_to->tm_mday
        ) {
            char expected[32];
            char returned[32];
            strftime(expected, sizeof(expected), "%F", expected_valid_to);
            strftime(returned, sizeof(returned), "%F", valid_to);
            free(valid_to);
            FAIL_TEST("Wrong valid_to date: expected=%s, returned=%s",
                    expected, returned);
        }
    }
    free(valid_to);

    PASS_TEST;
}


static void test_error(const char *code, const isds_error expected_error,
        struct isds_ctx *context) {
    struct arguments_DS_Dz_ResignISDSDocument service_arguments = {
        .status_code = code,
        .status_message = "some text",
        .valid_to = NULL
    };
    const struct service_configuration services[] = {
        { SERVICE_DS_Dz_DummyOperation, NULL },
        { SERVICE_DS_Dz_ResignISDSDocument, &service_arguments },
        { SERVICE_END, NULL }
    };
    const struct arguments_basic_authentication server_arguments = {
        .username = username,
        .password = password,
        .isds_deviations = 1,
        .services = services
    };
    const char data[] = "Hello world!";
    pid_t server_process;
    char *url = NULL;
    int error;

    error = start_server(&server_process, &url,
            server_basic_authentication, &server_arguments, NULL);
    if (error == -1) {
        isds_ctx_free(&context);
        isds_cleanup();
        ABORT_UNIT(server_error);
    }

    TEST("login", test_login, IE_SUCCESS,
            context, url, username, password, NULL, NULL);
    TEST(code, test_isds_resign_message,
            expected_error, context, data, sizeof(data), 1, NULL);

    isds_logout(context);
    if (stop_server(server_process)) {
        isds_ctx_free(&context);
        isds_cleanup();
        ABORT_UNIT(server_error);
    }
    free(url);
    url = NULL;
}

int main(void) {
    int error;
    pid_t server_process;
    struct isds_ctx *context = NULL;
    char *url = NULL;

    INIT_TEST("isds_resing_message");

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
        struct tm date;
        const struct arguments_DS_Dz_ResignISDSDocument service_arguments = {
            .status_code = "0000",
            .status_message = "some text",
            .valid_to = &date
        };
        const struct service_configuration services[] = {
            { SERVICE_DS_Dz_DummyOperation, NULL },
            { SERVICE_DS_Dz_ResignISDSDocument, &service_arguments },
            { SERVICE_END, NULL }
        };
        const struct arguments_basic_authentication server_arguments = {
            .username = username,
            .password = password,
            .isds_deviations = 1,
            .services = services
        };
        memset(&date, 0, sizeof(date));
        date.tm_year = 42;
        date.tm_mon = 2;
        date.tm_mday = 3;
        const char data[] = "Hello world!";

        error = start_server(&server_process, &url,
                server_basic_authentication, &server_arguments, NULL);
        if (error == -1) {
            isds_ctx_free(&context);
            isds_cleanup();
            ABORT_UNIT(server_error);
        }

        TEST("prior logging in", test_isds_resign_message,
                IE_CONNECTION_CLOSED, context, data, sizeof(data), 1, NULL);
        TEST("login", test_login, IE_SUCCESS,
                context, url, username, password, NULL, NULL);
        TEST("NULL message", test_isds_resign_message,
                IE_INVAL, context, NULL, 42, 1, NULL);
        TEST("empty message", test_isds_resign_message,
                IE_INVAL, context, "", 0, 1, NULL);
        TEST("valid message", test_isds_resign_message,
                IE_SUCCESS, context, data, sizeof(data), 1, &date);
        TEST("valid message without date", test_isds_resign_message,
                IE_SUCCESS, context, data, sizeof(data), 0, NULL);

        isds_logout(context);
        if (stop_server(server_process)) {
            isds_ctx_free(&context);
            isds_cleanup();
            ABORT_UNIT(server_error);
        }
        free(url);
        url = NULL;
    }

    test_error("2200", IE_INVAL, context);
    test_error("2201", IE_NOTUNIQ, context);
    test_error("2204", IE_INVAL, context);
    test_error("2207", IE_ISDS, context);

    isds_ctx_free(&context);
    isds_cleanup();
    SUM_TEST();
}
