#include "../test.h"
#include "isds.c"

static void test_destructor(void *argument) {
    if (NULL != argument) zfree(*(void **)argument);
}

static int test_timeval2timestring(const struct isds_timeval* time,
        const isds_error error, const xmlChar *correct_string,
        xmlChar **new_string) {
    isds_error err;

    err = timeval2timestring(time, new_string);
    TEST_DESTRUCTOR(test_destructor, new_string);

    if (err != error)
        FAIL_TEST("timeval2timetring() returned unexpected code: "
                "expected=%s got=%s", isds_strerror(error), isds_strerror(err));

    if (err)
        PASS_TEST;

    if (NULL == new_string)
        PASS_TEST;

    if (NULL == correct_string && NULL == *new_string)
        PASS_TEST;

    if (NULL == correct_string || NULL == *new_string ||
            xmlStrcmp(correct_string, *new_string))
        FAIL_TEST("Wrong time string returned: expected=`%s', got=`%s'",
                correct_string, *new_string);

    PASS_TEST;
}

int main(void) {
    INIT_TEST("Struct isds_timeval to ISO time string conversion");

    xmlChar *output = NULL;
    xmlChar *time = BAD_CAST "2001-02-03T04:05:06.123456";
    struct isds_timeval input = {.tv_sec = 981173106, .tv_usec = 123456 };
    TEST("tv_sec=981173106 tv_usec=123456", test_timeval2timestring, &input,
            IE_SUCCESS, time, &output);
    input.tv_sec = 981173106; input.tv_usec = 12;
    TEST("tv_sec=981173106 tv_usec=12", test_timeval2timestring, &input,
            IE_SUCCESS, BAD_CAST "2001-02-03T04:05:06.000012", &output);

    TEST("NULL input", test_timeval2timestring, NULL,
            IE_INVAL, time, &output);
    TEST("NULL output pointer", test_timeval2timestring, &input,
            IE_INVAL, time, NULL);

    SUM_TEST();
}
