#include "../test.h"
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
        FAIL_TEST("conversion from string to isds_DbType did not fail");

    PASS_TEST;
}

static int test_dbtype(const isds_DbType type, const xmlChar *name) {
    xmlChar *string;
    isds_error err;
    isds_DbType new_type;

    string = (xmlChar *) isds_DbType2string(type);
    if (!string)
        FAIL_TEST("conversion from isds_DbType to string failed");

    if (xmlStrcmp(name, string))
        FAIL_TEST("Wrong to string conversion result");

    err = string2isds_DbType(string, &new_type);
    if (err)
        FAIL_TEST("conversion from string to isds_DbType failed");

    if (type != new_type)
        FAIL_TEST("double conversion not idempotent");

    PASS_TEST;
}

int main(void) {
    INIT_TEST("isds_DbType conversion");

    isds_DbType types[] =  {
        DBTYPE_OVM,
        DBTYPE_OVM_NOTAR,
        DBTYPE_OVM_EXEKUT,
        DBTYPE_OVM_REQ,
        DBTYPE_OVM_FO,
        DBTYPE_OVM_PFO,
        DBTYPE_OVM_PO,
        DBTYPE_PO,
        DBTYPE_PO_ZAK,
        DBTYPE_PO_REQ,
        DBTYPE_PFO,
        DBTYPE_PFO_ADVOK,
        DBTYPE_PFO_DANPOR,
        DBTYPE_PFO_INSSPR,
        DBTYPE_PFO_AUDITOR,
        DBTYPE_PFO_ZNALEC,
        DBTYPE_PFO_TLUMOCNIK,
        DBTYPE_PFO_REQ,
        DBTYPE_FO
    };

    const xmlChar *names[] = {
        BAD_CAST "OVM",
        BAD_CAST "OVM_NOTAR",
        BAD_CAST "OVM_EXEKUT",
        BAD_CAST "OVM_REQ",
        BAD_CAST "OVM_FO",
        BAD_CAST "OVM_PFO",
        BAD_CAST "OVM_PO",
        BAD_CAST "PO",
        BAD_CAST "PO_ZAK",
        BAD_CAST "PO_REQ",
        BAD_CAST "PFO",
        BAD_CAST "PFO_ADVOK",
        BAD_CAST "PFO_DANPOR",
        BAD_CAST "PFO_INSSPR",
        BAD_CAST "PFO_AUDITOR",
        BAD_CAST "PFO_ZNALEC",
        BAD_CAST "PFO_TLUMOCNIK",
        BAD_CAST "PFO_REQ",
        BAD_CAST "FO"
    };

    TEST("DBTYPE_OVM_MAIN", test_dbtype2string_must_fail, DBTYPE_OVM_MAIN);
    TEST("DBTYPE_SYSTEM", test_dbtype2string_must_fail, DBTYPE_SYSTEM);

    for (size_t i = 0; i < sizeof(types)/sizeof(types[0]); i++)
        TEST(names[i], test_dbtype, types[i], names[i]);

    TEST("1234", test_dbtype2string_must_fail, 1234);

    TEST("X-Invalid_Type", test_string2dbtype_must_fail,
            BAD_CAST "X-Invalid_Type");

    SUM_TEST();
}
