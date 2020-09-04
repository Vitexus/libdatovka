#include "../test.h"
#include "isds.h"
#include <string.h>

static int test_isds_PersonName2_duplicate(struct isds_PersonName2 *origin) {
    struct isds_PersonName2 *copy = isds_PersonName2_duplicate(origin);
    TEST_DESTRUCTOR((void(*)(void *))isds_PersonName2_free, &copy);

    if (!origin) {
        if (copy)
            FAIL_TEST("Duplicate of NULL should be NULL");
        PASS_TEST;
    }

    if (!copy)
        FAIL_TEST("isds_PersonName2_duplicate() returned NULL instead of "
                "pointer to copy");

    TEST_STRING_DUPLICITY(origin->pnGivenNames, copy->pnGivenNames);
    TEST_STRING_DUPLICITY(origin->pnLastName, copy->pnLastName);

    PASS_TEST;
}


int main(void) {

    INIT_TEST("isds_PersonName2_duplicate()");
    if (isds_init())
        ABORT_UNIT("isds_init() failed");

    TEST("NULL", test_isds_PersonName2_duplicate, NULL);

    struct isds_PersonName2 empty;
    memset(&empty, 0, sizeof(empty));
    TEST("Empty structure", test_isds_PersonName2_duplicate, &empty);

    /* Full structure */
    struct isds_PersonName2 full = {
        .pnGivenNames = "1",
        .pnLastName = "2"
    };

    TEST("Full structure", test_isds_PersonName2_duplicate, &full);

    isds_cleanup();
    SUM_TEST();
}
