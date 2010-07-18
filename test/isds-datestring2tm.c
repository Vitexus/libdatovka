#include "test.h"
#include "isds.c"

static int test_datestring2tm(const xmlChar *date, const isds_error error,
        const struct tm *correct_tm, struct tm *new_tm) {
    isds_error err;

    err = datestring2tm(date, new_tm);
    if (err != error)
        FAIL_TEST("datestring2tm() returned unexpected code: "
                "expected=%s got=%s", isds_strerror(error), isds_strerror(err));

    if (err)
        PASS_TEST;

    if (correct_tm->tm_year != new_tm->tm_year)
        FAIL_TEST("Returned struct tm differs in tm_year: expected=%d, got=%d",
                correct_tm->tm_year, new_tm->tm_year);
    if (correct_tm->tm_mon != new_tm->tm_mon)
        FAIL_TEST("Returned struct tm differs in tm_mon: expected=%d, got=%d",
                correct_tm->tm_mon, new_tm->tm_mon);
    if (correct_tm->tm_mday != new_tm->tm_mday)
        FAIL_TEST("Returned struct tm differs in tm_mday: expected=%d, got=%d",
                correct_tm->tm_mday, new_tm->tm_mday);

    PASS_TEST;
}

int main(int argc, char **argv) {
    INIT_TEST("ISO date string to tm conversion");

    struct tm output;
    char *input = "2001-02-03";
    struct tm date = {.tm_year = 101, .tm_mon = 1, .tm_mday = 3};
    TEST(input, test_datestring2tm, BAD_CAST input, IE_SUCCESS, &date, &output);

    input = "20010203";
    TEST(input, test_datestring2tm, BAD_CAST input, IE_SUCCESS, &date, &output);

    input = "2001-34";
    TEST(input, test_datestring2tm, BAD_CAST input, IE_SUCCESS, &date, &output);


    input = "2001-02-03T05:06";
    TEST(input, test_datestring2tm, BAD_CAST input, IE_NOTSUP, &date, &output);

    input = "foo bar";
    TEST(input, test_datestring2tm, BAD_CAST input, IE_NOTSUP, &date, &output);

    TEST("Empty input", test_datestring2tm, BAD_CAST "", IE_NOTSUP, &date,
            &output);

    input = NULL;
    TEST(input, test_datestring2tm, BAD_CAST input, IE_INVAL, &date, &output);

    TEST("NULL output pointer", test_datestring2tm, BAD_CAST "", IE_INVAL,
            NULL, NULL);

    SUM_TEST();
}
