#include "../test.h"
#include "libdatovka/isds.h"

static int test_isds_fulltext_result_free(
        struct isds_fulltext_result **result) {
    isds_fulltext_result_free(result);
    if (NULL == result) PASS_TEST;

    if (NULL != *result)
        FAIL_TEST("isds_fulltext_result_free() did not null pointer");

    PASS_TEST;
}


int main(void) {

    INIT_TEST("isds_fulltext_result_free()");
    if (isds_init())
        ABORT_UNIT("isds_init() failed");

    struct isds_fulltext_result *result = NULL;
    TEST("NULL", test_isds_fulltext_result_free, NULL);
    TEST("*NULL", test_isds_fulltext_result_free, &result);

    TEST_CALLOC(result);
    TEST("Empty structure", test_isds_fulltext_result_free, &result);

    TEST_CALLOC(result);
    TEST_FILL_STRING(result->dbID);
    TEST_FILL_STRING(result->name);
    TEST_CALLOC(result->name_match_start);
    TEST_CALLOC(result->name_match_end);
    TEST_FILL_STRING(result->address);
    TEST_CALLOC(result->address_match_start);
    TEST_CALLOC(result->address_match_end);
    TEST_FILL_STRING(result->ic);
    TEST_CALLOC(result->biDate);
    TEST("Full structure", test_isds_fulltext_result_free, &result);

    isds_cleanup();
    SUM_TEST();
}
