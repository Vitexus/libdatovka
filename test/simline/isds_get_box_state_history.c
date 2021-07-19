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

/* Compare isds_box_state_period.
 * @return 0 if equal, 1 otherwise. */
static int compare_isds_box_state_period(
        const struct isds_box_state_period *expected_result,
        const struct isds_box_state_period *result) {

    TEST_POINTER_DUPLICITY(expected_result, result);
    if (NULL == expected_result)
        return 0;

    TEST_TIMEVALPTR_DUPLICITY(&expected_result->from, &result->from);
    TEST_TIMEVALPTR_DUPLICITY(&expected_result->to, &result->to);
    TEST_INT_DUPLICITY(expected_result->dbState, result->dbState);

    return 0;
}

/* Compare list of isds_box_state_period structures.
 * @return 0 if equal, 1 otherwise and set failure reason. */
static int compare_isds_box_state_period_lists(const struct isds_list *expected_list,
        const struct isds_list *list) {
    const struct isds_list *expected_item, *item;
    int i;

    for (i = 1, expected_item = expected_list, item = list;
            NULL != expected_item && NULL != item;
            i++, expected_item = expected_item->next, item = item->next) {
        if (compare_isds_box_state_period(
                    (struct isds_box_state_period *)expected_item->data,
                    (struct isds_box_state_period *)item->data))
            return 1;
    }
    if (NULL != expected_item && NULL == item)
        FAIL_TEST("Result list is missing %d. item", i);
    if (NULL == expected_item && NULL != item)
        FAIL_TEST("Result list has superfluous %d. item", i);

    return 0;
}

static int test_isds_get_box_state_history(const isds_error expected_error,
        struct isds_ctx *context,
        const char *dbID, const struct isds_timeval *from, const struct isds_timeval *to,
        const struct isds_list *expected_results) {
    isds_error error;
    struct isds_list *results;
    TEST_CALLOC(results);

    error = isds_get_box_state_history(context, dbID, from, to, &results);
    TEST_DESTRUCTOR((void(*)(void*))isds_list_free, &results);

    if (expected_error != error) {
        FAIL_TEST("Wrong return code: expected=%s, returned=%s (%s)",
                isds_strerror(expected_error), isds_strerror(error),
                isds_long_message(context));
    }

    if (IE_SUCCESS != error) {
        TEST_POINTER_IS_NULL(results);
        PASS_TEST;
    }

    if (compare_isds_box_state_period_lists(expected_results, results))
        return 1;


    PASS_TEST;
}

int main(void) {
    int error;
    pid_t server_process;
    struct isds_ctx *context = NULL;

    INIT_TEST("isds_get_box_state_history");

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
        /* Full response with two results */
        char *url = NULL;

        char *input_dbID = "A123456";
        struct isds_timeval input_from = {
            .tv_sec = 5,
            .tv_usec = 0
        };
        struct isds_timeval input_to = {
            .tv_sec = 6,
            .tv_usec = 0
        };

        struct isds_box_state_period period1 = {
            .from = {
                .tv_sec = 1,
                .tv_usec = 0
            },
            .to = {
                .tv_sec = 2,
                .tv_usec = 0
            },
            .dbState = 1
        };

        struct isds_box_state_period period2 = {
            .from = {
                .tv_sec = 3,
                .tv_usec = 0
            },
            .to = {
                .tv_sec = 4,
                .tv_usec = 0
            },
            .dbState = 2
        };
        struct isds_list results2 = {
            .next = NULL,
            .data = &period2,
            .destructor = NULL
        };
        struct isds_list results = {
            .next = &results2,
            .data = &period1,
            .destructor = NULL
        };

        struct server_box_state_period server_period1 = {
            .from = &period1.from,
            .to = &period1.to,
            .dbState = period1.dbState
        };
        struct server_box_state_period server_period2 = {
            .from = &period2.from,
            .to = &period2.to,
            .dbState = period2.dbState
        };
        struct server_list server_results2 = {
            .next = NULL,
            .data = &server_period2,
            .destructor = NULL
        };
        struct server_list server_results = {
            .next = &server_results2,
            .data = &server_period1,
            .destructor = NULL
        };

        const struct arguments_DS_df_GetDataBoxActivityStatus
            service_arguments = {
                .status_code = "0000",
                .status_message = "Ok.",
                .box_id = input_dbID,
                .from = &input_from,
                .to = &input_to,
                .result_box_id = "B123456",
                .results_exists = 0,
                .results = &server_results
        };
        const struct service_configuration services[] = {
            { SERVICE_DS_Dz_DummyOperation, NULL },
            { SERVICE_DS_df_GetDataBoxActivityStatus, &service_arguments },
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
        TEST("login", test_login, IE_SUCCESS,
                context, url, username, password, NULL, NULL);
        free(url);

        TEST("All data", test_isds_get_box_state_history, IE_SUCCESS,
                context, input_dbID, &input_from, &input_to, &results);

        isds_logout(context);
        if (stop_server(server_process)) {
            isds_ctx_free(&context);
            isds_cleanup();
            ABORT_UNIT(server_error);
        }
    }

    {
        /* Some error */
        char *url = NULL;

        char *input_dbID = "A123456";
        struct isds_timeval input_from = {
            .tv_sec = 5,
            .tv_usec = 0
        };
        struct isds_timeval input_to = {
            .tv_sec = 6,
            .tv_usec = 0
        };

        const struct arguments_DS_df_GetDataBoxActivityStatus
            service_arguments = {
                .status_code = "0002",
                .status_message = "No such box",
                .box_id = input_dbID,
                .from = &input_from,
                .to = &input_to,
                .result_box_id = NULL,
                .results_exists = 0,
                .results = NULL
        };
        const struct service_configuration services[] = {
            { SERVICE_DS_Dz_DummyOperation, NULL },
            { SERVICE_DS_df_GetDataBoxActivityStatus, &service_arguments },
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
        TEST("login", test_login, IE_SUCCESS,
                context, url, username, password, NULL, NULL);
        free(url);

        TEST("An error", test_isds_get_box_state_history, IE_ISDS,
                context, input_dbID, &input_from, &input_to, NULL);

        isds_logout(context);
        if (stop_server(server_process)) {
            isds_ctx_free(&context);
            isds_cleanup();
            ABORT_UNIT(server_error);
        }
    }


    isds_ctx_free(&context);
    isds_cleanup();
    SUM_TEST();
}
