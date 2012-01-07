#include "test.h"
#include "soap.c"

static int test_decode_header_value(const char *input,
        const char *correct_output) {
    char *decoded;

    decoded = decode_header_value(input);
    TEST_STRING_DUPLICITY(correct_output, decoded);
    PASS_TEST;
}

int main(int argc, char **argv) {
    int i;

    const char *inputs[] = {
        "foo",
        " foo ",
        "foo bar",
        "foo  bar",
        "foo\r\n bar",
        "foo\r\n\tbar",
        "foo \r\n bar",
    };
    const char *outputs[] = {
        "foo",
        "foo",
        "foo bar",
        "foo bar",
        "foo bar",
        "foo bar",
        "foo bar",
    };

    INIT_TEST("HTTP header value decoder");

    for (i = 0; i < sizeof(inputs)/sizeof(inputs[0]); i++) {
        TEST(inputs[i], test_decode_header_value, inputs[i], outputs[i]);
    }

    TEST("Empty string", test_decode_header_value, "", "");
    TEST("NULL pointer", test_decode_header_value, NULL, NULL);

    SUM_TEST();
}
