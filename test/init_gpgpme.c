#include "test.h"
#include "crypto.h"

static int test_init_gpgme(const isds_error error) {
    isds_error err;

    err = _isds_init_gpgme(NULL);
    if (err != error) 
        FAIL_TEST("Wrong return value");

    PASS_TEST;
}

int main(int argc, char **argv) {

    INIT_TEST("init_gpgme");

    TEST("initialization", test_init_gpgme, IE_SUCCESS);

    SUM_TEST();
}
