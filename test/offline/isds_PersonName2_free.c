#include "../test.h"
#include "libdatovka/isds.h"

static int test_isds_PersonName2_free(
        struct isds_PersonName2 **PersonName) {
    isds_PersonName2_free(PersonName);
    if (NULL == PersonName) PASS_TEST;

    if (NULL != *PersonName)
        FAIL_TEST("isds_PersonName2_free() did not null pointer");

    PASS_TEST;
}


int main(void) {

    INIT_TEST("isds_PersonName2_free()");
    if (isds_init())
        ABORT_UNIT("isds_init() failed");

    struct isds_PersonName2 *PersonName = NULL;
    TEST("NULL", test_isds_PersonName2_free, NULL);
    TEST("*NULL", test_isds_PersonName2_free, &PersonName);

    TEST_CALLOC(PersonName);
    TEST("Empty structure", test_isds_PersonName2_free, &PersonName);

    TEST_CALLOC(PersonName);
    TEST_FILL_STRING(PersonName->pnGivenNames);
    TEST_FILL_STRING(PersonName->pnLastName);
    TEST("Full structure", test_isds_PersonName2_free, &PersonName);

    isds_cleanup();
    SUM_TEST();
}
