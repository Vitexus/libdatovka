#include "../test.h"
#include "libdatovka/isds.h"

static int test_isds_approval_free(struct isds_approval **approval) {
    isds_approval_free(approval);
    if (!approval) PASS_TEST;

    if (*approval)
        FAIL_TEST("isds_approval_free() did not null pointer");

    PASS_TEST;
}


int main(void) {

    INIT_TEST("isds_approval_free()");
    if (isds_init())
        ABORT_UNIT("isds_init() failed");

    struct isds_approval *approval = NULL;

    TEST("NULL", test_isds_approval_free, NULL);
    TEST("*NULL", test_isds_approval_free, &approval);

    TEST_CALLOC(approval);
    TEST("Empty structure", test_isds_approval_free, &approval);

    TEST_CALLOC(approval);
    TEST_FILL_STRING(approval->reference);
    TEST("Full structure", test_isds_approval_free, &approval);

    isds_cleanup();
    SUM_TEST();
}
