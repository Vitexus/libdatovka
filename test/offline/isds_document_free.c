#include "../test.h"
#include "libdatovka/isds.h"

static int test_isds_document_free(
        struct isds_document **document) {
    isds_document_free(document);
    if (!document) PASS_TEST;

    if (*document)
        FAIL_TEST("isds_document_free() did not null pointer");

    PASS_TEST;
}


int main(void) {
    xmlNode node = { .type = XML_TEXT_NODE, .content = BAD_CAST "data" };

    INIT_TEST("isds_document_free()");
    if (isds_init())
        ABORT_UNIT("isds_init() failed");
    
    struct isds_document *document = NULL;
    TEST("NULL", test_isds_document_free, NULL);
    TEST("*NULL", test_isds_document_free, &document);

    TEST_CALLOC(document);
    TEST("Empty structure", test_isds_document_free, &document);

    TEST_CALLOC(document);
    document->is_xml = 0;
    TEST_FILL_STRING(document->data);
    TEST_FILL_STRING(document->dmMimeType);
    TEST_FILL_STRING(document->dmFileGuid);
    TEST_FILL_STRING(document->dmUpFileGuid);
    TEST_FILL_STRING(document->dmFileDescr);
    TEST_FILL_STRING(document->dmFormat);
    TEST("Binary document", test_isds_document_free, &document);

    TEST_CALLOC(document);
    document->is_xml = 1;
    document->xml_node_list = &node;
    TEST_FILL_STRING(document->dmMimeType);
    TEST_FILL_STRING(document->dmFileGuid);
    TEST_FILL_STRING(document->dmUpFileGuid);
    TEST_FILL_STRING(document->dmFileDescr);
    TEST_FILL_STRING(document->dmFormat);
    TEST("XML document", test_isds_document_free, &document);

    isds_cleanup();
    SUM_TEST();
}
