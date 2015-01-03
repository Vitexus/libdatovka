#include "../test.h"
#include "isds.h"

static int test_isds_commercial_permission_free(
        struct isds_commercial_permission **permission) {
    isds_commercial_permission_free(permission);
    if (!permission) PASS_TEST;

    if (*permission)
        FAIL_TEST("isds_commercial_permission_free() did not null pointer");

    PASS_TEST;
}


int main(void) {
    INIT_TEST("isds_commercial_permission_free()");
    if (isds_init())
        ABORT_UNIT("isds_init() failed");
    
    struct isds_commercial_permission *permission = NULL;
    TEST("NULL", test_isds_commercial_permission_free, NULL);
    TEST("*NULL", test_isds_commercial_permission_free, &permission);

    TEST_CALLOC(permission);
    TEST("Empty structure", test_isds_commercial_permission_free, &permission);

    TEST_CALLOC(permission);
    TEST_FILL_STRING(permission->recipient);
    TEST_FILL_STRING(permission->payer);
    TEST_CALLOC(permission->expiration);
    TEST_FILL_INT(permission->count);
    TEST_FILL_STRING(permission->reply_identifier);
    TEST("Full structure", test_isds_commercial_permission_free, &permission);

    isds_cleanup();
    SUM_TEST();
}
