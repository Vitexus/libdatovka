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
    char *last_message_id = NULL;

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



    /* Get list of received messages */
    {
        struct tm from_time_tm = {
            .tm_year = 2000 - 1900,
            .tm_mon = 1 - 1,
            .tm_mday = 1,
            .tm_hour = 1,
            .tm_min = 2,
            .tm_sec = 3,
            .tm_isdst = -1
        };
        time_t from_time_t = mktime(&from_time_tm);
        struct isds_timeval from_time = {
            .tv_sec = from_time_t,
            .tv_usec = 4000
        };
        unsigned long int number = 0;
        struct isds_list *messages = NULL, *item;
        struct isds_message *last_message = NULL;

        /* TODO: Try different criteria */
        printf("Getting list of received messages\n");
        err = isds_get_list_of_received_messages(ctx, &from_time, NULL, NULL,
                MESSAGESTATE_ANY, 0, &number, &messages);
        if (err)
            printf("isds_get_list_of_received_messages() failed: %s: %s\n",
                    isds_strerror(err), isds_long_message(ctx));
        else {
            printf("isds_get_list_of_received_messages() succeeded: "
                    "number of messages = %lu:\n", number);
            for(item = messages; item; item = item->next) {
                last_message = (struct isds_message *) (item->data);
            }

        }

        if (last_message) {
            /* Save last message for latter reference */
            if (last_message->envelope && last_message->envelope->dmID) {
                last_message_id = strdup(last_message->envelope->dmID);
            }
        }

        isds_list_free(&messages);
    }


    if (last_message_id) {
        /* Download last message */
        struct isds_message *message = NULL;

        printf("Getting last received message with ID: %s\n", last_message_id);
        err = isds_get_received_message(ctx, last_message_id, &message);
        if (err)
            printf("isds_get_received_message() failed: %s: %s\n",
                    isds_strerror(err), isds_long_message(ctx));
        else {
            printf("isds_get_received_message() succeeded:\n");
            print_message(message);
        }


        /* Verify message hash */
        printf("Verifying last received message hash against server\n");
        err = isds_verify_message_hash(ctx, message);
        if (!err) {
            printf("isds_verify_message_hash() succeeded: "
                    "message is genuine\n");
            printf("Computed hash: ");
            print_hash(message->envelope->hash);
        } else if (err == IE_NOTEQUAL) {
            printf("isds_verify_message_hash() failed: message is a fake\n");
            printf("Computed hash: ");
            print_hash(message->envelope->hash);
            printf("This should not happen\n");
        } else {
            printf("isds_verify_message_hash() failed: %s: %s\n",
                    isds_strerror(err), isds_long_message(ctx));
        }


        /* Download last signed message */
        printf("Getting last signed received message with ID: %s\n",
                last_message_id);
        err = isds_get_signed_received_message(ctx, last_message_id, &message);
        if (err)
            printf("isds_get_signed_received_message() failed: %s: %s\n",
                    isds_strerror(err), isds_long_message(ctx));
        else {
            printf("isds_get_signed_received_message() succeeded:\n");
            printf("\tMessage should have the same content ;)\n");
        }

        /* Verify signed message hash */
        printf("Verifying last signed received message hash against server\n");
        err = isds_verify_message_hash(ctx, message);
        if (!err) {
            printf("isds_verify_message_hash() succeeded: "
                    "message is genuine\n");
            printf("Computed hash: ");
            print_hash(message->envelope->hash);
        } else if (err == IE_NOTEQUAL) {
            printf("isds_verify_message_hash() failed: message is a fake\n");
            printf("Computed hash: ");
            print_hash(message->envelope->hash);
            printf("This should not happen\n");
        } else {
            printf("isds_verify_message_hash() failed: %s: %s\n",
                    isds_strerror(err), isds_long_message(ctx));
        }


        isds_message_free(&message);
        free(last_message_id);
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
