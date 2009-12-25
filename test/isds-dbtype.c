#include "test.h"
#include "isds.c"

static int test_dbtype2string_must_fail(const isds_DbType type) {
    xmlChar *string;

    string = (xmlChar *) isds_DbType2string(type);
    if (string) 
        FAIL_TEST("conversion from isds_DbType to string did not fail");

    PASS_TEST;
}

static int test_string2dbtype_must_fail(const xmlChar *string) {
    isds_error err;
    isds_DbType new_type;

    err = string2isds_DbType((xmlChar *)string, &new_type);
    if (!err)
        FAIL_TEST("conversion from string to isds_DbTyoe did not fail");

    PASS_TEST;
}

static int test_dbtype(const isds_DbType type) {
    xmlChar *string;
    isds_error err;
    isds_DbType new_type;

    string = (xmlChar *) isds_DbType2string(type);
    if (!string) 
        FAIL_TEST("conversion from isds_DbType to string failed");

    err = string2isds_DbType(string, &new_type);
    if (err)
        FAIL_TEST("conversion from string to isds_DbTyoe failed");

    if (type != new_type)
        FAIL_TEST("double conversion not idempotent");

    PASS_TEST;
}

int main(int argc, char **argv) {
    INIT_TEST("isds_DbType conversion");
    
    isds_DbType types[] =  {
        DBTYPE_OVM,
        DBTYPE_OVM_NOTAR,
        DBTYPE_OVM_EXEKUT,
        DBTYPE_OVM_REQ,
        DBTYPE_PO,
        DBTYPE_PO_ZAK,
        DBTYPE_PO_REQ,
        DBTYPE_PFO,
        DBTYPE_PFO_ADVOK,
        DBTYPE_PFO_DANPOR,
        DBTYPE_PFO_INSSPR,
        DBTYPE_FO
    };

    TEST("DBTYPE_SYSTEM", test_dbtype2string_must_fail, DBTYPE_SYSTEM); 

    for (int i = 0; i < sizeof(types)/sizeof(types[0]); i++)
        TEST(isds_DbType2string(types[i]), test_dbtype, types[i]);

    TEST("1234", test_dbtype2string_must_fail, 1234); 
    
    TEST("X-Invalid_Type", test_string2dbtype_must_fail,
            BAD_CAST "X-Invalid_Type");

    SUM_TEST();
}
