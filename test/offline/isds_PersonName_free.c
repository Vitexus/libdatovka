#include "../test.h"
#include "libdatovka/isds.h"

static int test_isds_PersonName_free(
        struct isds_PersonName **PersonName) {
    isds_PersonName_free(PersonName);
    if (NULL == PersonName) PASS_TEST;

    if (NULL != *PersonName)
        FAIL_TEST("isds_PersonName_free() did not null pointer");

    PASS_TEST;
}


int main(void) {

    INIT_TEST("isds_PersonName_free()");
    if (isds_init())
        ABORT_UNIT("isds_init() failed");
    
    struct isds_PersonName *PersonName = NULL;
    TEST("NULL", test_isds_PersonName_free, NULL);
    TEST("*NULL", test_isds_PersonName_free, &PersonName);

    TEST_CALLOC(PersonName);
    TEST("Empty structure", test_isds_PersonName_free, &PersonName);

    TEST_CALLOC(PersonName);
    TEST_FILL_STRING(PersonName->pnFirstName);
    TEST_FILL_STRING(PersonName->pnMiddleName);
    TEST_FILL_STRING(PersonName->pnLastName);
    TEST_FILL_STRING(PersonName->pnLastNameAtBirth);
    TEST("Full structure", test_isds_PersonName_free, &PersonName);

    isds_cleanup();
    SUM_TEST();
}
