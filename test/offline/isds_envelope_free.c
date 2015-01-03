#include "../test.h"
#include "isds.h"

static int test_isds_envelope_free(
        struct isds_envelope **envelope) {
    isds_envelope_free(envelope);
    if (!envelope) PASS_TEST;

    if (*envelope)
        FAIL_TEST("isds_envelope_free() did not null pointer");

    PASS_TEST;
}


int main(void) {

    INIT_TEST("isds_envelope_free()");
    if (isds_init())
        ABORT_UNIT("isds_init() failed");
    
    struct isds_envelope *envelope = NULL;
    TEST("NULL", test_isds_envelope_free, NULL);
    TEST("*NULL", test_isds_envelope_free, &envelope);

    TEST_CALLOC(envelope);
    TEST("Empty structure", test_isds_envelope_free, &envelope);

    TEST_CALLOC(envelope);
    TEST_FILL_STRING(envelope->dmID);
    TEST_FILL_STRING(envelope->dbIDSender);
    TEST_FILL_STRING(envelope->dmSender);
    TEST_FILL_STRING(envelope->dmSenderAddress);
    TEST_FILL_INT(envelope->dmSenderType);
    TEST_FILL_STRING(envelope->dmRecipient);
    TEST_FILL_STRING(envelope->dmRecipientAddress);
    TEST_FILL_INT(envelope->dmAmbiguousRecipient);

    TEST_FILL_INT(envelope->dmOrdinal);
    TEST_FILL_INT(envelope->dmMessageStatus);
    TEST_FILL_INT(envelope->dmAttachmentSize);
    TEST_CALLOC(envelope->dmDeliveryTime);  /* Time of delivery into a box */
    TEST_CALLOC(envelope->dmAcceptanceTime);   /* Time of acceptance */
    TEST_CALLOC(envelope->hash);        /* Message hash */
    TEST_CALLOC(envelope->timestamp);       /* Qualified time stamp */
    TEST_CALLOC(envelope->events);      /* Events message passed trough */

    TEST_FILL_STRING(envelope->dmSenderOrgUnit);
    TEST_FILL_INT(envelope->dmSenderOrgUnitNum);
    TEST_FILL_STRING(envelope->dbIDRecipient);
    TEST_FILL_STRING(envelope->dmRecipientOrgUnit);
    TEST_FILL_INT(envelope->dmRecipientOrgUnitNum);

    TEST("Full structure", test_isds_envelope_free, &envelope);

    isds_cleanup();
    SUM_TEST();
}
