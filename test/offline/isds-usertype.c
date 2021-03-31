#include "../test.h"
#include "isds.c"

static int test_usertype2string_must_fail(const isds_UserType type) {
    xmlChar *string;

    string = (xmlChar *) isds_UserType2string(type);
    if (string)
        FAIL_TEST("conversion from isds_UserType to string did not fail");

    PASS_TEST;
}

static int test_string2usertype_must_fail(const xmlChar *string) {
    isds_error err;
    isds_UserType new_type;

    err = string2isds_UserType((xmlChar *)string, &new_type);
    if (!err)
        FAIL_TEST("conversion from string to isds_UserType did not fail");

    PASS_TEST;
}

static int test_usertype(const isds_UserType type, const xmlChar *name) {
    xmlChar *string;
    isds_error err;
    isds_UserType new_type;

    string = (xmlChar *) isds_UserType2string(type);
    if (!string)
        FAIL_TEST("conversion from isds_UserType to string failed");

    if (xmlStrcmp(name, string))
        FAIL_TEST("Wrong to string conversion result: "
                "expected=`%s', got=`%s'", name, string);

    err = string2isds_UserType(string, &new_type);
    if (err)
        FAIL_TEST("conversion from string to isds_UserType failed");

    if (type != new_type)
        FAIL_TEST("double conversion not idempotent: expected=%d, got=%d",
                type, new_type);

    PASS_TEST;
}

int main(void) {
    INIT_TEST("isds_UserType conversion");

    isds_UserType types[] =  {
        USERTYPE_PRIMARY,
        USERTYPE_ENTRUSTED,
        USERTYPE_ADMINISTRATOR,
        USERTYPE_OFFICIAL,
        USERTYPE_OFFICIAL_CERT,
        USERTYPE_LIQUIDATOR,
        USERTYPE_RECEIVER,
        USERTYPE_GUARDIAN
    };

    const xmlChar *names[] = {
        BAD_CAST "PRIMARY_USER",
        BAD_CAST "ENTRUSTED_USER",
        BAD_CAST "ADMINISTRATOR",
        BAD_CAST "OFFICIAL",
        BAD_CAST "OFFICIAL_CERT",
        BAD_CAST "LIQUIDATOR",
        BAD_CAST "RECEIVER",
        BAD_CAST "GUARDIAN"
    };


    for (size_t i = 0; i < sizeof(types)/sizeof(types[0]); i++)
        TEST(names[i], test_usertype, types[i], names[i]);

    TEST("1234", test_usertype2string_must_fail, 1234);

    TEST("X-Invalid_Type", test_string2usertype_must_fail,
            BAD_CAST "X-Invalid_Type");

    SUM_TEST();
}
