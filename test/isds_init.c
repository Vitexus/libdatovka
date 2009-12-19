#include "test.h"
#include "isds.h"

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

int main(int argc, char **argv) {

    INIT_TEST("isds_init");

    TEST("first initialization", test_init, IE_SUCCESS);
    TEST("first cleanup", test_cleanup, IE_SUCCESS);

    SUM_TEST();
}
