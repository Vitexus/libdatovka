#include "test.h"
#include "isds.c"


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
        DBTYPE_SYSTEM,          /* This is special sender value for messages
                                       sent by ISDS. */
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

    for (int i = 0; i < sizeof(types)/sizeof(types[0]); i++)
        TEST(isds_DbType2string(types[i]), test_dbtype, types[i]);

    SUM_TEST();
}
