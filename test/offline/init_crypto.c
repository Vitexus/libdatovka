#include "../test.h"
#include "crypto.h"

static int test_init_crypto(const isds_error error) {
    isds_error err;

    err = _isds_init_crypto();
    if (err != error) 
        FAIL_TEST("Wrong return value");

    PASS_TEST;
}

int main(int argc, char **argv) {

    INIT_TEST("init_gpgme");

    TEST("initialization", test_init_crypto, IE_SUCCESS);

    SUM_TEST();
}
