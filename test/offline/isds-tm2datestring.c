#include "../test.h"
#include "isds.c"

static void test_destructor(void *string) {
    if (NULL != string) zfree(*(void **)string);
}

static int test_tm2datestring(const struct tm* date, const isds_error error,
        const xmlChar *correct_string, xmlChar **new_string) {
    isds_error err;

    err = tm2datestring(date, new_string);
    TEST_DESTRUCTOR(test_destructor, new_string);

    if (err != error)
        FAIL_TEST("tm2datestring() returned unexpected code: "
                "expected=%s got=%s", isds_strerror(error), isds_strerror(err));

    if (err)
        PASS_TEST;

    if (NULL == new_string)
        PASS_TEST;

    if (NULL == *new_string && NULL != correct_string)
        PASS_TEST;

    if (NULL == correct_string || NULL == *new_string ||
            xmlStrcmp(correct_string, *new_string))
        FAIL_TEST("Wrong date string returned: expected=`%s', got=`%s'",
                correct_string, *new_string);

    PASS_TEST;
}

int main(int argc, char **argv) {
    INIT_TEST("Struct tm to ISO date string conversion");

    xmlChar *output = NULL;
    xmlChar *date = BAD_CAST "2001-02-03";
    struct tm input = {.tm_year = 101, .tm_mon = 1, .tm_mday = 3};
    TEST("tm_year=101 tm_mon=1 tm_mday=3", test_tm2datestring, &input,
            IE_SUCCESS, date, &output);

    TEST("NULL input", test_tm2datestring, NULL,
            IE_INVAL, date, &output);
    TEST("NULL output pointer", test_tm2datestring, &input,
            IE_INVAL, date, NULL);

    SUM_TEST();
}
