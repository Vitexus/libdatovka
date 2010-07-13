#include "test.h"
#include "utils.h"
#include <string.h>

static int test_b64decode(const void *input, const void *correct,
        size_t correct_length) {
    void *output = NULL;
    size_t length;

    length = _isds_b64decode(input, &output);

    if (length == correct_length) {
        if (length != -1) {
            if (!memcmp(correct, output, length)) {
                free(output);
                PASS_TEST;
            } else {
                free(output);
                FAIL_TEST("Output length matches, but content differs");
            }
        } else {
            free(output);

            if (output == NULL) {
                PASS_TEST;
            } else {
                FAIL_TEST("Output length signals error as expected, "
                        "but content has not been deallocated");
            }
        }
    } else {
        free(output);
        /* Format as signed to get -1 as error. Big positive numbers should
         * not occure in these tests */
        FAIL_TEST("Output length differs: expected=%zd, got=%zd",
                correct_length, length);
    }

    free(output);
    FAIL_TEST("Test could not been judged -- Internal test error");
}


int main(int argc, char **argv) {
    INIT_TEST("b64decode");

    TEST("generic with new line", test_b64decode, "NDIA\n", "42", 3);
    TEST("generic without new line", test_b64decode, "NDIA", "42", 3);
    TEST("new line only", test_b64decode, "\n", NULL, 0);
    TEST("empty string", test_b64decode, "", NULL, 0);
    TEST("invalid input", test_b64decode, "42", NULL, (size_t) -1);

    SUM_TEST();
}
