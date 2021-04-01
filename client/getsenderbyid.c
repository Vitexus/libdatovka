#define _XOPEN_SOURCE 500
#include <stdlib.h>
#include <stdio.h>
#include <locale.h>
#include <time.h>
#include <string.h>
#include <libdatovka/isds.h>
#include "common.h"


int main(int argc, char **argv) {
    struct isds_ctx *ctx = NULL;
    isds_error err;

    setlocale(LC_ALL, "");

    if (argc < 2) {
        fprintf(stderr, "Usage: program YOUR_MESSAGE_ID\n");
        exit(EXIT_FAILURE);
    }

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


    /* Get sender of message selected by message ID */
    if (argv[1]) {
        isds_sender_type *type = NULL;
        char *raw_type = NULL;
        char *name = NULL;

        printf("Getting sender of message with ID: %s\n",
                argv[1]);
        err = isds_get_message_sender(ctx, argv[1], &type, &raw_type, &name);
        if (err)
            printf("isds_get_message_sender() failed: %s: %s\n",
                    isds_strerror(err), isds_long_message(ctx));
        else {
            printf("isds_get_message_sender() succeeded:\n");
            printf("type = ");
            print_sender_type(type);
            printf("raw_type = %s\n"
                    "name = %s\n", raw_type, name);
        }

        free(type);
        free(raw_type);
        free(name);
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
