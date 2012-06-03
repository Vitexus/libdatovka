#include "../test.h"
#include "isds.c"

static int test_timestring2timeval(const xmlChar *time, const isds_error error,
        const struct timeval *correct_timeval, struct timeval **new_timeval) {
    isds_error err;

    err = timestring2timeval(time, new_timeval);
    if (err != error)
        FAIL_TEST("timestring2timeval() returned unexpected code: "
                "expected=%s got=%s", isds_strerror(error), isds_strerror(err));

    if (err) {
        if (new_timeval && *new_timeval)
            FAIL_TEST("timestring2timeval() failed as exprected "
                    "but did not freed output timeval structure")
        else
            PASS_TEST;
    }

    if (!correct_timeval) {
        if (!new_timeval  || !*new_timeval)
            PASS_TEST
        else
            FAIL_TEST("timestring2timeval() did not returned NULL output "
                    "timeval struct as expected");
    }

    if (new_timeval && !*new_timeval)
        FAIL_TEST("timestring2timeval() freed output timeval struct "
                "unexpectedly");

    if (correct_timeval->tv_sec != (*new_timeval)->tv_sec)
        FAIL_TEST("Returned struct timeval differs in tv_sec: expected=%d, got=%d",
                correct_timeval->tv_sec, (*new_timeval)->tv_sec);
    if (correct_timeval->tv_usec != (*new_timeval)->tv_usec)
        FAIL_TEST("Returned struct timeval differs in tv_usec: expected=%d, got=%d",
                correct_timeval->tv_usec, (*new_timeval)->tv_usec);

    PASS_TEST;
}


int main(int argc, char **argv) {
    INIT_TEST("ISO date-time string to timeval conversion");

    /* Generic */
    struct timeval *output = NULL;
    char *input = "2001-02-03T04:05:06.123456+01:45";
    struct timeval time = {.tv_sec = 981166806, .tv_usec = 123456};
    TEST(input, test_timestring2timeval, BAD_CAST input, IE_SUCCESS, &time,
            &output);

    /* Negative time zone */
    input = "2001-02-03T04:05:06.123456-01:45";
    time.tv_sec = 981179406; time.tv_usec = 123456;
    TEST(input, test_timestring2timeval, BAD_CAST input, IE_SUCCESS, &time,
            &output);

    /* Shorten subseconds */
    input = "2001-02-03T04:05:06.01-01:45";
    time.tv_sec = 981179406; time.tv_usec = 10000;
    TEST(input, test_timestring2timeval, BAD_CAST input, IE_SUCCESS, &time,
            &output);

    /* No subseconds */
    input = "2001-02-03T04:05:06+01:45";
    time.tv_sec = 981166806; time.tv_usec = 0;
    TEST(input, test_timestring2timeval, BAD_CAST input, IE_SUCCESS, &time,
            &output);

    input = "2001-02-03T04:05:06-01:45";
    time.tv_sec = 981179406; time.tv_usec = 0;
    TEST(input, test_timestring2timeval, BAD_CAST input, IE_SUCCESS, &time,
            &output);

    input = "2001-02-03T04:05:06Z";
    time.tv_sec = 981173106; time.tv_usec = 0;
    TEST(input, test_timestring2timeval, BAD_CAST input, IE_SUCCESS, &time,
            &output);

    input = "2001-02-03T04:05:06";
    time.tv_sec = 981173106; time.tv_usec = 0;
    TEST(input, test_timestring2timeval, BAD_CAST input, IE_SUCCESS, &time,
            &output);

    /* Zulu with subseconds */
    input = "2001-02-03T04:05:06.123456+00:00";
    time.tv_sec = 981173106; time.tv_usec = 123456;
    TEST(input, test_timestring2timeval, BAD_CAST input, IE_SUCCESS, &time,
            &output);

    input = "2001-02-03T04:05:06.123456Z";
    time.tv_sec = 981173106; time.tv_usec = 123456;
    TEST(input, test_timestring2timeval, BAD_CAST input, IE_SUCCESS, &time,
            &output);

    input = "2001-02-03T04:05:06.123456";
    time.tv_sec = 981173106; time.tv_usec = 123456;
    TEST(input, test_timestring2timeval, BAD_CAST input, IE_SUCCESS, &time,
            &output);

    /* Invalid invocation */
    input = "foo bar";
    TEST(input, test_timestring2timeval, BAD_CAST input, IE_DATE, &time,
            &output);

    TEST("Empty input", test_timestring2timeval, BAD_CAST "", IE_DATE, &time,
            &output);

    TEST("NULL input", test_timestring2timeval, NULL, IE_INVAL, &time,
            &output);

    TEST("NULL output pointer", test_timestring2timeval, BAD_CAST "", IE_INVAL,
            NULL, NULL);

    SUM_TEST();
}
