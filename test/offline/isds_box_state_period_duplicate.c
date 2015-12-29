#include "../test.h"
#include "isds.h"
#include <string.h>

static int test_isds_box_state_period_duplicate(struct isds_box_state_period *origin) {
    struct isds_box_state_period *copy = isds_box_state_period_duplicate(origin);
    TEST_DESTRUCTOR((void (*)(void *))isds_box_state_period_free, &copy);

    if (!origin) {
        if (copy)
            FAIL_TEST("Duplicate of NULL should be NULL");
        PASS_TEST;
    }

    if (!copy)
        FAIL_TEST("isds_box_state_period_duplicate() returned NULL instead of "
                "pointer to copy");

    TEST_TIMEVALPTR_DUPLICITY(&origin->from, &copy->from);
    TEST_TIMEVALPTR_DUPLICITY(&origin->to, &copy->to);
    TEST_INT_DUPLICITY(origin->dbState, copy->dbState);

    PASS_TEST;
}


int main(void) {
    INIT_TEST("isds_box_state_period_duplicate()");
    if (isds_init())
        ABORT_UNIT("isds_init() failed");

    TEST("NULL", test_isds_box_state_period_duplicate, NULL);

    struct isds_box_state_period empty;
    memset(&empty, 0, sizeof(empty));
    TEST("Empty structure", test_isds_box_state_period_duplicate, &empty);

    /* Full structure */
    struct isds_box_state_period full = {
        .from = {
            .tv_sec = 1,
            .tv_usec = 2
        },
        .to = {
            .tv_sec = 3,
            .tv_usec = 4
        },
        .dbState = 42
    };
    TEST("Full structure", test_isds_box_state_period_duplicate, &full);

    isds_cleanup();
    SUM_TEST();
}
