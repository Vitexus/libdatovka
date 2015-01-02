#define _XOPEN_SOURCE 500
#include <stdlib.h>
#include <stdio.h>
#include <locale.h>
#include <time.h>
#include <string.h>
#include <isds.h>
#include <libxml/debugXML.h>
#include "common.h"


int main(void) {
    struct isds_ctx *ctx = NULL;
    isds_error err;
    
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


    {
        /* Load plain signed sent message with XML documents */
        struct isds_message *message = NULL;
        void *buffer;
        int fd;
        size_t length;

        if (mmap_file(SRCDIR
                    "/server/messages/signed_sent_xml_message-376701.xml",
                &fd, &buffer, &length)) {
            fprintf(stderr, "Could not map file with message");
            isds_ctx_free(&ctx);
            isds_cleanup();
            exit(EXIT_FAILURE);
        }

        printf("Loading plain signed sent message\n");
        err = isds_load_message(ctx, RAWTYPE_PLAIN_SIGNED_OUTGOING_MESSAGE,
                buffer, length, &message, BUFFER_DONT_STORE);
        if (err)
            printf("isds_load_message() failed: %s: %s\n",
                    isds_strerror(err), isds_long_message(ctx));
        else {
            printf("isds_load_message() succeeded:\n");
            print_message(message);

            /* Dump XML documents */
            int i = 1;
            for (struct isds_list *item = message->documents; item;
                    item = item->next, i++) {
                if (item->data) {
                    struct isds_document *document =
                        (struct isds_document *)item->data;
                    if (document->is_xml) {
                        printf("* XML Document #%d dump:\n", i);
                        xmlDebugDumpNodeList(stdout,
                                document->xml_node_list, 2);
                    }
                }
            }
        }

        isds_message_free(&message);
        munmap_file(fd, buffer, length);
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
