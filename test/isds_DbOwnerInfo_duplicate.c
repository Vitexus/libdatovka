#include "test.h"
#include "isds.h"

static int test_isds_DbOwnerInfo_duplicate(struct isds_DbOwnerInfo *origin) {
    struct isds_DbOwnerInfo *copy = isds_DbOwnerInfo_duplicate(origin);

    if (!origin) {
        if (copy) 
            FAIL_TEST("Duplicate of NULL should be NULL");
        PASS_TEST;
    }

    if (!copy)
        FAIL_TEST("isds_DbOwnerInfo_duplicate() returned NULL instead of "
                "pointer to copy");

    TEST_STRING_DUPLICITY(origin->dbID, copy->dbID);
    TEST_INT_DUPLICITY(origin->dbType, copy->dbType);
    TEST_STRING_DUPLICITY(origin->ic, copy->ic);

    /* Name of person */
    TEST_POINTER_DUPLICITY(origin->personName, copy->personName);
    if (origin->personName && copy->personName) {
        TEST_STRING_DUPLICITY(origin->personName->pnFirstName,
                copy->personName->pnFirstName);
        TEST_STRING_DUPLICITY(origin->personName->pnMiddleName,
                copy->personName->pnMiddleName);
        TEST_STRING_DUPLICITY(origin->personName->pnLastName,
                copy->personName->pnLastName);
        TEST_STRING_DUPLICITY(origin->personName->pnLastNameAtBirth,
                copy->personName->pnLastNameAtBirth);
    }

    /* Birth of person */
    TEST_POINTER_DUPLICITY(origin->birthInfo, copy->birthInfo);
    if(origin->birthInfo && copy->birthInfo) {
        TEST_POINTER_DUPLICITY(origin->birthInfo->biDate,
                copy->birthInfo->biDate);
        if(origin->birthInfo->biDate && copy->birthInfo->biDate) {
            if (origin->birthInfo->biDate->tm_year !=
                    copy->birthInfo->biDate->tm_year)
                FAIL_TEST("biDate differs in tm_year: expected=%d, got=%d",
                        origin->birthInfo->biDate->tm_year,
                        copy->birthInfo->biDate->tm_year);
            if (origin->birthInfo->biDate->tm_mon !=
                    copy->birthInfo->biDate->tm_mon)
                FAIL_TEST("biDate differs in tm_mon: expected=%d, got=%d",
                        origin->birthInfo->biDate->tm_mon,
                        copy->birthInfo->biDate->tm_mon);
            if (origin->birthInfo->biDate->tm_mday !=
                    copy->birthInfo->biDate->tm_mday)
                FAIL_TEST("biDate differs in tm_mday: expected=%d, got=%d",
                        origin->birthInfo->biDate->tm_mday,
                        copy->birthInfo->biDate->tm_mday);
        }

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
        TEST_STRING_DUPLICITY(origin->address->adCity,
                copy->address->adCity);
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
    TEST_STRING_DUPLICITY(origin->email, copy->email);
    TEST_STRING_DUPLICITY(origin->telNumber, copy->telNumber);
    TEST_STRING_DUPLICITY(origin->identifier, copy->identifier);
    TEST_STRING_DUPLICITY(origin->registryCode, copy->registryCode);
    TEST_INT_DUPLICITY(origin->dbState, copy->dbState);
    TEST_BOOLEAN_DUPLICITY(origin->dbEffectiveOVM, copy->dbEffectiveOVM);
    TEST_BOOLEAN_DUPLICITY(origin->dbOpenAddressing, copy->dbOpenAddressing);

    PASS_TEST;
}


int main(int argc, char **argv) {

    INIT_TEST("isds_DbOwnerInfo_duplicate()");
    if (isds_init())
        ABORT_UNIT("isds_init() failed");
    
    TEST("NULL", test_isds_DbOwnerInfo_duplicate, NULL);

    struct isds_DbOwnerInfo *empty;
    TEST_CALLOC(empty);
    TEST("Empty structure", test_isds_DbOwnerInfo_duplicate, empty);

    /* Full structure */
    isds_DbType dbType = 2;
    struct isds_PersonName PersonName = {
        .pnFirstName = "P1",
        .pnMiddleName = "P2",
        .pnLastName = "P3",
        .pnLastNameAtBirth = "P4"
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
    struct isds_Address Address = {
        .adCity = "A1",
        .adStreet = "A2",
        .adNumberInStreet = "A3",
        .adNumberInMunicipality = "A4",
        .adZipCode = "A5",
        .adState = "A6"
    };
    long int DbState = 13;
    _Bool DbEffectiveOVM = 1;
    _Bool DbOpenAddressing = 1;
    struct isds_DbOwnerInfo full = {
        .dbID = "1",
        .dbType = &dbType,
        .ic = "3",
        .personName = &PersonName,
        .firmName = "5",
        .birthInfo = &BirthInfo,
        .address = &Address,
        .nationality = "8",
        .email = "9",
        .telNumber = "10",
        .identifier = "11",
        .registryCode = "12",
        .dbState = &DbState,
        .dbEffectiveOVM = &DbEffectiveOVM,
        .dbOpenAddressing = &DbOpenAddressing
    };
    TEST("Full structure", test_isds_DbOwnerInfo_duplicate, &full);

    isds_cleanup();
    SUM_TEST();
}
