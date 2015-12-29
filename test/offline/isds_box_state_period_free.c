#include "../test.h"
#include "isds.h"

static int test_isds_box_state_period_free(
        struct isds_box_state_period **period) {
    isds_box_state_period_free(period);
    if (!period) PASS_TEST;

    if (*period)
        FAIL_TEST("isds_box_state_period_free() did not null pointer");

    PASS_TEST;
}


int main(void) {

    INIT_TEST("isds_box_state_period_free()");
    if (isds_init())
        ABORT_UNIT("isds_init() failed");

    struct isds_box_state_period *period = NULL;
    TEST("NULL", test_isds_box_state_period_free, NULL);
    TEST("*NULL", test_isds_box_state_period_free, &period);

    TEST_CALLOC(period);
    TEST("Empty structure", test_isds_box_state_period_free, &period);

    /* Full structure */
    TEST_CALLOC(period);
    TEST("Full structure", test_isds_box_state_period_free, &period);

    isds_cleanup();
    SUM_TEST();
}
