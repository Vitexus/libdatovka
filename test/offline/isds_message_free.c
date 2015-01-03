#include "../test.h"
#include "isds.h"

static int test_isds_message_free(struct isds_message **message) {
    isds_message_free(message);
    if (!message) PASS_TEST;

    if (*message)
        FAIL_TEST("isds_message_free() did not null pointer");

    PASS_TEST;
}


int main(void) {

    INIT_TEST("isds_message_free()");
    if (isds_init())
        ABORT_UNIT("isds_init() failed");
    
    struct isds_message *message = NULL;
    struct isds_document *document = NULL;
    xmlNode node = { .type = XML_TEXT_NODE, .content = BAD_CAST "data" };
    xmlDocPtr tree = xmlParseDoc(BAD_CAST "<root>data</root>");

    TEST("NULL", test_isds_message_free, NULL);
    TEST("*NULL", test_isds_message_free, &message);

    TEST_CALLOC(message);
    TEST("Empty structure", test_isds_message_free, &message);

    TEST_CALLOC(message);
    TEST_FILL_STRING(message->raw); 
    message->xml = NULL;    /* Parsed XML message */
    TEST_CALLOC(message->envelope);     /* Message envelope */
    TEST_CALLOC(message->documents);    /* List of isds_document's. */
    TEST("Message without XML documents", test_isds_message_free,
            &message);

    TEST_CALLOC(message);
    TEST_FILL_STRING(message->raw); 
    message->xml = NULL;    /* Parsed XML message */
    TEST_CALLOC(message->envelope);     /* Message envelope */
    TEST_CALLOC(message->documents);    /* List of isds_document's. */
    message->documents->destructor = (void (*)(void**))isds_document_free;
    TEST_CALLOC(document);
    document->is_xml = 1;
    document->xml_node_list = &node;
    message->documents->data = document;
    TEST("Message with XML document without XML tree", test_isds_message_free,
        &message);

    TEST_CALLOC(message);
    TEST_FILL_STRING(message->raw); 
    message->xml = tree;    /* Parsed XML message */
    TEST_CALLOC(message->envelope);     /* Message envelope */
    TEST_CALLOC(message->documents);    /* List of isds_document's. */
    message->documents->destructor = (void (*)(void**))isds_document_free;
    TEST_CALLOC(document);
    document->is_xml = 1;
    document->xml_node_list = tree->children;
    message->documents->data = document;
    TEST("Message with XML document with XML tree", test_isds_message_free,
        &message);

    isds_cleanup();
    SUM_TEST();
}
