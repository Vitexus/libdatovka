#include "../test.h"
#include "isds.h"

static int test_isds_message_status_change_free(
        struct isds_message_status_change **message_status_change) {
    isds_message_status_change_free(message_status_change);
    if (!message_status_change) PASS_TEST;

    if (*message_status_change)
        FAIL_TEST("isds_message_status_change_free() did not null pointer");

    PASS_TEST;
}


int main(int argc, char **argv) {

    INIT_TEST("isds_message_status_change_free()");
    if (isds_init())
        ABORT_UNIT("isds_init() failed");
    
    struct isds_message_status_change *message_status_change = NULL;

    TEST("NULL", test_isds_message_status_change_free, NULL);
    TEST("*NULL", test_isds_message_status_change_free, &message_status_change);

    TEST_CALLOC(message_status_change);
    TEST("Empty structure", test_isds_message_status_change_free,
            &message_status_change);

    TEST_CALLOC(message_status_change);
    TEST_FILL_STRING(message_status_change->dmID);
    TEST_CALLOC(message_status_change->time);
    TEST_FILL_INT(message_status_change->dmMessageStatus);
    TEST("Full structure", test_isds_message_status_change_free,
            &message_status_change);

    isds_cleanup();
    SUM_TEST();
}
