#define _XOPEN_SOURCE 500
#include <stdlib.h>
#include <stdio.h>
#include <locale.h>
#include <string.h>
#include <sys/time.h>
#include <isds.h>
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


    {
        struct isds_list *changed_states = NULL, *item;
        struct timeval not_before = { .tv_sec = 0, .tv_usec = 0 };
        struct timeval not_after;

        if (!gettimeofday(&not_after, NULL)) {
            /* Changes older than 15 days are not reported currently. */
            not_before.tv_sec = not_after.tv_sec - 10 * 24 * 3600;

            err = isds_get_list_of_sent_message_state_changes(ctx,
                    &not_before, &not_after, &changed_states);
            if (err) {
                printf("isds_get_list_of_sent_message_state_changes() failed: "
                        "%s: %s\n", isds_strerror(err), isds_long_message(ctx));
            } else {
                printf("isds_get_list_of_sent_message_state_changes() "
                        "succeeded:\n");
                if (!changed_states)
                    printf("No message status changes available\n");
                else
                    for(item = changed_states; item; item = item->next) {
                        printf("List item:\n");
                        print_message_status_change(item->data);
                    }
            }
            printf("\n");

            isds_list_free(&changed_states);
        } else {
            perror("Could not get current time");
        }
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
