#include "../test.h"
#include "libdatovka/isds.h"

static int test_isds_credentials_delivery_free(
        struct isds_credentials_delivery **credentials_delivery) {
    isds_credentials_delivery_free(credentials_delivery);
    if (!credentials_delivery) PASS_TEST;

    if (*credentials_delivery)
        FAIL_TEST("isds_credentials_delivery_free() did not null pointer");

    PASS_TEST;
}


int main(void) {

    INIT_TEST("isds_credentials_delivery_free()");
    if (isds_init())
        ABORT_UNIT("isds_init() failed");
    
    struct isds_credentials_delivery *credentials_delivery = NULL;

    TEST("NULL", test_isds_credentials_delivery_free, NULL);
    TEST("*NULL", test_isds_credentials_delivery_free, &credentials_delivery);

    TEST_CALLOC(credentials_delivery);
    TEST("Empty structure", test_isds_credentials_delivery_free,
            &credentials_delivery);

    TEST_CALLOC(credentials_delivery);
    TEST_FILL_STRING(credentials_delivery->email);
    TEST_FILL_STRING(credentials_delivery->token);
    TEST_FILL_STRING(credentials_delivery->new_user_name);
    TEST("Full structure", test_isds_credentials_delivery_free,
            &credentials_delivery);

    isds_cleanup();
    SUM_TEST();
}
