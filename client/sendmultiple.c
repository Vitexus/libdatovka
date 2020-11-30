#include "../config.h"
#define _XOPEN_SOURCE XOPEN_SOURCE_LEVEL_FOR_STRDUP
#include <stdlib.h>
#include <stdio.h>
#include <locale.h>
#include <time.h>
#include <string.h>
#include <libdatovka/isds.h>
#include "common.h"


int main(void) {
    struct isds_ctx *ctx = NULL;
    isds_error err;
    char *recipient1 = NULL, *recipient2 = NULL;
    
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

    err = isds_login(ctx, url, username(), password(), NULL, NULL);
    if (err) {
        printf("isds_login() failed: %s: %s\n", isds_strerror(err),
                isds_long_message(ctx));
    } else {
        printf("Logged in :)\n");
    }


    {
        /* Find some recipients */
        struct isds_list *boxes = NULL, *item;
        struct isds_DbOwnerInfo criteria;
        isds_DbType criteria_db_type = DBTYPE_OVM;
        memset(&criteria, 0, sizeof(criteria));
        criteria.firmName = "Místní";
        criteria.dbType = &criteria_db_type;

        printf("Searching box with firm name `%s':\n", criteria.firmName);
        err = isds_FindDataBox(ctx, &criteria, &boxes);
        if (err == IE_SUCCESS || err == IE_2BIG) {
            printf("isds_FindDataBox() succeeded:\n");

            int n;
            for(item = boxes, n = 1; item; item = item->next, n++) {
                if (err != IE_2BIG) {
                    printf("List item #%d:\n", n);
                    print_DbOwnerInfo(item->data);
                }
                if (n == 1) recipient1 = strdup(
                        ((struct isds_DbOwnerInfo *)(item->data))->dbID);
                if (n == 2) recipient2 = strdup(
                        ((struct isds_DbOwnerInfo *)(item->data))->dbID);
            }
            if (err == IE_2BIG) 
                printf("isds_FindDataBox() results truncated to %d boxes\n",
                        --n);
        } else {
            printf("isds_FindDataBox() failed: %s: %s\n",
                    isds_strerror(err), isds_long_message(ctx));
        }

        isds_list_free(&boxes);
    }


    {
        /* Send one message to more recipients  */
        struct isds_message message;
        memset(&message, 0, sizeof(message));

        struct isds_envelope envelope;
        memset(&envelope, 0, sizeof(envelope));
        message.envelope = &envelope;
        long int dmSenderOrgUnitNum = 42;
        envelope.dmSenderOrgUnitNum = &dmSenderOrgUnitNum;
        envelope.dmAnnotation = "Multiple recipients";
       
        struct isds_document minor_document;
        memset(&minor_document, 0, sizeof(minor_document));
        minor_document.data = "hello world?";
        minor_document.data_length = strlen(minor_document.data);
        minor_document.dmMimeType = "text/plain";
        minor_document.dmFileMetaType = FILEMETATYPE_ENCLOSURE;
        minor_document.dmFileDescr = "minor_standard_text.txt";
        minor_document.dmFileGuid = "2";
        minor_document.dmUpFileGuid = "1";

        struct isds_document main_document;
        memset(&main_document, 0, sizeof(main_document));
        main_document.data = "Hello World!";
        main_document.data_length = strlen(main_document.data);
        main_document.dmMimeType = "text/plain";
        main_document.dmFileMetaType = FILEMETATYPE_MAIN;
        main_document.dmFileDescr = "standard_text.txt";
        main_document.dmFileGuid = "1";

        struct isds_list documents_main_item = {
            .data = &main_document,
            .next = NULL,
            .destructor = NULL
        };
        struct isds_list documents_minor_item = {
            .data = &minor_document,
            .next = &documents_main_item,
            .destructor = NULL
        };
        message.documents = &documents_minor_item;

        /* Build recipients list */
        struct isds_list *copies = NULL;
        copies = calloc(1, sizeof(*copies));
        copies->destructor = (void (*)(void**)) isds_message_copy_free;
        copies->data = calloc(1, sizeof(struct isds_message_copy));
        ((struct isds_message_copy*) copies->data)->dbIDRecipient = recipient1;

        copies->next = calloc(1, sizeof(*copies->next));
        copies->next->destructor = (void (*)(void**)) isds_message_copy_free;
        copies->next->data = calloc(1, sizeof(struct isds_message_copy));
        ((struct isds_message_copy*)copies->next->data)->dbIDRecipient =
            recipient2;


        printf("Sending message to box ID `%s' and box ID `%s'\n",
                recipient1, recipient2);
        err = isds_send_message_to_multiple_recipients(ctx, &message, copies);
        
        if (err == IE_SUCCESS || err == IE_PARTIAL_SUCCESS) {
            if (err == IE_SUCCESS){
                printf("isds_send_message_to_multiple_recipients() succeeded "
                        "cempletely:\n");
            }
            if (err == IE_PARTIAL_SUCCESS){
                printf("isds_send_message_to_multiple_recipients() succeeded "
                        "partialy:\n");
            }
            print_copies(copies);
        } else 
            printf("isds_send_message_to_multiple_recipients() failed: "
                    "%s: %s\n", isds_strerror(err), isds_long_message(ctx));

        isds_list_free(&copies);
    }



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
