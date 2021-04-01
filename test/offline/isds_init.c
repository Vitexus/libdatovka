#include "../test.h"
#include "libdatovka/isds.h"

static int test_init(const isds_error error) {
    isds_error err;

    err = isds_init();
    if (err != error)
        FAIL_TEST("Wrong return value");

    PASS_TEST;
}

static int test_cleanup(const isds_error error) {
    isds_error err;

    err = isds_cleanup();
    if (err != error)
        FAIL_TEST("Wrong return value");

    PASS_TEST;
}

static int test_reinit(void) {
    if (isds_init()) {
        isds_cleanup();
        FAIL_TEST("isds_init() failed");
    }
    if (isds_cleanup())
        FAIL_TEST("isds_cleanup() failed");
    PASS_TEST;
}

int main(void) {

    INIT_TEST("isds_init");

    TEST("first initialization", test_init, IE_SUCCESS);
    TEST("first clean-up", test_cleanup, IE_SUCCESS);

    TEST("reinitialization", test_reinit);

    SUM_TEST();
}
