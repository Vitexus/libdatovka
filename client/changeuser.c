#define _XOPEN_SOURCE 500
#include <stdlib.h>
#include <stdio.h>
#include <locale.h>
#include <time.h>
#include <string.h>
#include <isds.h>
#include "common.h"

/* Get info about my account */
isds_error get_my_account(struct isds_ctx *ctx,
        struct isds_DbUserInfo **db_user_info) {
    isds_error err = IE_SUCCESS;
    printf("Getting info about my account:\n");
    err = isds_GetUserInfoFromLogin(ctx, db_user_info);
    if (err) {
        printf("isds_GetUserInfoFromLogin() failed: %s: %s\n",
                isds_strerror(err), isds_long_message(ctx));
    } else {
        printf("isds_GetUserInfoFromLogin() succeeded\n");
        print_DbUserInfo(*db_user_info);
    }
    return err;
}


int main(int argc, char **argv) {
    struct isds_ctx *ctx = NULL;
    isds_error err;
    struct isds_DbOwnerInfo *db_owner_info = NULL;
    struct isds_DbUserInfo *db_user_info = NULL;
    
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

    err = isds_login(ctx, url, username, password, NULL, NULL);
    if (err) {
        printf("isds_login() failed: %s: %s\n", isds_strerror(err),
                isds_long_message(ctx));
    } else {
        printf("Logged in :)\n");
    }


    {
        printf("Getting info about my box:\n");
        err = isds_GetOwnerInfoFromLogin(ctx, &db_owner_info);
        if (err) {
            printf("isds_GetOwnerInfoFromLogin() failed: %s: %s\n",
                    isds_strerror(err), isds_long_message(ctx));
        } else {
            printf("isds_GetOwnerInfoFromLogin() succeeded\n");
        }
        print_DbOwnerInfo(db_owner_info);

    }


    {
        /* Get info about my account */
        err = get_my_account(ctx, &db_user_info);
    }

    if (db_user_info && db_owner_info) {
        /* Update user info */
        struct isds_DbUserInfo *old_user_info = NULL;
        old_user_info = isds_DbUserInfo_duplicate(db_user_info);
        if (!old_user_info) {
            fprintf(stderr, "No enough memory\n");
            exit(EXIT_FAILURE);
        }

        printf("Updating info about my account: with no change\n");
        err = isds_UpdateDataBoxUser(ctx, db_owner_info,
                old_user_info, db_user_info);
        if (err) {
            printf("isds_UpdateDataBoxUser() failed: %s: %s\n",
                    isds_strerror(err), isds_long_message(ctx));
        } else {
            printf("isds_UpdateDataBoxUser() succeeded\n");

            /* Verify info about my account */
            get_my_account(ctx, &db_user_info);
        }

        isds_DbUserInfo_free(&old_user_info);
    }

    isds_DbUserInfo_free(&db_user_info);
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
