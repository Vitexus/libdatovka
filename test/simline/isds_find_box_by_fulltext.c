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

static int compare_match_lists(const char *label,
        const char *expected_string, const struct isds_list *expected_list,
        const char* string, const struct isds_list *list) {
    const struct isds_list *expected_item, *item;
    ptrdiff_t expected_offset, offset;
    int i;

    for (i = 1, expected_item = expected_list, item = list;
            NULL != expected_item && NULL != item;
            i++, expected_item = expected_item->next, item = item->next) {
        expected_offset = (char *)expected_item->data - expected_string;
        offset = (char *)item->data - string;
        if (expected_offset != offset)
            FAIL_TEST("%d. %s match offsets differ: expected=%td, got=%td",
                    i, label, expected_offset, offset);
    }
    if (NULL != expected_item && NULL == item)
        FAIL_TEST("%s match offsets list is missing %d. item", label, i);
    if (NULL == expected_item && NULL != item)
        FAIL_TEST("%s match offsets list has superfluous %d. item", label, i);

    return 0;
}

static int compare_fulltext_results(
        const struct isds_fulltext_result *expected_result,
        const struct isds_fulltext_result *result) {

    TEST_POINTER_DUPLICITY(expected_result, result);

    TEST_STRING_DUPLICITY(expected_result->dbID, result->dbID);
    TEST_INT_DUPLICITY(expected_result->dbType, result->dbType);
    TEST_STRING_DUPLICITY(expected_result->name, result->name);
    if (compare_match_lists("start name",
                expected_result->name, expected_result->name_match_start,
                result->name, result->name_match_start))
        return 1;
    if (compare_match_lists("end name",
                expected_result->name, expected_result->name_match_end,
                result->name, result->name_match_end))
        return 1;
    TEST_STRING_DUPLICITY(expected_result->address, result->address);
    if (compare_match_lists("start address",
                expected_result->address, expected_result->address_match_start,
                result->address, result->address_match_start))
        return 1;
    if (compare_match_lists("end address",
                expected_result->address, expected_result->address_match_end,
                result->address, result->address_match_end))
        return 1;
    TEST_STRING_DUPLICITY(expected_result->ic, result->ic);
    TEST_TMPTR_DUPLICITY(expected_result->biDate, result->biDate);
    TEST_BOOLEAN_DUPLICITY(expected_result->dbEffectiveOVM,
            result->dbEffectiveOVM);
    TEST_BOOLEAN_DUPLICITY(expected_result->active, result->active);
    TEST_BOOLEAN_DUPLICITY(expected_result->public_sending,
            result->public_sending);
    TEST_BOOLEAN_DUPLICITY(expected_result->commercial_sending,
            result->commercial_sending);
    return 0;
}

static int compare_result_lists(const struct isds_list *expected_list,
        const struct isds_list *list) {
    const struct isds_list *expected_item, *item;
    int i;

    for (i = 1, expected_item = expected_list, item = list;
            NULL != expected_item && NULL != item;
            i++, expected_item = expected_item->next, item = item->next) {
        if (compare_fulltext_results(
                    (struct isds_fulltext_result *)expected_item->data,
                    (struct isds_fulltext_result *)item->data))
            return 1;
    }
    if (NULL != expected_item && NULL == item)
        FAIL_TEST("Result list is missing %d. item", i);
    if (NULL == expected_item && NULL != item)
        FAIL_TEST("Result list has superfluous %d. item", i);

    return 0;
}

struct test_destructor_argument {
    unsigned long int *total_matching_boxes;
    unsigned long int *current_page_beginning;
    unsigned long int *current_page_size;
    _Bool *last_page;
    struct isds_list *results;
};

static void test_destructor(void *argument) {
    if (NULL == argument) return;
    free(((struct test_destructor_argument *)argument)->total_matching_boxes);
    free(((struct test_destructor_argument *)argument)->current_page_beginning);
    free(((struct test_destructor_argument *)argument)->current_page_size);
    free(((struct test_destructor_argument *)argument)->last_page);
    isds_list_free(
            &((struct test_destructor_argument *)argument)->results
    );
}

