#include "../test.h"
#include "libdatovka/isds.h"

static int test_isds_Address_free(
        struct isds_Address **Address) {
    isds_Address_free(Address);
    if (NULL == Address) PASS_TEST;

    if (NULL != *Address)
        FAIL_TEST("isds_Address_free() did not null pointer");

    PASS_TEST;
}


int main(void) {

    INIT_TEST("isds_Address_free()");
    if (isds_init())
        ABORT_UNIT("isds_init() failed");
    
    struct isds_Address *Address = NULL;
    TEST("NULL", test_isds_Address_free, NULL);
    TEST("*NULL", test_isds_Address_free, &Address);

    TEST_CALLOC(Address);
    TEST("Empty structure", test_isds_Address_free, &Address);

    TEST_CALLOC(Address);
    TEST_FILL_STRING(Address->adCity);
    TEST_FILL_STRING(Address->adStreet);
    TEST_FILL_STRING(Address->adNumberInStreet);
    TEST_FILL_STRING(Address->adNumberInMunicipality);
    TEST_FILL_STRING(Address->adZipCode);
    TEST_FILL_STRING(Address->adState);
    TEST("Full structure", test_isds_Address_free, &Address);

    isds_cleanup();
    SUM_TEST();
}
