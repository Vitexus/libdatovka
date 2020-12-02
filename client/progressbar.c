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

    isds_set_logging(ILF_ALL & ~ILF_HTTP & ~ILF_SOAP, ILL_ALL);

    ctx = isds_ctx_create();
    if (!ctx) {
        printf("isds_ctx_create() failed");
    }

    err = isds_set_timeout(ctx, 10000);
    if (err) {
        printf("isds_set_timeout() failed: %s\n", isds_strerror(err));
    }

   
    /* Register progresbar */
    err = isds_set_progress_callback(ctx, progressbar, NULL);
    if (err) {
        printf("isds_set_progress_callback() failed: %s: %s\n",
                isds_strerror(err), isds_long_message(ctx));
    } else {
        printf("isds_set_progress_callback() succeeded.\n");
    }

    err = isds_login(ctx, url, username(), password(), NULL, NULL);
    if (err) {
        printf("isds_login() failed: %s: %s\n", isds_strerror(err),
                isds_long_message(ctx));
    } else {
        printf("Logged in :)\n");
    }


    /* Register aborting progresbar */
    printf("\nTesting aborting progress callback\n");
    err = isds_set_progress_callback(ctx, progressbar, (void *)1);
    if (err) {
        printf("isds_set_progress_callback() failed: %s: %s\n",
                isds_strerror(err), isds_long_message(ctx));
    } else {
        printf("isds_set_progress_callback() succeeded.\n");
    }
    err = isds_ping(ctx);
    if (err) {
        printf("isds_ping() failed: %s: %s\n", isds_strerror(err),
                isds_long_message(ctx));
    } else {
        printf("Ping succeded\n");
    }


    /* Register normal progresbar */
    printf("\nTesting non-aborting progress callback\n");
    err = isds_set_progress_callback(ctx, progressbar, NULL);
    if (err) {
        printf("isds_set_progress_callback() failed: %s: %s\n",
                isds_strerror(err), isds_long_message(ctx));
    } else {
        printf("isds_set_progress_callback() succeeded.\n");
    }
    err = isds_ping(ctx);
    if (err) {
        printf("isds_ping() failed: %s: %s\n", isds_strerror(err),
                isds_long_message(ctx));
    } else {
        printf("Ping succeded\n");
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
