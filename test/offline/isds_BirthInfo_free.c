#include "../test.h"
#include "isds.h"

static int test_isds_BirthInfo_free(
        struct isds_BirthInfo **BirthInfo) {
    isds_BirthInfo_free(BirthInfo);
    if (NULL == BirthInfo) PASS_TEST;

    if (NULL != *BirthInfo)
        FAIL_TEST("isds_BirthInfo_free() did not null pointer");

    PASS_TEST;
}


int main(void) {

    INIT_TEST("isds_BirthInfo_free()");
    if (isds_init())
        ABORT_UNIT("isds_init() failed");
    
    struct isds_BirthInfo *BirthInfo = NULL;
    TEST("NULL", test_isds_BirthInfo_free, NULL);
    TEST("*NULL", test_isds_BirthInfo_free, &BirthInfo);

    TEST_CALLOC(BirthInfo);
    TEST("Empty structure", test_isds_BirthInfo_free, &BirthInfo);

    /* Full structure */
    TEST_CALLOC(BirthInfo);
    TEST_CALLOC(BirthInfo->biDate);
    TEST_FILL_STRING(BirthInfo->biCity);
    TEST_FILL_STRING(BirthInfo->biCounty);
    TEST_FILL_STRING(BirthInfo->biState);
    TEST("Full structure", test_isds_BirthInfo_free, &BirthInfo);

    isds_cleanup();
    SUM_TEST();
}
