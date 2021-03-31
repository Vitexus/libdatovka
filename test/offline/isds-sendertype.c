#include "../test.h"
#include "isds.c"

static int test_string2sendertype(const xmlChar *string, isds_error error,
        isds_sender_type *type) {
    isds_error err;
    isds_sender_type new_type = 0;

    err = string2isds_sender_type(string, (type) ? &new_type : NULL);
    if (err != error)
        FAIL_TEST("string2isds_sender_type() returend wrong exit code: "
                "expected=%s, got=%s",
                isds_strerror(error), isds_strerror(err));
    if (err != IE_SUCCESS)
        PASS_TEST;

    if (type && *type != new_type)
        FAIL_TEST("string2isds_sender_type() returned wrong type: "
                "expected=%d, got=%d", *type, new_type);

    PASS_TEST;
}

int main(void) {
    INIT_TEST("isds_sender_type conversion");

    isds_sender_type types[] =  {
        SENDERTYPE_PRIMARY,
        SENDERTYPE_ENTRUSTED,
        SENDERTYPE_ADMINISTRATOR,
        SENDERTYPE_OFFICIAL,
        SENDERTYPE_VIRTUAL,
        SENDERTYPE_OFFICIAL_CERT,
        SENDERTYPE_LIQUIDATOR,
        SENDERTYPE_RECEIVER,
        SENDERTYPE_GUARDIAN
    };

    const xmlChar *names[] = {
        BAD_CAST "PRIMARY_USER",
        BAD_CAST "ENTRUSTED_USER",
        BAD_CAST "ADMINISTRATOR",
        BAD_CAST "OFFICIAL",
        BAD_CAST "VIRTUAL",
        BAD_CAST "OFFICIAL_CERT",
        BAD_CAST "LIQUIDATOR",
        BAD_CAST "RECEIVER",
        BAD_CAST "GUARDIAN"
    };


    /* Known values */
    for (size_t i = 0; i < sizeof(types)/sizeof(types[0]); i++)
        TEST(names[i], test_string2sendertype,
            names[i], IE_SUCCESS, &types[i]);

    /* Uknown value */
    TEST("X-Invalid_Type", test_string2sendertype,
            BAD_CAST "X-Invalid_Type", IE_ENUM, &types[0]);

    /* Invalid invocation */
    TEST("NULL string", test_string2sendertype,
            BAD_CAST NULL, IE_INVAL, &types[0]);
    TEST("NULL type", test_string2sendertype,
            names[0], IE_INVAL, NULL);
    SUM_TEST();
}
