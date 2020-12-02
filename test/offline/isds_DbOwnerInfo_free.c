#include "../test.h"
#include "libdatovka/isds.h"

static int test_isds_DbOwnerInfo_free(
        struct isds_DbOwnerInfo **DbOwnerInfo) {
    isds_DbOwnerInfo_free(DbOwnerInfo);
    if (!DbOwnerInfo) PASS_TEST;

    if (*DbOwnerInfo)
        FAIL_TEST("isds_DbOwnerInfo_free() did not null pointer");

    PASS_TEST;
}


int main(void) {

    INIT_TEST("isds_DbOwnerInfo_free()");
    if (isds_init())
        ABORT_UNIT("isds_init() failed");
    
    struct isds_DbOwnerInfo *DbOwnerInfo = NULL;
    TEST("NULL", test_isds_DbOwnerInfo_free, NULL);
    TEST("*NULL", test_isds_DbOwnerInfo_free, &DbOwnerInfo);

    TEST_CALLOC(DbOwnerInfo);
    TEST("Empty structure", test_isds_DbOwnerInfo_free, &DbOwnerInfo);


    /* Full structure */
    TEST_CALLOC(DbOwnerInfo);
    TEST_FILL_STRING(DbOwnerInfo->dbID);
    TEST_FILL_INT(DbOwnerInfo->dbType);
    TEST_FILL_STRING(DbOwnerInfo->ic);

    TEST_CALLOC(DbOwnerInfo->personName);       /* Name of person */
    TEST_FILL_STRING(DbOwnerInfo->personName->pnFirstName);
    TEST_FILL_STRING(DbOwnerInfo->personName->pnMiddleName);
    TEST_FILL_STRING(DbOwnerInfo->personName->pnLastName);
    TEST_FILL_STRING(DbOwnerInfo->personName->pnLastNameAtBirth);

    TEST_FILL_STRING(DbOwnerInfo->firmName);

    TEST_CALLOC(DbOwnerInfo->birthInfo);        /* Birth of person */
    TEST_CALLOC(DbOwnerInfo->birthInfo->biDate);      /* Date of Birth */
    TEST_FILL_STRING(DbOwnerInfo->birthInfo->biCity);
    TEST_FILL_STRING(DbOwnerInfo->birthInfo->biCounty); 
    TEST_FILL_STRING(DbOwnerInfo->birthInfo->biState);

    TEST_CALLOC(DbOwnerInfo->address);          /* Post address */
    TEST_FILL_STRING(DbOwnerInfo->address->adCity);
    TEST_FILL_STRING(DbOwnerInfo->address->adStreet);
    TEST_FILL_STRING(DbOwnerInfo->address->adNumberInStreet);
    TEST_FILL_STRING(DbOwnerInfo->address->adNumberInMunicipality);
    TEST_FILL_STRING(DbOwnerInfo->address->adZipCode);
    TEST_FILL_STRING(DbOwnerInfo->address->adState);

    TEST_FILL_STRING(DbOwnerInfo->nationality);
    TEST_FILL_STRING(DbOwnerInfo->email);
    TEST_FILL_STRING(DbOwnerInfo->telNumber);
    TEST_FILL_STRING(DbOwnerInfo->identifier);
    TEST_FILL_STRING(DbOwnerInfo->registryCode);
    TEST_FILL_INT(DbOwnerInfo->dbState);
    TEST_FILL_INT(DbOwnerInfo->dbEffectiveOVM);
    TEST_FILL_INT(DbOwnerInfo->dbOpenAddressing);
    TEST("Full structure", test_isds_DbOwnerInfo_free, &DbOwnerInfo);

    isds_cleanup();
    SUM_TEST();
}
