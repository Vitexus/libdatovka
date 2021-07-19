#define _XOPEN_SOURCE 500
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
        struct isds_timeval from_time = {
            .tv_sec = from_time_t,
            .tv_usec = 4000
        };
        unsigned long int number = 0;
        struct isds_list *messages = NULL, *item;

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
                printf("List item:\n");
                print_message(item->data);
            }

        }
        isds_list_free(&messages);
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
