#include "../test.h"
#include "libdatovka/isds.h"

static int test_isds_DbOwnerInfoExt2_free(
        struct isds_DbOwnerInfoExt2 **DbOwnerInfo) {
    isds_DbOwnerInfoExt2_free(DbOwnerInfo);
    if (!DbOwnerInfo) PASS_TEST;

    if (*DbOwnerInfo)
        FAIL_TEST("isds_DbOwnerInfoExt2_free() did not null pointer");

    PASS_TEST;
}


int main(void) {

    INIT_TEST("isds_DbOwnerInfoExt2_free()");
    if (isds_init())
        ABORT_UNIT("isds_init() failed");

    struct isds_DbOwnerInfoExt2 *DbOwnerInfo = NULL;
    TEST("NULL", test_isds_DbOwnerInfoExt2_free, NULL);
    TEST("*NULL", test_isds_DbOwnerInfoExt2_free, &DbOwnerInfo);

    TEST_CALLOC(DbOwnerInfo);
    TEST("Empty structure", test_isds_DbOwnerInfoExt2_free, &DbOwnerInfo);


    /* Full structure */
    TEST_CALLOC(DbOwnerInfo);
    TEST_FILL_STRING(DbOwnerInfo->dbID);
    TEST_FILL_INT(DbOwnerInfo->aifoIsds);
    TEST_FILL_INT(DbOwnerInfo->dbType);
    TEST_FILL_STRING(DbOwnerInfo->ic);

    TEST_CALLOC(DbOwnerInfo->personName);       /* Name of person */
    TEST_FILL_STRING(DbOwnerInfo->personName->pnGivenNames);
    TEST_FILL_STRING(DbOwnerInfo->personName->pnLastName);

    TEST_FILL_STRING(DbOwnerInfo->firmName);

    TEST_CALLOC(DbOwnerInfo->birthInfo);        /* Birth of person */
    TEST_CALLOC(DbOwnerInfo->birthInfo->biDate);      /* Date of Birth */
    TEST_FILL_STRING(DbOwnerInfo->birthInfo->biCity);
    TEST_FILL_STRING(DbOwnerInfo->birthInfo->biCounty);
    TEST_FILL_STRING(DbOwnerInfo->birthInfo->biState);

    TEST_CALLOC(DbOwnerInfo->address);          /* Post address */
    TEST_FILL_STRING(DbOwnerInfo->address->adCode);
    TEST_FILL_STRING(DbOwnerInfo->address->adCity);
    TEST_FILL_STRING(DbOwnerInfo->address->adDistrict);
    TEST_FILL_STRING(DbOwnerInfo->address->adStreet);
    TEST_FILL_STRING(DbOwnerInfo->address->adNumberInStreet);
    TEST_FILL_STRING(DbOwnerInfo->address->adNumberInMunicipality);
    TEST_FILL_STRING(DbOwnerInfo->address->adZipCode);
    TEST_FILL_STRING(DbOwnerInfo->address->adState);

    TEST_FILL_STRING(DbOwnerInfo->nationality);
    TEST_FILL_STRING(DbOwnerInfo->dbIdOVM);
    TEST_FILL_INT(DbOwnerInfo->dbState);
    TEST_FILL_INT(DbOwnerInfo->dbOpenAddressing);
    TEST_FILL_STRING(DbOwnerInfo->dbUpperID);
    TEST("Full structure", test_isds_DbOwnerInfoExt2_free, &DbOwnerInfo);

    isds_cleanup();
    SUM_TEST();
}
