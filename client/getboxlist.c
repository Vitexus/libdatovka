#define _XOPEN_SOURCE 500
#include <stdlib.h>
#include <stdio.h>
#include <locale.h>
#include <string.h>
#include <isds.h>
#include "common.h"


int main(int argc, char **argv) {
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

    /* err = isds_set_tls(ctx, ITLS_VERIFY_SERVER, 0);
    if (err) {
        printf("isds_set_tls(ITLS_VERIFY_SERVER) failed: %s\n",
                isds_strerror(err));
    }

    err = isds_set_tls(ctx, ITLS_CA_FILE, "/etc/ssl/certs/ca-certificates.crt");
    if (err) {
        printf("isds_set_tls(ITLS_CA_FILE) failed: %s\n",
                isds_strerror(err));
    }*/

    err = isds_login(ctx, url, username, password, NULL);
    if (err) {
        printf("isds_login() failed: %s: %s\n", isds_strerror(err),
                isds_long_message(ctx));
    } else {
        printf("Logged in :)\n");
    }


    {
        void *archive = NULL;
        size_t length;

        err = isds_get_box_list_archive(ctx, "UPG", &archive, &length);
        if (err) {
            printf("isds_getboxlist() failed: %s: %s\n",
                    isds_strerror(err), isds_long_message(ctx));
        } else {
            printf("isds_getboxlist() succeeded:\n");
            printf("length = %zu B\n", length);
            save_data("Saving ZIP archive", archive, length);
        }
        printf("\n");

        free(archive);
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
