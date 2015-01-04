#include "../config.h"
#define _XOPEN_SOURCE XOPEN_SOURCE_LEVEL_FOR_STRDUP
#include <stdlib.h>
#include <stdio.h>
#include <locale.h>
#include <time.h>
#include <string.h>
#include <isds.h>
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



    /* Get list of sent messages */
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
        struct timeval from_time = {
            .tv_sec = from_time_t,
            .tv_usec = 4000
        };
        unsigned long int number = 0;
        struct isds_list *messages = NULL, *item;
        struct isds_message *last_message = NULL;

        /* TODO: Try different criteria */
        printf("Getting list of sent messages\n");
        err = isds_get_list_of_sent_messages(ctx, &from_time, NULL, NULL,
                MESSAGESTATE_ANY, 0, &number, &messages);
        if (err)
            printf("isds_get_list_of_sent_messages() failed: %s: %s\n",
                    isds_strerror(err), isds_long_message(ctx));
        else {
            printf("isds_get_list_of_sent_messages() succeeded: "
                    "number of messages = %lu:\n", number);
            for(item = messages; item; item = item->next) {
                last_message = (struct isds_message *) (item->data);
            }

        }

        if (last_message) {
            /*Save last message for latter refference */
            if (last_message->envelope && last_message->envelope->dmID) {
                last_message_id = strdup(last_message->envelope->dmID);
            }
        }

        isds_list_free(&messages);
    }


    /* Download last signed message */
    if (last_message_id) {
        struct isds_message *message = NULL;

        printf("Getting last signed sent message with ID: %s\n",
                last_message_id);
        err = isds_get_signed_sent_message(ctx, last_message_id, &message);
        if (err)
            printf("isds_get_signed_sent_message() failed: %s: %s\n",
                    isds_strerror(err), isds_long_message(ctx));
        else {
            printf("isds_get_signed_sent_message() succeeded:\n");
            print_message(message);
            save_data("Saving signed message",
                    message->raw, message->raw_length);
        }

        isds_message_free(&message);
        free(last_message_id);
    }


    /* Download signed message with invalid ID*/
    {
        struct isds_message *message = NULL;
        char *id = "123456789112345678921";

        printf("Getting signed sent message with invalid ID: %s\n", id);
        err = isds_get_signed_sent_message(ctx, id, &message);
        if (err)
            printf("isds_get_signed_sent_message() failed as assumed: "
                    "%s: %s\n", isds_strerror(err), isds_long_message(ctx));
        else {
            printf("isds_get_signed_sent_message() succeeded. "
                    "This should not happen:\n");
            print_message(message);
        }

        isds_message_free(&message);
    }


    /* Download nonexistent signed message */
    {
        struct isds_message *message = NULL;
        char *id = "7777777";

        printf("Getting nonexistent signed sent message with ID: %s\n", id);
        err = isds_get_signed_sent_message(ctx, id, &message);
        if (err)
            printf("isds_get_signed_sent_message() failed as assumed: %s: %s\n",
                    isds_strerror(err), isds_long_message(ctx));
        else {
            printf("isds_get_signed_sent_message() succeeded. "
                    "This should not happen:\n");
            print_message(message);
        }

        isds_message_free(&message);
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
