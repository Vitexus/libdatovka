#include "../test.h"
#include "isds.h"
#include <string.h>

static int test_isds_AddressExt2_duplicate(struct isds_AddressExt2 *origin) {
    struct isds_AddressExt2 *copy = isds_AddressExt2_duplicate(origin);
    TEST_DESTRUCTOR((void(*)(void*))isds_AddressExt2_free, (void *)&copy);

    if (!origin) {
        if (copy)
            FAIL_TEST("Duplicate of NULL should be NULL");
        PASS_TEST;
    }

    if (!copy)
        FAIL_TEST("isds_AddressExt2_duplicate() returned NULL instead of "
                "pointer to copy");

    TEST_STRING_DUPLICITY(origin->adCode, copy->adCode);
    TEST_STRING_DUPLICITY(origin->adCity, copy->adCity);
    TEST_STRING_DUPLICITY(origin->adDistrict, copy->adDistrict);
    TEST_STRING_DUPLICITY(origin->adStreet, copy->adStreet);
    TEST_STRING_DUPLICITY(origin->adNumberInStreet, copy->adNumberInStreet);
    TEST_STRING_DUPLICITY(origin->adNumberInMunicipality,
            copy->adNumberInMunicipality);
    TEST_STRING_DUPLICITY(origin->adZipCode, copy->adZipCode);
    TEST_STRING_DUPLICITY(origin->adState, copy->adState);

    PASS_TEST;
}


int main(void) {

    INIT_TEST("isds_AddressExt2_duplicate()");
    if (isds_init())
        ABORT_UNIT("isds_init() failed");

    TEST("NULL", test_isds_AddressExt2_duplicate, NULL);

    struct isds_AddressExt2 empty;
    memset(&empty, 0, sizeof(empty));
    TEST("Empty structure", test_isds_AddressExt2_duplicate, &empty);

    /* Full structure */
    struct isds_AddressExt2 full = {
        .adCode = "1",
        .adCity = "2",
        .adDistrict = "3",
        .adStreet = "4",
        .adNumberInStreet = "5",
        .adNumberInMunicipality = "6",
        .adZipCode = "7",
        .adState = "8"
    };

    TEST("Full structure", test_isds_AddressExt2_duplicate, &full);

    isds_cleanup();
    SUM_TEST();
}
