#include "../test.h"
#include "isds.c"

static int test_timeval2timestring(const struct timeval* time,
        const isds_error error, const xmlChar *correct_string,
        xmlChar **new_string) {
    isds_error err;

    err = timeval2timestring(time, new_string);
    if (err != error) {
        if (new_string) zfree(*new_string);
        FAIL_TEST("timeval2timetring() returned unexpected code: "
                "expected=%s got=%s", isds_strerror(error), isds_strerror(err));
    }

    if (err) {
        if (new_string) zfree(*new_string);
        PASS_TEST;
    }

    if (!*new_string && !correct_string)
        PASS_TEST;

    if (!correct_string || !*new_string ||
            xmlStrcmp(correct_string, *new_string)) {
        FAILURE_REASON("Wrong time string returned: expected=`%s', got=`%s'",
                correct_string, *new_string);
        if (new_string) zfree(*new_string);
        return 1;
    }

    if (new_string) zfree(*new_string);
    PASS_TEST;
}

int main(int argc, char **argv) {
    INIT_TEST("Struct timeval to ISO time string conversion");

    xmlChar *output = NULL;
    xmlChar *time = BAD_CAST "2001-02-03T04:05:06.123456";
    struct timeval input = {.tv_sec = 981173106, .tv_usec = 123456 };
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
