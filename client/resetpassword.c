#define _XOPEN_SOURCE 500
#include <stdlib.h>
#include <stdio.h>
#include <locale.h>
#include <time.h>
#include <string.h>
#include <isds.h>
#include "common.h"

void reset_password(struct isds_ctx *ctx,
        const struct isds_DbOwnerInfo *box, const struct isds_DbUserInfo *user,
        const _Bool paid,
        struct isds_credentials_delivery *credentials_delivery) {
    char *refnumber = NULL;

    printf("Resetting password\n");
    isds_error err = isds_reset_password(ctx, box, user, paid, NULL,
            credentials_delivery, &refnumber);
    if (err) {
        printf("isds_reset_password() failed: %s: %s\n",
                isds_strerror(err), isds_long_message(ctx));
    } else {
        printf("isds_reset_password() succeeded as request #%s.\n"
                "This should not happen\n", refnumber);
        if (credentials_delivery) {
            printf("email = %s\n", credentials_delivery->email);
            printf("token = %s\n", credentials_delivery->token);
            printf("new_user_name = %s\n", credentials_delivery->new_user_name);
        }
    }
    printf("\n");
    free(refnumber);
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

    err = isds_login(ctx, url, username, password, NULL);
    if (err) {
        printf("isds_login() failed: %s: %s\n", isds_strerror(err),
                isds_long_message(ctx));
    } else {
        printf("Logged in :)\n");
    }


    {
        /* Get info about my box */
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
        printf("Getting info about my account:\n");
        err = isds_GetUserInfoFromLogin(ctx, &db_user_info);
        if (err) {
            printf("isds_GetUserInfoFromLogin() failed: %s: %s\n",
                    isds_strerror(err), isds_long_message(ctx));
        } else {
            printf("isds_GetUserInfoFromLogin() succeeded\n");
            print_DbUserInfo(db_user_info);
        }
    }

    if (db_owner_info && db_user_info) {
#if 0
        struct isds_credentials_delivery credentials_delivery;
        /* Try some invalid invocation that should fail */
        reset_password(ctx, db_owner_info, db_user_info, 0, NULL);
        reset_password(ctx, db_owner_info, db_user_info, 1, NULL);

        credentials_delivery.email = NULL;
        credentials_delivery.token = NULL;
        credentials_delivery.new_user_name = NULL;
        reset_password(ctx, db_owner_info, db_user_info, 1,
                &credentials_delivery);
        free(credentials_delivery.token);
        free(credentials_delivery.new_user_name);
        
        credentials_delivery.email = "42";
        credentials_delivery.token = NULL;
        credentials_delivery.new_user_name = NULL;
        reset_password(ctx, db_owner_info, db_user_info, 1,
                &credentials_delivery);
        free(credentials_delivery.token);
        free(credentials_delivery.new_user_name);
#endif
        fprintf(stderr, "This function can lose current user credentials. "
                "DO NOT TRY IT!\n");
    }

    isds_DbOwnerInfo_free(&db_owner_info);
    isds_DbUserInfo_free(&db_user_info);


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
