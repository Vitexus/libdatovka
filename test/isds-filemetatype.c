#include "test.h"
#include "isds.c"

static int test_filemetatype2string_must_fail(const isds_FileMetaType type) {
    xmlChar *string;

    string = (xmlChar *) isds_FileMetaType2string(type);
    if (string) 
        FAIL_TEST("conversion from isds_FileMetaType to string did not fail");

    PASS_TEST;
}

static int test_string2filemetatype_must_fail(const xmlChar *string) {
    isds_error err;
    isds_FileMetaType new_type;

    err = string2isds_FileMetaType((xmlChar *)string, &new_type);
    if (!err)
        FAIL_TEST("conversion from string to isds_FileMetaType did not fail");

    PASS_TEST;
}

static int test_filemetatype(const isds_FileMetaType type) {
    xmlChar *string;
    isds_error err;
    isds_FileMetaType new_type;

    string = (xmlChar *) isds_FileMetaType2string(type);
    if (!string) 
        FAIL_TEST("conversion from isds_FileMetaType to string failed");

    err = string2isds_FileMetaType(string, &new_type);
    if (err)
        FAIL_TEST("conversion from string to isds_FileMetaTyoe failed");

    if (type != new_type)
        FAIL_TEST("double conversion not idempotent");

    PASS_TEST;
}

int main(int argc, char **argv) {
    INIT_TEST("isds_FileMetaType conversion");
    
    isds_FileMetaType types[] =  {
        FILEMETATYPE_MAIN,              /* Main document */
        FILEMETATYPE_ENCLOSURE,         /* Appendix */
        FILEMETATYPE_SIGNATURE,         /* Digital signature of other document */
        FILEMETATYPE_META               /* XML document for ESS (electronic */
    };

    for (int i = 0; i < sizeof(types)/sizeof(types[0]); i++)
        TEST(isds_FileMetaType2string(types[i]), test_filemetatype, types[i]);

    TEST("1234", test_filemetatype2string_must_fail, 1234); 
    
    TEST("X-Invalid_Type", test_string2filemetatype_must_fail,
            BAD_CAST "X-Invalid_Type");

    SUM_TEST();
}
