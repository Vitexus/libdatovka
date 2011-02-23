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


/* @node_list is pointer to by-function allocated weak copy of libxml node
 * pointers list. *NULL means empty list. */
int xpath2nodelist(xmlNodePtr *node_list, xmlXPathContextPtr xpath_ctx, const xmlChar *xpath_expr) {
    xmlXPathObjectPtr result = NULL;
    xmlNodePtr node = NULL, prev_node = NULL;

    if (!node_list || !xpath_ctx || !xpath_expr) return -1;

    result = xmlXPathEvalExpression(xpath_expr, xpath_ctx);
    if (!result) {
        printf("Error while evaluating XPath expression `%s'\n", xpath_expr);
        return -1;
    }

    if (xmlXPathNodeSetIsEmpty(result->nodesetval)) {
        printf("Empty match, returning empty node list\n");
        *node_list = NULL;
    } else {
        /* Convert node set to list of siblings */
        for (int i = 0; i < result->nodesetval->nodeNr; i++) {
            /* Make weak copy of the node */
            node = malloc(sizeof(*node));
            if (!node) {
                fprintf(stderr, "Not enoungh memory\n");
                xmlXPathFreeObject(result);
                for (node = *node_list; node; node = node->next)
                    free(node);
                *node_list = NULL;
                return -1;
            }
            memcpy(node, result->nodesetval->nodeTab[i], sizeof(*node));

            /* Add node to node_list */
            node->prev = prev_node;
            node->next = NULL;
            if (prev_node)
                prev_node->next = node;
            else
                *node_list = node;
            prev_node = node;

            /* Debug */
            printf("* Embeding node #%d:\n", i);
            xmlDebugDumpNode(stdout, node, 2);
        }

    }

    xmlXPathFreeObject(result);
    return 0;
}


int main(int argc, char **argv) {
    struct isds_ctx *ctx = NULL;
    isds_error err;
    char *recipient = NULL;
    xmlDocPtr xml = NULL;
    struct isds_list *documents = NULL;
    
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

    err = isds_login(ctx, url, username(), password(), NULL);
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
        /* Create XML documents */
        xmlXPathContextPtr xpath_ctx = NULL;

        struct isds_document *document;
        struct isds_list *documents_item, *prev_documents_item = NULL;

        if (argc < 4) {
            printf("Bad number of arguments\n");
            printf("Usage: %s RECIPIENT XML_FILE XPATH_EXPRESSION...\n"
"Send a message with XML document defined by XPATH_EXPRESSION on XML_FILE\n"
"to RECIPIENT. If RECIPIENT is empty, send to random foubd one. If more\n"
"XPATH_EXPRESSIONS are specified creates XML document for each of them.\n",
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

        for (int j = 3; j < argc; j++) {
            printf("** Building XML document #%d:\n", j);

            document = calloc(1, sizeof(*document));
            if (!document) {
                printf("Not enough memory\n");
                exit(EXIT_FAILURE);
            }
            document->is_xml = 1;
            document->dmMimeType = "text/xml";
            if (prev_documents_item)
                document->dmFileMetaType = FILEMETATYPE_ENCLOSURE; 
            else
                document->dmFileMetaType = FILEMETATYPE_MAIN; 
            document->dmFileDescr = "in-line.xml";

            if (xpath2nodelist(&document->xml_node_list, xpath_ctx,
                    BAD_CAST argv[j])) {
                printf("Could not convert XPath result to node list: %s\n",
                        argv[j]);
                exit(EXIT_FAILURE);
            }

            documents_item = calloc(1, sizeof(*documents_item));
            if (!documents_item) {
                printf("Not enough memory\n");
                exit(EXIT_FAILURE);
            }

            documents_item->data = document,
            documents_item->destructor = (void(*)(void**))isds_document_free;
            if (!prev_documents_item) 
                documents = prev_documents_item = documents_item;
            else {
                prev_documents_item->next = documents_item;
                prev_documents_item = documents_item;
            }
        }

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

        message.documents = documents;

        printf("Sending message to box ID `%s'\n", recipient);
        err = isds_send_message(ctx, &message);
        
        if (err == IE_SUCCESS){
            printf("isds_send_message() succeeded: message ID = %s\n",
                    message.envelope->dmID);
        } else 
            printf("isds_send_message() failed: "
                    "%s: %s\n", isds_strerror(err), isds_long_message(ctx));

    }

    /* Free document xml_node_lists because they are weak copies of nodes in
     * message->xml and isds_document_free() does not free it. */
    for (struct isds_list *item = documents; item; item = item->next) {
        if (item->data) {
            struct isds_document *document =
                (struct isds_document *)item->data;
            if (document->is_xml) {
                for (xmlNodePtr node = document->xml_node_list; node;
                        node = node->next)
                    free(node);
            }
        }
    }

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
