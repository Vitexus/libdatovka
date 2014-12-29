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
    unsigned long int total_matching_boxes = 99;
    unsigned long int current_page_beginning = 99;
    unsigned long int current_page_size = 99;
    _Bool last_page = 1;
    struct isds_list *results = NULL;

    error = isds_find_box_by_fulltext(context,
            query, target, box_type, page_size, page_number, track_matches,
            (NULL == expected_total_matching_boxes) ?
                NULL : &total_matching_boxes,
            (NULL == expected_current_page_beginning) ?
                NULL : &current_page_beginning,
            (NULL == expected_current_page_size) ?
                NULL : &current_page_size,
            (NULL == expected_last_page) ?
                NULL : &last_page,
            &results);
    TEST_DESTRUCTOR((void(*)(void*))isds_list_free, &results);

    if (expected_error != error) {
        FAIL_TEST("Wrong return code: expected=%s, returned=%s (%s)",
                isds_strerror(expected_error), isds_strerror(error),
                isds_long_message(context));
    }

    if (IE_SUCCESS != error)
        PASS_TEST;

    if (NULL != expected_total_matching_boxes)
        TEST_INT_DUPLICITY(*expected_total_matching_boxes,
                total_matching_boxes);
    if (NULL != expected_current_page_beginning)
        TEST_INT_DUPLICITY(*expected_current_page_beginning,
                current_page_beginning);
    if (NULL != expected_current_page_size)
        TEST_INT_DUPLICITY(*expected_current_page_size,
                current_page_size);
    if (NULL != expected_last_page)
        TEST_BOOLEAN_DUPLICITY(*expected_last_page,
                last_page);

    if (compare_result_lists(expected_results, results))
        return 1;

    
    PASS_TEST;
}

int main(int argc, char **argv) {
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


    isds_ctx_free(&context);
    isds_cleanup();
    SUM_TEST();
}