static int test_isds_find_box_by_fulltext(const isds_error expected_error,
        struct isds_ctx *context,
        const char *query,
        const isds_fulltext_target *target,
        const isds_DbType *box_type,
        const unsigned long int *page_size,
        const unsigned long int *page_number,
        const _Bool *track_matches,
        const unsigned long int *expected_total_matching_boxes,
        const unsigned long int *expected_current_page_beginning,
        const unsigned long int *expected_current_page_size,
        const _Bool *expected_last_page,
        const struct isds_list *expected_results) {
    isds_error error;
    struct test_destructor_argument allocated;
    TEST_FILL_INT(allocated.total_matching_boxes);
    TEST_FILL_INT(allocated.current_page_beginning);
    TEST_FILL_INT(allocated.current_page_size);
    TEST_FILL_INT(allocated.last_page);
    TEST_CALLOC(allocated.results);

    error = isds_find_box_by_fulltext(context,
            query, target, box_type, page_size, page_number, track_matches,
            &allocated.total_matching_boxes,
            &allocated.current_page_beginning,
            &allocated.current_page_size,
            &allocated.last_page,
            &allocated.results);
    TEST_DESTRUCTOR(test_destructor, &allocated);

    if (expected_error != error) {
        FAIL_TEST("Wrong return code: expected=%s, returned=%s (%s)",
                isds_strerror(expected_error), isds_strerror(error),
                isds_long_message(context));
    }

    if (IE_SUCCESS != error) {
        TEST_POINTER_IS_NULL(allocated.total_matching_boxes);
        TEST_POINTER_IS_NULL(allocated.current_page_beginning);
        TEST_POINTER_IS_NULL(allocated.current_page_size);
        TEST_POINTER_IS_NULL(allocated.last_page);
        TEST_POINTER_IS_NULL(allocated.results);
        PASS_TEST;
    }

    TEST_INTPTR_DUPLICITY(expected_total_matching_boxes,
            allocated.total_matching_boxes);
    TEST_INTPTR_DUPLICITY(expected_current_page_beginning,
            allocated.current_page_beginning);
    TEST_INTPTR_DUPLICITY(expected_current_page_size,
            allocated.current_page_size);
    TEST_BOOLEANPTR_DUPLICITY(expected_last_page,
            allocated.last_page);

    if (compare_result_lists(expected_results, allocated.results))
        return 1;

    
    PASS_TEST;
}

