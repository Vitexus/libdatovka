#include "../test.h"
#include "isds.h"

static int test_isds_message_copy_free(struct isds_message_copy **copy) {
    isds_message_copy_free(copy);
    if (!copy) PASS_TEST;

    if (*copy)
        FAIL_TEST("isds_message_copy_free() did not null pointer");

    PASS_TEST;
}


int main(void) {

    INIT_TEST("isds_message_copy_free()");
    if (isds_init())
        ABORT_UNIT("isds_init() failed");
    
    struct isds_message_copy *copy = NULL;

    TEST("NULL", test_isds_message_copy_free, NULL);
    TEST("*NULL", test_isds_message_copy_free, &copy);

    TEST_CALLOC(copy);
    TEST("Empty structure", test_isds_message_copy_free, &copy);

    TEST_CALLOC(copy);
    TEST_FILL_STRING(copy->dbIDRecipient);
    TEST_FILL_STRING(copy->dmRecipientOrgUnit);
    TEST_FILL_INT(copy->dmRecipientOrgUnitNum);
    TEST_FILL_STRING(copy->dmToHands);
    TEST_FILL_STRING(copy->dmStatus);
    TEST_FILL_STRING(copy->dmID);
    TEST("Full structure", test_isds_message_copy_free, &copy);

    isds_cleanup();
    SUM_TEST();
}
