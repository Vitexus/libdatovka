#define _XOPEN_SOURCE 500
#include <stdlib.h>
#include <stdio.h>
#include <locale.h>
#include <time.h>
#include <string.h>
#include <isds.h>
#include "common.h"

/* Get info about my account */
isds_error get_my_box(struct isds_ctx *ctx,
        struct isds_DbOwnerInfo **db_owner_info) {
    isds_error err = IE_SUCCESS;
        printf("Getting info about my box:\n");
        err = isds_GetOwnerInfoFromLogin(ctx, db_owner_info);
        if (err) {
            printf("isds_GetOwnerInfoFromLogin() failed: %s: %s\n",
                    isds_strerror(err), isds_long_message(ctx));
        } else {
            printf("isds_GetOwnerInfoFromLogin() succeeded\n");
        }
        print_DbOwnerInfo(*db_owner_info);
    return err;
}


int main(int argc, char **argv) {
    struct isds_ctx *ctx = NULL;
    isds_error err;
    struct isds_DbOwnerInfo *db_owner_info = NULL;
    
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
        printf("Get current box info\n");
        get_my_box(ctx, &db_owner_info);
    }


    if (db_owner_info) {
        /* Update box info */
        struct isds_DbOwnerInfo *old_owner_info = NULL;
        char *refnumber = NULL;

        old_owner_info = isds_DbOwnerInfo_duplicate(db_owner_info);
        if (!old_owner_info) {
            fprintf(stderr, "No enough memory\n");
            exit(EXIT_FAILURE);
        }

        printf("Updating info about my box: with no change\n");
        err = isds_UpdateDataBoxDescr(ctx, old_owner_info, db_owner_info,
                NULL, &refnumber);
        if (err) {
            printf("isds_UpdateDataBoxDescr() failed: %s: %s\n",
                    isds_strerror(err), isds_long_message(ctx));
        } else {
            printf("isds_UpdateDataBoxDescr() succeeded as request #%s\n",
                    refnumber);
            printf("Get new box info\n");
            get_my_box(ctx, &db_owner_info);
        }

        free(refnumber);
        isds_DbOwnerInfo_free(&old_owner_info);
    }

    isds_DbOwnerInfo_free(&db_owner_info);



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