int main(void) {
    int error;
    pid_t server_process;
    struct isds_ctx *context = NULL;

    INIT_TEST("isds_find_box_by_fulltext");

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
        /* Full response */
        char *url = NULL;

        struct isds_list name_match_start = {
            .next = NULL,
            .data = NULL,
            .destructor = NULL
        };
        struct isds_list name_match_end = {
            .next = NULL,
            .data = NULL,
            .destructor = NULL
        };
        struct isds_list address_match_start = {
            .next = NULL,
            .data = NULL,
            .destructor = NULL
        };
        struct isds_list address_match_end = {
            .next = NULL,
            .data = NULL,
            .destructor = NULL
        };
        struct isds_fulltext_result result = {
            .dbID = "foo1234",
            .dbType = DBTYPE_OVM,
            .name = "Foo name",
            .name_match_start = &name_match_start,
            .name_match_end = &name_match_end,
            .address = "Address foo",
            .address_match_start = &address_match_start,
            .address_match_end = &address_match_end,
            .ic = "Foo IC",
            .biDate = NULL,
            .dbEffectiveOVM = 1,
            .active = 1,
            .public_sending = 1,
            .commercial_sending = 1
        };
        name_match_start.data = &result.name[0];
        name_match_end.data = &result.name[3];
        address_match_start.data = &result.address[8];
        address_match_end.data = &result.address[11];
        struct server_db_result server_result = {
            .id = "foo1234",
            .type = "OVM",
            .name = "|$*HL_START*$|Foo|$*HL_END*$| name",
            .address = "Address |$*HL_START*$|foo|$*HL_END*$|",
            .birth_date = NULL,
            .ic = "Foo IC",
            .ovm = 1,
            .send_options = "ALL"
        };
        const struct isds_list results = {
            .next = NULL,
            .data = &result,
            .destructor = NULL
        };
        const struct server_list server_results = {
            .next = NULL,
            .data = &server_result,
            .destructor = NULL
        };


        isds_fulltext_target target = FULLTEXT_ALL;
        isds_DbType box_type = DBTYPE_SYSTEM;
        long int search_page_number = 42;
        long int search_page_size = 43;
        _Bool search_highlighting_value = 1;
        unsigned long int total_count = 44;
        unsigned long int current_count = 1;
        unsigned long int position = 43 * 42;
        _Bool last_page = 1;
        const struct arguments_DS_df_ISDSSearch2 service_arguments = {
            .status_code = "0000",
            .status_message = "Ok.",
            .search_text = "foo",
            .search_type = "GENERAL",
            .search_scope = "ALL",
            .search_page_number = &search_page_number,
            .search_page_size = &search_page_size,
            .search_highlighting_value = &search_highlighting_value,
            .total_count = &total_count,
            .current_count = &current_count,
            .position = &position,
            .last_page = &last_page,
            .results_exists = 0,
            .results = &server_results
        };
        const struct service_configuration services[] = {
            { SERVICE_DS_Dz_DummyOperation, NULL },
            { SERVICE_DS_df_ISDSSearch2, &service_arguments },
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

        TEST("All data", test_isds_find_box_by_fulltext, IE_SUCCESS,
                context, service_arguments.search_text, &target, &box_type,
                (unsigned long int *)service_arguments.search_page_size,
                (unsigned long int *)service_arguments.search_page_number,
                service_arguments.search_highlighting_value,
                service_arguments.total_count,
                service_arguments.position,
                service_arguments.current_count,
                service_arguments.last_page,
                &results);

        isds_logout(context);
        if (stop_server(server_process)) {
            isds_ctx_free(&context);
            isds_cleanup();
            ABORT_UNIT(server_error);
        }
    }


    {
        /* Empty response due to an error */
        char *url = NULL;

        long int search_page_number = 42;
        long int search_page_size = 43;
        const struct arguments_DS_df_ISDSSearch2 service_arguments = {
            .status_code = "1156",
            .status_message = "pageSize is too large",
            .search_text = "foo",
            .search_type = NULL,
            .search_scope = NULL,
            .search_page_number = &search_page_number,
            .search_page_size = &search_page_size,
            .search_highlighting_value = NULL,
            .total_count = NULL,
            .current_count = NULL,
            .position = NULL,
            .last_page = NULL,
            .results_exists = 0,
            .results = NULL
        };
        const struct service_configuration services[] = {
            { SERVICE_DS_Dz_DummyOperation, NULL },
            { SERVICE_DS_df_ISDSSearch2, &service_arguments },
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

        TEST("No data", test_isds_find_box_by_fulltext, IE_2BIG,
                context, service_arguments.search_text, NULL, NULL,
                (unsigned long int *)service_arguments.search_page_size,
                (unsigned long int *)service_arguments.search_page_number,
                NULL,
                NULL,
                NULL,
                NULL,
                NULL,
                NULL);

        isds_logout(context);
        if (stop_server(server_process)) {
            isds_ctx_free(&context);
            isds_cleanup();
            ABORT_UNIT(server_error);
        }
    }


    {
        /* Successful response without a mandatory element must be discarded
         * by the client, otherwise a garbage would appear in the output
         * arguments. */
        char *url = NULL;

        const struct arguments_DS_df_ISDSSearch2 service_arguments = {
            .status_code = "0000",
            .status_message = "totalCount is missing",
            .search_text = "foo",
            .search_type = NULL,
            .search_scope = NULL,
            .search_page_number = NULL,
            .search_page_size = NULL,
            .search_highlighting_value = NULL,
            .total_count = NULL,
            .current_count = NULL,
            .position = NULL,
            .last_page = NULL,
            .results_exists = 0,
            .results = NULL
        };
        const struct service_configuration services[] = {
            { SERVICE_DS_Dz_DummyOperation, NULL },
            { SERVICE_DS_df_ISDSSearch2, &service_arguments },
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

        TEST("Missing totalCount in a response", test_isds_find_box_by_fulltext,
                IE_SUCCESS, context, service_arguments.search_text, NULL, NULL,
                NULL,
                NULL,
                NULL,
                NULL,
                NULL,
                NULL,
                NULL,
                NULL);

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
