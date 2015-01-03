#include "../test.h"
#include "isds.h"
#include <string.h>

static int test_isds_PersonName_duplicate(struct isds_PersonName *origin) {
    struct isds_PersonName *copy = isds_PersonName_duplicate(origin);
    TEST_DESTRUCTOR((void(*)(void *))isds_PersonName_free, &copy);

    if (!origin) {
        if (copy) 
            FAIL_TEST("Duplicate of NULL should be NULL");
        PASS_TEST;
    }

    if (!copy)
        FAIL_TEST("isds_PersonName_duplicate() returned NULL instead of "
                "pointer to copy");

    TEST_STRING_DUPLICITY(origin->pnFirstName, copy->pnFirstName);
    TEST_STRING_DUPLICITY(origin->pnMiddleName, copy->pnMiddleName);
    TEST_STRING_DUPLICITY(origin->pnLastName, copy->pnLastName);
    TEST_STRING_DUPLICITY(origin->pnLastNameAtBirth, copy->pnLastNameAtBirth);

    PASS_TEST;
}


int main(void) {

    INIT_TEST("isds_PersonName_duplicate()");
    if (isds_init())
        ABORT_UNIT("isds_init() failed");
    
    TEST("NULL", test_isds_PersonName_duplicate, NULL);

    struct isds_PersonName empty;
    memset(&empty, 0, sizeof(empty));
    TEST("Empty structure", test_isds_PersonName_duplicate, &empty);

    /* Full structure */
    struct isds_PersonName full = {
        .pnFirstName = "1",
        .pnMiddleName = "2",
        .pnLastName = "3",
        .pnLastNameAtBirth = "4"
    };

    TEST("Full structure", test_isds_PersonName_duplicate, &full);

    isds_cleanup();
    SUM_TEST();
}
