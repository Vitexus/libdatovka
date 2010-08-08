#include "test.h"
#include "isds.h"

static int test_isds_DbUserInfo_free(
        struct isds_DbUserInfo **DbUserInfo) {
    isds_DbUserInfo_free(DbUserInfo);
    if (!DbUserInfo) PASS_TEST;

    if (*DbUserInfo)
        FAIL_TEST("isds_DbUserInfo_free() did not null pointer");

    PASS_TEST;
}


int main(int argc, char **argv) {

    INIT_TEST("isds_DbUserInfo_free()");
    if (isds_init())
        ABORT_UNIT("isds_init() failed");
    
    struct isds_DbUserInfo *DbUserInfo = NULL;
    TEST("NULL", test_isds_DbUserInfo_free, NULL);
    TEST("*NULL", test_isds_DbUserInfo_free, &DbUserInfo);

    TEST_CALLOC(DbUserInfo);
    TEST("Empty structure", test_isds_DbUserInfo_free, &DbUserInfo);

    /* Full structure */
    TEST_CALLOC(DbUserInfo);
    TEST_FILL_STRING(DbUserInfo->userID);
    TEST_FILL_INT(DbUserInfo->userType)
    TEST_FILL_INT(DbUserInfo->userPrivils);

    TEST_CALLOC(DbUserInfo->personName);       /* Name of the person */
    TEST_FILL_STRING(DbUserInfo->personName->pnFirstName);
    TEST_FILL_STRING(DbUserInfo->personName->pnMiddleName);
    TEST_FILL_STRING(DbUserInfo->personName->pnLastName);
    TEST_FILL_STRING(DbUserInfo->personName->pnLastNameAtBirth);

    TEST_CALLOC(DbUserInfo->address);          /* Post address */
    TEST_FILL_STRING(DbUserInfo->address->adCity);
    TEST_FILL_STRING(DbUserInfo->address->adStreet);
    TEST_FILL_STRING(DbUserInfo->address->adNumberInStreet);
    TEST_FILL_STRING(DbUserInfo->address->adNumberInMunicipality);
    TEST_FILL_STRING(DbUserInfo->address->adZipCode);
    TEST_FILL_STRING(DbUserInfo->address->adState);

    TEST_CALLOC(DbUserInfo->biDate);           /* Date of birth in local time */
    TEST_FILL_STRING(DbUserInfo->ic);
    TEST_FILL_STRING(DbUserInfo->firmName);
    TEST_FILL_STRING(DbUserInfo->caStreet);
    TEST_FILL_STRING(DbUserInfo->caCity);
    TEST_FILL_STRING(DbUserInfo->caZipCode);
    TEST_FILL_STRING(DbUserInfo->caState);
    TEST("Full structure", test_isds_DbUserInfo_free, &DbUserInfo);

    isds_cleanup();
    SUM_TEST();
}
