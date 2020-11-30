#include "../test.h"
#include "libdatovka/isds.h"
#include <string.h>

static int test_isds_DbUserInfoExt2_duplicate(
        struct isds_DbUserInfoExt2 *origin) {
    struct isds_DbUserInfoExt2 *copy = isds_DbUserInfoExt2_duplicate(origin);
    TEST_DESTRUCTOR((void (*)(void *))isds_DbUserInfoExt2_free, &copy);

    if (!origin) {
        if (copy)
            FAIL_TEST("Duplicate of NULL should be NULL");
        PASS_TEST;
    }

    if (!copy)
        FAIL_TEST("isds_DbUserInfoExt2_duplicate() returned NULL instead of "
                "pointer to copy");

    TEST_BOOLEANPTR_DUPLICITY(origin->aifoIsds, copy->aifoIsds);

    /* Name of person */
    TEST_POINTER_DUPLICITY(origin->personName, copy->personName);
    if (origin->personName && copy->personName) {
        TEST_STRING_DUPLICITY(origin->personName->pnGivenNames,
                copy->personName->pnGivenNames);
        TEST_STRING_DUPLICITY(origin->personName->pnLastName,
                copy->personName->pnLastName);
    }

    /* Post address */
    TEST_POINTER_DUPLICITY(origin->address, copy->address);
    if (origin->address && copy->address) {
        TEST_STRING_DUPLICITY(origin->address->adCode,
                copy->address->adCode);
        TEST_STRING_DUPLICITY(origin->address->adCity,
                copy->address->adCity);
        TEST_STRING_DUPLICITY(origin->address->adDistrict,
                copy->address->adDistrict);
        TEST_STRING_DUPLICITY(origin->address->adStreet,
                copy->address->adStreet);
        TEST_STRING_DUPLICITY(origin->address->adNumberInStreet,
                copy->address->adNumberInStreet);
        TEST_STRING_DUPLICITY(origin->address->adNumberInMunicipality,
                copy->address->adNumberInMunicipality);
        TEST_STRING_DUPLICITY(origin->address->adZipCode,
                copy->address->adZipCode);
        TEST_STRING_DUPLICITY(origin->address->adState,
                copy->address->adState);
    }

    TEST_TMPTR_DUPLICITY(origin->biDate, copy->biDate);
    TEST_STRING_DUPLICITY(origin->isdsID, copy->isdsID);
    TEST_INTPTR_DUPLICITY(origin->userType, copy->userType);
    TEST_INTPTR_DUPLICITY(origin->userPrivils, copy->userPrivils);
    TEST_INTPTR_DUPLICITY(origin->ic, copy->ic);
    TEST_INTPTR_DUPLICITY(origin->firmName, copy->firmName);
    TEST_INTPTR_DUPLICITY(origin->caStreet, copy->caStreet);
    TEST_INTPTR_DUPLICITY(origin->caCity, copy->caCity);
    TEST_INTPTR_DUPLICITY(origin->caZipCode, copy->caZipCode);
    TEST_INTPTR_DUPLICITY(origin->caState, copy->caState);

    PASS_TEST;
}


int main(void) {

    INIT_TEST("isds_DbUserInfoExt2_duplicate()");
    if (isds_init())
        ABORT_UNIT("isds_init() failed");

    TEST("NULL", test_isds_DbUserInfoExt2_duplicate, NULL);

    struct isds_DbUserInfoExt2 empty;
    memset(&empty, 0, sizeof(empty));
    TEST("Empty structure", test_isds_DbUserInfoExt2_duplicate, &empty);

    /* Full structure */
    _Bool aifoIsds = 1;
    struct isds_PersonName2 PersonName = {
        .pnGivenNames = "P1",
        .pnLastName = "P2"
    };
    struct isds_AddressExt2 Address = {
        .adCode = "A1",
        .adCity = "A2",
        .adDistrict = "A3",
        .adStreet = "A4",
        .adNumberInStreet = "A5",
        .adNumberInMunicipality = "A6",
        .adZipCode = "A7",
        .adState = "A8"
    };
    struct tm BiDate = {
        .tm_year = 1,
        .tm_mon = 2,
        .tm_mday = 3
    };
    isds_UserType UserType = 6;
    long int UserPrivils = 7;
    struct isds_DbUserInfoExt2 full = {
        .aifoIsds = &aifoIsds,
        .personName = &PersonName,
        .address = &Address,
        .biDate = &BiDate,
        .isdsID = "5",
        .userType = &UserType,
        .userPrivils = &UserPrivils,
        .ic = "8",
        .firmName = "9",
        .caStreet = "10",
        .caCity = "11",
        .caZipCode = "12",
        .caState = "13"
    };
    TEST("Full structure", test_isds_DbUserInfoExt2_duplicate, &full);

    isds_cleanup();
    SUM_TEST();
}
