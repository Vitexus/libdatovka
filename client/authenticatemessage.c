#define _XOPEN_SOURCE 500
#include <stdlib.h>
#include <stdio.h>
#include <locale.h>
#include <time.h>
#include <string.h>
#include <libdatovka/isds.h>
#include "common.h"
#include <libgen.h>


int main(int argc, char **argv) {
    struct isds_ctx *ctx = NULL;
    isds_error err;

    setlocale(LC_ALL, "");

    if (argc != 2 || !argv[1] || !*argv[1]) {
        printf("Usage: %s CMS_SIGNED_MESSAGE_IN_LOCAL_FILE\n",
                basename(argv[0]));
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


    {
        /* Authenticate message saved in local file */
        int fd;
        void *buffer;
        size_t length;

        if (mmap_file(argv[1], &fd, &buffer, &length))
            exit(EXIT_FAILURE);

        printf("Sending message from file `%s' to ISDS for authenticity "
                "check...\n", argv[1]);
        err = isds_authenticate_message(ctx, buffer, length);
        if (!err)
            printf("ISDS states: message is original\n");
        else if (err == IE_NOTEQUAL)
            printf("ISDS states: message is unkown or tampered\n");
        else
            printf("isds_authenticate_message() failed: %s: %s\n",
                    isds_strerror(err), isds_long_message(ctx));

        munmap_file(fd, buffer,length);
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
