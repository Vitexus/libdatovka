#include "../test.h"
#include "utils.h"
#include <locale.h>

static int prepare_locale(char type) {
    char *old_locale;

    if ('U' == type) {
        old_locale = setlocale(LC_ALL, "C.UTF-8");
        if (old_locale != NULL) return 0;

        old_locale = setlocale(LC_ALL, "en_US.UTF-8");
        if (old_locale != NULL) return 0;

        old_locale = setlocale(LC_ALL, "cs_CZ.UTF-8");
        if (old_locale != NULL) return 0;
    } else if ('2' == type) {
        old_locale = setlocale(LC_ALL, "cs_CZ.ISO8859-2");
        if (old_locale != NULL) return 0;
    } else {
        old_locale = setlocale(LC_ALL, "C");
        if (old_locale != NULL) return 0;
    }

    return -1;
}

static int test_utf82locale(const void *input, const void *correct) {
    void *output = NULL;

    output = _isds_utf82locale(input);
    TEST_DESTRUCTOR(free, output);

    TEST_STRING_DUPLICITY(correct, output);

    PASS_TEST;
}


int main(void) {
    INIT_TEST("utf8locale");

    if (prepare_locale('U')) {
        SKIP_TESTS(5, "Could not set any UTF-8 locale");
    } else {
        TEST("NULL input", test_utf82locale, NULL, NULL);
        TEST("Empty string", test_utf82locale, "", "");
        TEST("ASCII text", test_utf82locale, "lazy fox", "lazy fox");
        TEST("non-ASCII text in UTF-8 locale", test_utf82locale, "Šíleně žluťoučký",
                "Šíleně žluťoučký");
        TEST("OTP message in UTF-8 locale", test_utf82locale,
                "Jednorázový kód odeslán.", "Jednorázový kód odeslán.");
    }

    if (prepare_locale('2')) {
        SKIP_TESTS(1, "Could not set any ISO-8859-2 locale");
    } else {
        TEST("non-ASCII text in ISO-8859-2 locale", test_utf82locale,
                "Šíleně žluťoučký", "\xa9\xedlen\xec \xbelu\xbbou\xe8k\xfd");
    }

    if (prepare_locale('C')) {
        SKIP_TESTS(1, "Could not set C locale");
    } else {
        TEST("non-ASCII text in C locale", test_utf82locale, "Šíleně žluťoučký", NULL);
    }

    SUM_TEST();
}
