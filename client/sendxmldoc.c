#define _XOPEN_SOURCE 500
#include <stdlib.h>
#include <stdio.h>
#include <locale.h>
#include <time.h>
#include <string.h>
#include <libgen.h>
#include <isds.h>
#include <libxml/parser.h>
#include <libxml/xpath.h>
#include <libxml/debugXML.h>
#include "common.h"


int main(int argc, char **argv) {
    struct isds_ctx *ctx = NULL;
    isds_error err;
    char *recipient = NULL;
    xmlDocPtr xml = NULL;
    xmlNodePtr node_list = NULL;
    
    setlocale(LC_ALL, "");

    err = isds_init();
    if (err) {
        printf("isds_init() failed: %s\n", isds_strerror(err));
        exit(EXIT_FAILURE);
    }

    isds_set_logging(ILF_ALL & ~ILF_HTTP, ILL_ALL);

    ctx = isds_ctx_create();
    if (!ctx) {
        printf("isds_ctx_create() failed");
    }

    err = isds_set_timeout(ctx, 10000);
    if (err) {
        printf("isds_set_timeout() failed: %s\n", isds_strerror(err));
    }

    err = isds_login(ctx, url, username, password, NULL);
    if (err) {
        printf("isds_login() failed: %s: %s\n", isds_strerror(err),
                isds_long_message(ctx));
    } else {
        printf("Logged in :)\n");
    }


    if (argv[1] && argv[1][0]) {
        recipient = strdup(argv[1]);
    } else {
        /* Find a recipient */
        struct isds_list *boxes = NULL;
        struct isds_DbOwnerInfo criteria;
        isds_DbType criteria_db_type = DBTYPE_OVM;
        memset(&criteria, 0, sizeof(criteria));
        criteria.firmName = "Místní";
        criteria.dbType = &criteria_db_type;

        printf("Searching box with firm name `%s':\n", criteria.firmName);
        err = isds_FindDataBox(ctx, &criteria, &boxes);
        if (err == IE_SUCCESS || err == IE_2BIG) {
            printf("isds_FindDataBox() succeeded:\n");

            if (boxes && boxes->data) {
                printf("Selected recipient:\n");
                print_DbOwnerInfo(boxes->data);
                recipient = strdup(
                    ((struct isds_DbOwnerInfo *)(boxes->data))->dbID);
            }
        } else {
            printf("isds_FindDataBox() failed: %s: %s\n",
                    isds_strerror(err), isds_long_message(ctx));
        }

        isds_list_free(&boxes);
    }


    {
        xmlXPathContextPtr xpath_ctx = NULL;
        xmlXPathObjectPtr result = NULL;
        xmlNodePtr node = NULL, prev_node = NULL;

        /* Get XML node list */
        if (argc != 4) {
            printf("Bad number of arguments\n");
            printf("Usage: %s RECIPIENT XML_FILE XPATH_EXPRESSION\n"
"Send a message with XML document defined by XPATH_EXPRESSION on XML_FILE\n"
"to RECIPIENT. If RECIPIENT is empty, send to random foubd one.",
            basename(argv[0]));
            exit(EXIT_FAILURE);
        }

        xml = xmlParseFile(argv[2]);
        if (!xml) {
            printf("Error while parsing `%s'\n", argv[1]);
            exit(EXIT_FAILURE);
        }

        xpath_ctx = xmlXPathNewContext(xml);
        if (!xpath_ctx) {
            printf("Error while creating XPath context\n");
            exit(EXIT_FAILURE);
        }
        result = xmlXPathEvalExpression(BAD_CAST argv[3], xpath_ctx);
        if (!result) {
            printf("Error while evaluating XPath expression `%s'\n", argv[3]);
            exit(EXIT_FAILURE);
        }
        if (xmlXPathNodeSetIsEmpty(result->nodesetval)) {
            printf("Empty match, trying to send empty XML document\n");
            node_list = NULL;
        } else {
            /* Convert node set to list of siblings */
            for (int i = 0; i < result->nodesetval->nodeNr; i++) {
                /* Make weak copy of the node */
                node = malloc(sizeof(*node));
                if (!node) {
                    fprintf(stderr, "Not enoungh memory\n");
                    exit (EXIT_FAILURE);
                }
                memcpy(node, result->nodesetval->nodeTab[i], sizeof(*node));

                /* Add node to node_list */
                node->prev = prev_node;
                node->next = NULL;
                if (prev_node)
                    prev_node->next = node;
                else
                    node_list = node;
                prev_node = node;

                /* Debug */
                printf("* Embeding node #%d:\n", i);
                xmlDebugDumpNode(stdout, node, 2);
            }

        }
        xmlXPathFreeObject(result);
        xmlXPathFreeContext(xpath_ctx);

    }


    {
        /* Send one message */
        struct isds_message message;
        memset(&message, 0, sizeof(message));

        struct isds_envelope envelope;
        memset(&envelope, 0, sizeof(envelope));
        message.envelope = &envelope;
        long int dmSenderOrgUnitNum = 42;
        envelope.dmSenderOrgUnitNum = &dmSenderOrgUnitNum;
        envelope.dmAnnotation = "XML documents";
        envelope.dbIDRecipient = recipient;

       
        struct isds_document main_document;
        memset(&main_document, 0, sizeof(main_document));
        main_document.is_xml = 0;
        main_document.data = "Hello World!";
        main_document.data_length = strlen(main_document.data);
        main_document.dmMimeType = "text/plain";
        main_document.dmFileMetaType = FILEMETATYPE_MAIN;
        main_document.dmFileDescr = "standard_text.txt";
        main_document.dmFileGuid = "1";

        struct isds_document xml_document;
        memset(&xml_document, 0, sizeof(xml_document));
        xml_document.is_xml = 1;
        xml_document.xml_node_list = node_list;
        xml_document.dmMimeType = "text/xml";
        xml_document.dmFileMetaType = FILEMETATYPE_ENCLOSURE;
        xml_document.dmFileDescr = "in-line.xml";
        xml_document.dmFileGuid = "2";

        struct isds_list documents_xml_item = {
            .data = &xml_document,
            .next = NULL,
            .destructor = NULL
        };
        struct isds_list documents_main_item = {
            .data = &main_document,
            .next = &documents_xml_item,
            .destructor = NULL
        };
        message.documents = &documents_main_item;


        printf("Sending message to box ID `%s'\n", recipient);
        err = isds_send_message(ctx, &message);
        
        if (err == IE_SUCCESS){
            printf("isds_send_message() succeeded: message ID = %s\n",
                    message.envelope->dmID);
        } else 
            printf("isds_send_message() failed: "
                    "%s: %s\n", isds_strerror(err), isds_long_message(ctx));

    }

    for (xmlNodePtr node = node_list; node; node = node->next) free(node);
    free(recipient);
    xmlFreeDoc(xml);


    err = isds_logout(ctx);
    if (err) {
        printf("isds_logout() failed: %s\n", isds_strerror(err));
    }


    err = isds_ctx_free(&ctx);
    if (err) {
        printf("isds_ctx_free() failed: %s\n", isds_strerror(err));
    }


    err = isds_cleanup();
    if (err) {
        printf("isds_cleanup() failed: %s\n", isds_strerror(err));
    }

    exit (EXIT_SUCCESS);
}
