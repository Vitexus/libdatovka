#include "../test.h"
#include "libdatovka/isds.h"

static int test_isds_pki_credentials_free(
        struct isds_pki_credentials **credentials) {
    isds_pki_credentials_free(credentials);
    if (!credentials) PASS_TEST;

    if (*credentials)
        FAIL_TEST("isds_pki_credentials_free() did not null pointer");

    PASS_TEST;
}


int main(void) {

    INIT_TEST("isds_pki_credentials_free()");
    if (isds_init())
        ABORT_UNIT("isds_init() failed");

    struct isds_pki_credentials *credentials = NULL;
    TEST("NULL", test_isds_pki_credentials_free, NULL);
    TEST("*NULL", test_isds_pki_credentials_free, &credentials);

    TEST_CALLOC(credentials);
    TEST("Empty structure", test_isds_pki_credentials_free, &credentials);

    TEST_CALLOC(credentials);
    TEST_FILL_STRING(credentials->engine);
    TEST_FILL_STRING(credentials->certificate);
    TEST_FILL_STRING(credentials->key);
    TEST_FILL_STRING(credentials->passphrase)
    TEST("Full structure", test_isds_pki_credentials_free, &credentials);

    isds_cleanup();
    SUM_TEST();
}
