#include "../test.h"
#include "isds.h"
#include <string.h>

static int test_isds_Address_duplicate(struct isds_Address *origin) {
    struct isds_Address *copy = isds_Address_duplicate(origin);
    TEST_DESTRUCTOR((void(*)(void*))isds_Address_free, (void *)&copy);

    if (!origin) {
        if (copy) 
            FAIL_TEST("Duplicate of NULL should be NULL");
        PASS_TEST;
    }

    if (!copy)
        FAIL_TEST("isds_Address_duplicate() returned NULL instead of "
                "pointer to copy");

    TEST_STRING_DUPLICITY(origin->adCity, copy->adCity);
    TEST_STRING_DUPLICITY(origin->adStreet, copy->adStreet);
    TEST_STRING_DUPLICITY(origin->adNumberInStreet, copy->adNumberInStreet);
    TEST_STRING_DUPLICITY(origin->adNumberInMunicipality,
            copy->adNumberInMunicipality);
    TEST_STRING_DUPLICITY(origin->adZipCode, copy->adZipCode);
    TEST_STRING_DUPLICITY(origin->adState, copy->adState);

    PASS_TEST;
}


int main(int argc, char **argv) {

    INIT_TEST("isds_Address_duplicate()");
    if (isds_init())
        ABORT_UNIT("isds_init() failed");
    
    TEST("NULL", test_isds_Address_duplicate, NULL);

    struct isds_Address empty;
    memset(&empty, 0, sizeof(empty));
    TEST("Empty structure", test_isds_Address_duplicate, &empty);

    /* Full structure */
    struct isds_Address full = {
        .adCity = "1",
        .adStreet = "2",
        .adNumberInStreet = "3",
        .adNumberInMunicipality = "4",
        .adZipCode = "5",
        .adState = "6"
    };

    TEST("Full structure", test_isds_Address_duplicate, &full);

    isds_cleanup();
    SUM_TEST();
}
