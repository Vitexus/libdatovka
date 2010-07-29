#include "test.h"
#include "isds.c"

static int test_eventstring2event(const xmlChar *string, isds_error error,
        const struct isds_event *correct_event, struct isds_event *new_event) {
    isds_error err;

    err = eventstring2event(string, new_event);

    if (error != err)
        FAIL_TEST("eventstring2event() returned wrong error code: "
                "expected=%s, got=%s",
                isds_strerror(error), isds_strerror(err));
    if (err != IE_SUCCESS)
        PASS_TEST;

    if (new_event && correct_event) {
        if (!correct_event->time && new_event->time)
            FAIL_TEST("Returned event time is not NULL as expected");
        if (correct_event->time) {
            if (!new_event->time)
                FAIL_TEST("Returned event time is NULL as not expected");
            if (correct_event->time->tv_sec != new_event->time->tv_sec)
                FAIL_TEST("Returned event time differs in tv_sec: "
                        "expected=%d, got=%d",
                        correct_event->time->tv_sec, new_event->time->tv_sec);
            if (correct_event->time->tv_usec != new_event->time->tv_usec)
                FAIL_TEST("Returned event time differs in tv_usec: "
                        "expected=%d, got=%d",
                        correct_event->time->tv_usec, new_event->time->tv_usec);
        }

        if (!correct_event->type && new_event->type)
            FAIL_TEST("Returned event type is not NULL as expected");
        if (correct_event->type) {
            if (!new_event->type)
                FAIL_TEST("Returned event type is NULL as not expected");
            if (*correct_event->type != *new_event->type)
                FAIL_TEST("Returned wrong event type: expected=%d, got=%d",
                        *correct_event->type, *new_event->type);
        }

        if (!correct_event->description && new_event->description)
            FAIL_TEST("Returned event description is not NUTLL as expected");
        if (correct_event->description) {
            if (!new_event->description)
                FAIL_TEST("Returned event description is NULL as "
                        "not expected");
            if (strcmp(correct_event->description, new_event->description))
                FAIL_TEST("Returned wrong event description: "
                        "expected=`%s', got=`%s'",
                        correct_event->description, new_event->description);
        }
    }

    PASS_TEST;
}


int main(int argc, char **argv) {
    INIT_TEST("eventstring to event conversion");
    
    xmlChar *input = NULL;
    isds_event_type event_type;
    struct isds_event event;
    struct isds_event output = {
        .time = NULL, .type = NULL, .description = NULL
    };


    isds_init();

    /* Known event prefixes */
    input = BAD_CAST "EV1: some text";
    event.time = NULL;
    event_type = EVENT_ACCEPTED_BY_RECIPIENT;
    event.type = &event_type;
    event.description = "some text";
    TEST((char*)input, test_eventstring2event, input,
            IE_SUCCESS, &event, &output);

    input = BAD_CAST "EV2: some text";
    event.time = NULL;
    event_type = EVENT_ACCEPTED_BY_FICTION;
    event.type = &event_type;
    event.description = "some text";
    TEST((char*)input, test_eventstring2event, input,
            IE_SUCCESS, &event, &output);

    input = BAD_CAST "EV3: some text";
    event.time = NULL;
    event_type = EVENT_UNDELIVERABLE;
    event.type = &event_type;
    event.description = "some text";
    TEST((char*)input, test_eventstring2event, input,
            IE_SUCCESS, &event, &output);

    input = BAD_CAST "EV4: some text";
    event.time = NULL;
    event_type = EVENT_COMMERCIAL_ACCEPTED;
    event.type = &event_type;
    event.description = "some text";
    TEST((char*)input, test_eventstring2event, input,
            IE_SUCCESS, &event, &output);

    /* Unknown event prefixes */
    input = BAD_CAST "EV0: out of range event";
    event.time = NULL;
    event_type = EVENT_UKNOWN;
    event.type = &event_type;
    event.description = (char *) input;
    TEST((char*)input, test_eventstring2event, input,
            IE_SUCCESS, &event, &output);

    input = BAD_CAST "EV5: out of range event";
    event.time = NULL;
    event_type = EVENT_UKNOWN;
    event.type = &event_type;
    event.description = (char *) input;
    TEST((char*)input, test_eventstring2event, input,
            IE_SUCCESS, &event, &output);

    /* Other valid cases */
    event.time = NULL;
    event_type = EVENT_UKNOWN;
    event.type = &event_type;
    event.description = "";
    TEST("Empty input", test_eventstring2event, BAD_CAST "",
            IE_SUCCESS, &event, &output);

    /* Invalid invocations */
    TEST("NULL input", test_eventstring2event, NULL,
            IE_INVAL, NULL, &output);

    input = BAD_CAST "EV1: some text";
    event.time = NULL;
    event_type = EVENT_ACCEPTED_BY_RECIPIENT;
    event.type = &event_type;
    event.description = "some text";
    TEST("NULL output", test_eventstring2event, input,
            IE_INVAL, &event, NULL);

    isds_cleanup();
    SUM_TEST();
}
