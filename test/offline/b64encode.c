#include "../test.h"
#include "utils.h"
#include <string.h>

static int test_b64encode(const char *correct, const void *input,
        size_t length) {
    char *output = NULL;

    output = _isds_b64encode(input, length);

    if (correct == NULL && output == NULL)
        PASS_TEST;

    if (correct != NULL && output == NULL)
        FAIL_TEST("Excpected non-NULL, got NULL");

    if (correct == NULL && output != NULL) {
        free(output);
        FAIL_TEST("Excpected NULL, got non-NULL");
    }

    if (strcmp(correct, output)) {
        FAILURE_REASON("Wrong return value: expected=`%s', got=`%s'",
                correct, output);
        free(output);
        return 1;
    }

    free(output);
    PASS_TEST;
}


int main(void) {
    INIT_TEST("b64encode");

    TEST("generic", test_b64encode, "Af+qVQA=\n", "\x1\xff\xaa\x55", 5);
    TEST("partial cycle", test_b64encode, "MQA=\n", "1", 2);
    TEST("1 cycle", test_b64encode, "NDIA\n", "42", 3);
    TEST("2 cycles", test_b64encode, "MTIzNDUA\n", "12345", 6);
    TEST("empty string", test_b64encode, "AA==\n", "", 1);
    TEST("NULL input, 0 length", test_b64encode, "\n", NULL, 0);
    TEST("non-NULL input, 0 length", test_b64encode, "\n", "", 0);
    TEST("NULL input, non-zero length", test_b64encode, NULL, NULL, 1);

    SUM_TEST();
}
