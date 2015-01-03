#include "../test.h"
#include "isds.h"

static int test_isds_event_free(
        struct isds_event **event) {
    isds_event_free(event);
    if (!event) PASS_TEST;

    if (*event)
        FAIL_TEST("isds_event_free() did not null pointer");

    PASS_TEST;
}


int main(void) {

    INIT_TEST("isds_event_free()");
    if (isds_init())
        ABORT_UNIT("isds_init() failed");
    
    struct isds_event *event = NULL;
    TEST("NULL", test_isds_event_free, NULL);
    TEST("*NULL", test_isds_event_free, &event);

    TEST_CALLOC(event);
    TEST("Empty structure", test_isds_event_free, &event);

    TEST_CALLOC(event);
    TEST_CALLOC(event->time);           /* When the event occurred */
    TEST_FILL_INT(event->type);
    TEST_FILL_STRING(event->description);
    TEST("Full structure", test_isds_event_free, &event);

    isds_cleanup();
    SUM_TEST();
}
