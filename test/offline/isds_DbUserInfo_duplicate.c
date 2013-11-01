#include "../test.h"
#include "isds.h"
#include <string.h>

static int test_isds_DbUserInfo_duplicate(struct isds_DbUserInfo *origin) {
    struct isds_DbUserInfo *copy = isds_DbUserInfo_duplicate(origin);
    TEST_DESTRUCTOR((void (*)(void *))isds_DbUserInfo_free, &copy);

    if (!origin) {
        if (copy) 
            FAIL_TEST("Duplicate of NULL should be NULL");
        PASS_TEST;
    }

    if (!copy)
        FAIL_TEST("isds_DbUserInfo_duplicate() returned NULL instead of "
                "pointer to copy");

    TEST_STRING_DUPLICITY(origin->userID, copy->userID);
    TEST_INTPTR_DUPLICITY(origin->userType, copy->userType);
    TEST_INTPTR_DUPLICITY(origin->userPrivils, copy->userPrivils);

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

    /* Date of birth */
    TEST_POINTER_DUPLICITY(origin->biDate,
            copy->biDate);
    if(origin->biDate && copy->biDate) {
        if (origin->biDate->tm_year != copy->biDate->tm_year)
            FAIL_TEST("biDate differs in tm_year: expected=%d, got=%d",
                    origin->biDate->tm_year, copy->biDate->tm_year);
        if (origin->biDate->tm_mon != copy->biDate->tm_mon)
            FAIL_TEST("biDate differs in tm_mon: expected=%d, got=%d",
                    origin->biDate->tm_mon, copy->biDate->tm_mon);
        if (origin->biDate->tm_mday != copy->biDate->tm_mday)
            FAIL_TEST("biDate differs in tm_mday: expected=%d, got=%d",
                    origin->biDate->tm_mday, copy->biDate->tm_mday);
    }

    TEST_INTPTR_DUPLICITY(origin->ic, copy->ic);
    TEST_INTPTR_DUPLICITY(origin->firmName, copy->firmName);
    TEST_INTPTR_DUPLICITY(origin->caStreet, copy->caStreet);
    TEST_INTPTR_DUPLICITY(origin->caCity, copy->caCity);
    TEST_INTPTR_DUPLICITY(origin->caZipCode, copy->caZipCode);
    TEST_INTPTR_DUPLICITY(origin->caState, copy->caState);

    PASS_TEST;
}


int main(int argc, char **argv) {

    INIT_TEST("isds_DbUserInfo_duplicate()");
    if (isds_init())
        ABORT_UNIT("isds_init() failed");
    
    TEST("NULL", test_isds_DbUserInfo_duplicate, NULL);

    struct isds_DbUserInfo empty;
    memset(&empty, 0, sizeof(empty));
    TEST("Empty structure", test_isds_DbUserInfo_duplicate, &empty);

    /* Full structure */
    isds_UserType UserType = 2;
    long int UserPrivils = 3;
    struct isds_PersonName PersonName = {
        .pnFirstName = "P1",
        .pnMiddleName = "P2",
        .pnLastName = "P3",
        .pnLastNameAtBirth = "P4"
    };
    struct isds_Address Address = {
        .adCity = "A1",
        .adStreet = "A2",
        .adNumberInStreet = "A3",
        .adNumberInMunicipality = "A4",
        .adZipCode = "A5",
        .adState = "A6"
    };
    struct tm BiDate = {
        .tm_year = 1,
        .tm_mon = 2,
        .tm_mday = 3
    };
    struct isds_DbUserInfo full = {
        .userID = "1",
        .userType = &UserType,
        .userPrivils = &UserPrivils,
        .personName = &PersonName,
        .address = &Address,
        .biDate = &BiDate,
        .ic = "7",
        .firmName = "8",
        .caStreet = "9",
        .caCity = "10",
        .caZipCode = "11",
        .caState = "12"
    };
    TEST("Full structure", test_isds_DbUserInfo_duplicate, &full);

    isds_cleanup();
    SUM_TEST();
}
