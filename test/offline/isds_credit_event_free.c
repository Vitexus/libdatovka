#include "../test.h"
#include "isds.h"

static int test_isds_credit_event_free(
        struct isds_credit_event **event) {
    isds_credit_event_free(event);
    if (!event) PASS_TEST;

    if (*event)
        FAIL_TEST("isds_credit_event_free() did not null pointer");

    PASS_TEST;
}


int main(int argc, char **argv) {
    INIT_TEST("isds_credit_event_free()");
    if (isds_init())
        ABORT_UNIT("isds_init() failed");
    
    struct isds_credit_event *event = NULL;
    TEST("NULL", test_isds_credit_event_free, NULL);
    TEST("*NULL", test_isds_credit_event_free, &event);

    TEST_CALLOC(event);
    TEST("Empty structure", test_isds_credit_event_free, &event);

    TEST_CALLOC(event);
    TEST_CALLOC(event->time);
    event->type = ISDS_CREDIT_CHARGED;
    TEST_FILL_STRING(event->details.charged.transaction);
    TEST("Full structure ISDS_CREDIT_CHARGED",
            test_isds_credit_event_free, &event);

    TEST_CALLOC(event);
    TEST_CALLOC(event->time);
    event->type = ISDS_CREDIT_DISCHARGED;
    TEST_FILL_STRING(event->details.discharged.transaction);
    TEST("Full structure ISDS_CREDIT_DISCHARGED",
            test_isds_credit_event_free, &event);

    TEST_CALLOC(event);
    TEST_CALLOC(event->time);
    event->type = ISDS_CREDIT_MESSAGE_SENT;
    TEST_FILL_STRING(event->details.message_sent.recipient);
    TEST_FILL_STRING(event->details.message_sent.message_id);
    TEST("Full structure ISDS_CREDIT_MESSAGE_SENT",
            test_isds_credit_event_free, &event);

    TEST_CALLOC(event);
    TEST_CALLOC(event->time);
    event->type = ISDS_CREDIT_STORAGE_SET;
    TEST_CALLOC(event->details.storage_set.new_valid_from);
    TEST_CALLOC(event->details.storage_set.new_valid_to);
    TEST_FILL_INT(event->details.storage_set.old_capacity);
    TEST_CALLOC(event->details.storage_set.old_valid_from);
    TEST_CALLOC(event->details.storage_set.old_valid_to);
    TEST_FILL_STRING(event->details.storage_set.initiator);
    TEST("Full structure ISDS_CREDIT_STORAGE_SET",
            test_isds_credit_event_free, &event);

    isds_cleanup();
    SUM_TEST();
}
