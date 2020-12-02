#include "../test.h"
#include "libdatovka/isds.h"
#include <string.h>

static int test_isds_DbOwnerInfoExt2_duplicate(
        struct isds_DbOwnerInfoExt2 *origin) {
    struct isds_DbOwnerInfoExt2 *copy = isds_DbOwnerInfoExt2_duplicate(origin);
    TEST_DESTRUCTOR((void(*)(void*))isds_DbOwnerInfoExt2_free, &copy);

    if (!origin) {
        if (copy)
            FAIL_TEST("Duplicate of NULL should be NULL");
        PASS_TEST;
    }

    if (!copy)
        FAIL_TEST("isds_DbOwnerInfoExt2_duplicate() returned NULL instead of "
                "pointer to copy");

    TEST_STRING_DUPLICITY(origin->dbID, copy->dbID);
    TEST_BOOLEANPTR_DUPLICITY(origin->aifoIsds, copy->aifoIsds);
    TEST_INTPTR_DUPLICITY(origin->dbType, copy->dbType);
    TEST_STRING_DUPLICITY(origin->ic, copy->ic);

    /* Name of person */
    TEST_POINTER_DUPLICITY(origin->personName, copy->personName);
    if (origin->personName && copy->personName) {
        TEST_STRING_DUPLICITY(origin->personName->pnGivenNames,
                copy->personName->pnGivenNames);
        TEST_STRING_DUPLICITY(origin->personName->pnLastName,
                copy->personName->pnLastName);
    }

    TEST_STRING_DUPLICITY(origin->firmName, copy->firmName);

    /* Birth of person */
    TEST_POINTER_DUPLICITY(origin->birthInfo, copy->birthInfo);
    if(origin->birthInfo && copy->birthInfo) {
        TEST_TMPTR_DUPLICITY(origin->birthInfo->biDate,
                copy->birthInfo->biDate);
        TEST_STRING_DUPLICITY(origin->birthInfo->biCity,
                copy->birthInfo->biCity);
        TEST_STRING_DUPLICITY(origin->birthInfo->biCounty,
                copy->birthInfo->biCounty);
        TEST_STRING_DUPLICITY(origin->birthInfo->biState,
                copy->birthInfo->biState);
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

    TEST_STRING_DUPLICITY(origin->nationality, copy->nationality);
    TEST_STRING_DUPLICITY(origin->dbIdOVM, copy->dbIdOVM);
    TEST_INTPTR_DUPLICITY(origin->dbState, copy->dbState);
    TEST_BOOLEANPTR_DUPLICITY(origin->dbOpenAddressing, copy->dbOpenAddressing);
    TEST_INTPTR_DUPLICITY(origin->dbUpperID, copy->dbUpperID);

    PASS_TEST;
}


int main(void) {

    INIT_TEST("isds_DbOwnerInfoExt2_duplicate()");
    if (isds_init())
        ABORT_UNIT("isds_init() failed");

    TEST("NULL", test_isds_DbOwnerInfoExt2_duplicate, NULL);

    struct isds_DbOwnerInfoExt2 empty;
    memset(&empty, 0, sizeof(empty));
    TEST("Empty structure", test_isds_DbOwnerInfoExt2_duplicate, &empty);

    /* Full structure */
    _Bool aifoIsds = 1;
    isds_DbType dbType = 2;
    struct isds_PersonName2 PersonName = {
        .pnGivenNames = "P1",
        .pnLastName = "P2"
    };
    struct tm BiDate = {
        .tm_year = 1,
        .tm_mon = 2,
        .tm_mday = 3
    };
    struct isds_BirthInfo BirthInfo = {
        .biDate = &BiDate,
        .biCity = "B2",
        .biCounty = "B3",
        .biState = "B4"
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
    long int DbState = 11;
    _Bool DbOpenAddressing = 1;
    struct isds_DbOwnerInfoExt2 full = {
        .dbID = "1",
        .aifoIsds = &aifoIsds,
        .dbType = &dbType,
        .ic = "4",
        .personName = &PersonName,
        .firmName = "6",
        .birthInfo = &BirthInfo,
        .address = &Address,
        .nationality = "9",
        .dbIdOVM = "10",
        .dbState = &DbState,
        .dbOpenAddressing = &DbOpenAddressing,
        .dbUpperID = "13"
    };
    TEST("Full structure", test_isds_DbOwnerInfoExt2_duplicate, &full);

    isds_cleanup();
    SUM_TEST();
}
