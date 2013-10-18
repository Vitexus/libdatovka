#include "../test.h"
#include "isds.c"

static int test_string2isds_payment_type(const xmlChar *string,
        isds_error error, isds_payment_type *type) {
    isds_error err;
    isds_payment_type new_type = 0;

    err = string2isds_payment_type(string, (type) ? &new_type : NULL);
    if (err != error)
        FAIL_TEST("string2isds_payment_type() returend wrong exit code: "
                "expected=%s, got=%s",
                isds_strerror(error), isds_strerror(err));
    if (err != IE_SUCCESS)
        PASS_TEST;

    if (type && *type != new_type)
        FAIL_TEST("string2isds_payment_type() returned wrong type: "
                "expected=%d, got=%d", *type, new_type);

    PASS_TEST;
}

int main(int argc, char **argv) {
    INIT_TEST("isds_payment_type conversion");
    
    const xmlChar *names[] = {
        BAD_CAST "K",
        BAD_CAST "E",
        BAD_CAST "G",
        BAD_CAST "O",
        BAD_CAST "Z",
        BAD_CAST "D",
    };

    isds_payment_type types[] =  {
        PAYMENT_SENDER,
        PAYMENT_STAMP,
        PAYMENT_SPONSOR,
        PAYMENT_RESPONSE,
        PAYMENT_SPONSOR_LIMITED,
        PAYMENT_SPONSOR_EXTERNAL
    };


    /* Known values */
    for (int i = 0; i < sizeof(types)/sizeof(types[0]); i++)
        TEST(names[i], test_string2isds_payment_type,
            names[i], IE_SUCCESS, &types[i]);

    /* Uknown value */
    TEST("X-Invalid_Type", test_string2isds_payment_type,
            BAD_CAST "X-Invalid_Type", IE_ENUM, &types[0]);

    /* Invalid invocation */
    TEST("NULL string", test_string2isds_payment_type,
            BAD_CAST NULL, IE_INVAL, &types[0]);
    TEST("NULL type", test_string2isds_payment_type,
            names[0], IE_INVAL, NULL);
    SUM_TEST();
}
