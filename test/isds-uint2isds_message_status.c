#include "test.h"
#include "isds.c"

static int test_uint2isds_message_status(struct isds_ctx *context,
        const unsigned long int *number, isds_error error,
        isds_message_status correct_status, isds_message_status **new_status) {
    isds_error err;

    err = uint2isds_message_status(context, number, new_status);

    if (error != err)
        FAIL_TEST("uint2isds_message_status() returned wrong error code: "
                "expected=%s, got=%s",
                isds_strerror(error), isds_strerror(err));
    if (err != IE_SUCCESS)
        PASS_TEST;

    if (new_status) {
        if (!*new_status)
            FAIL_TEST("uint2isds_message_status() does not failed "
                    "but freed output status variable");
        if (correct_status != **new_status)
            FAIL_TEST("uint2isds_message_status() returned wrong status: "
                    "expected=%d, got=%lu", correct_status, **new_status);
    }

    PASS_TEST;
}


int main(int argc, char **argv) {
    INIT_TEST("unsigned int to isds_message_status conversion");
    
    struct isds_ctx *context = NULL;
    isds_message_status *state = NULL;
    char *text = NULL;

    unsigned long int numbers[] = {
        1, 2, 3, 4, 5, 6, 7, 8, 9, 10
    };

    isds_message_status states[] =  {
        MESSAGESTATE_SENT,
        MESSAGESTATE_STAMPED,
        MESSAGESTATE_INFECTED,
        MESSAGESTATE_DELIVERED,
        MESSAGESTATE_SUBSTITUTED,
        MESSAGESTATE_RECEIVED,
        MESSAGESTATE_READ,
        MESSAGESTATE_UNDELIVERABLE,
        MESSAGESTATE_REMOVED,
        MESSAGESTATE_IN_SAFE
    };


    isds_init();
    context = isds_ctx_create();
    if (!context) ABORT_UNIT("Could not create ISDS context");

    /* Valid input */
    for (int i = 0; i < sizeof(numbers)/sizeof(numbers[0]); i++) {
        test_asprintf(&text, "%lu", numbers[i]);
        TEST(text, test_uint2isds_message_status, context, &numbers[i],
                IE_SUCCESS, states[i], &state);
    }

    /* Invalid invocations */
    unsigned long int number = 0;
    test_asprintf(&text, "Too low input (%lu)", number);
    TEST(text, test_uint2isds_message_status, context, &number,
            IE_ENUM, states[0], &state);

    number = 11;
    test_asprintf(&text, "Too high input (%lu)", number);
    TEST(text, test_uint2isds_message_status, context, &number,
            IE_ENUM, states[0], &state);

    TEST("NULL context", test_uint2isds_message_status, NULL, &numbers[0],
            IE_INVALID_CONTEXT, states[0], &state);
    TEST("NULL input", test_uint2isds_message_status, context, NULL,
            IE_INVAL, states[0], &state);
    TEST("NULL output", test_uint2isds_message_status, context, &numbers[0],
            IE_INVAL, states[0], NULL);

    isds_ctx_free(&context);
    isds_cleanup();
    SUM_TEST();
}
