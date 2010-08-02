#include "test.h"
#include "isds.h"

static int test_isds_message_free(struct isds_message **message) {
    isds_message_free(message);
    if (!message) PASS_TEST;

    if (*message)
        FAIL_TEST("isds_message_free() did not null pointer");

    PASS_TEST;
}


int main(int argc, char **argv) {

    INIT_TEST("isds_message_free()");
    if (isds_init())
        ABORT_UNIT("isds_init() failed");
    
    struct isds_message *message = NULL;
    TEST("NULL", test_isds_message_free, NULL);
    TEST("*NULL", test_isds_message_free, &message);

    TEST_CALLOC(message);
    TEST("Empty structure", test_isds_message_free, &message);

    TEST_CALLOC(message);
    TEST_FILL_STRING(message->raw); 
    message->xml = NULL;    /* Parsed XML message */
    TEST_CALLOC(message->envelope);     /* Message envelope */
    TEST_CALLOC(message->documents);    /* List of isds_document's. */
    TEST("Full message without XML documents", test_isds_message_free,
            &message);

    /* TODO: XML documents without XML tree,
     * TODO: XML documents with XML tree */

    isds_cleanup();
    SUM_TEST();
}
