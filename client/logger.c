#define _XOPEN_SOURCE 500
#include <stdlib.h>
#include <stdio.h>
#include <locale.h>
#include <time.h>
#include <string.h>
#include <libdatovka/isds.h>
#include "common.h"

void logger(isds_log_facility facility, isds_log_level level,
        const char *message, int length, void *data) {
    /* Silent warning about unused argument.
     * It's a protopype isds_log_callback. */
    (void) data;

    printf("\033[32mLOG(%02d,%02d): ", facility, level);
    printf("%.*s", length, message);
    printf("\033[m");
}


int main(void) {
    struct isds_ctx *ctx = NULL;
    isds_error err;

    setlocale(LC_ALL, "");

    err = isds_init();
    if (err) {
        printf("isds_init() failed: %s\n", isds_strerror(err));
        exit(EXIT_FAILURE);
    } else {
        printf("isds_init() succeeded\n");
    }

    isds_set_log_callback(logger, NULL);
    isds_set_logging(ILF_ALL, ILL_ALL);

    ctx = isds_ctx_create();
    if (!ctx) {
        printf("isds_ctx_create() failed");
    } else {
        printf("isds_ctx_create() succeeded\n");
    }


    err = isds_set_timeout(ctx, 10000);
    if (err) {
        printf("isds_set_timeout() failed: %s\n", isds_strerror(err));
    } else {
        printf("isds_set_timeout() succeeded\n");
    }


    err = isds_login(ctx, url, username(), password(), NULL, NULL);
    if (err) {
        printf("isds_login() failed: %s: %s\n", isds_strerror(err),
                isds_long_message(ctx));
    } else {
        printf("Logged in :)\n");
    }


    err = isds_logout(ctx);
    if (err) {
        printf("isds_logout() failed: %s\n", isds_strerror(err));
    } else {
        printf("isds_logout() succeeded\n");
    }



    err = isds_ctx_free(&ctx);
    if (err) {
        printf("isds_ctx_free() failed: %s\n", isds_strerror(err));
    } else {
        printf("isds_ctx_free() succeeded\n");
    }



    err = isds_cleanup();
    if (err) {
        printf("isds_cleanup() failed: %s\n", isds_strerror(err));
    } else {
        printf("isds_ctx_cleanup() succeeded\n");
    }


    exit (EXIT_SUCCESS);
}
