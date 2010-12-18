#define _XOPEN_SOURCE 500
#include <stdlib.h>
#include <stdio.h>
#include <locale.h>
#include <time.h>
#include <string.h>
#include <isds.h>
#include "common.h"


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

    err = isds_login(ctx, url, username, password, NULL);
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


    /* Switch commecrial message receiving status */
    if (db_owner_info) {
        _Bool allow;
        struct isds_DbOwnerInfo *new_db_owner_info = NULL;
        char *refnumber = NULL;

        if (db_owner_info->dbOpenAddressing)
            allow = !*db_owner_info->dbOpenAddressing;
        else
            allow = 1;

        printf("Switching commerical receiving status to: %s\n",
                (allow) ? "true" : "false");
        err = isds_switch_commercial_receiving(ctx, db_owner_info->dbID, allow,
                NULL, &refnumber);
        if (err)
            printf("isds_switch_commercial_receiving() failed: %s: %s\n",
                    isds_strerror(err), isds_long_message(ctx));
        else {
            printf("isds_switch_commercial_receiving() succeeded "
                    "as request #%s\n", refnumber);
        }
        free(refnumber);

        printf("Verifying info about my box:\n");
        err = isds_GetOwnerInfoFromLogin(ctx, &new_db_owner_info);
        if (err) {
            printf("isds_GetOwnerInfoFromLogin() failed: %s: %s\n",
                    isds_strerror(err), isds_long_message(ctx));
        } else {
            printf("isds_GetOwnerInfoFromLogin() succeeded\n");
            printf("New status is: ");
            print_bool(new_db_owner_info->dbOpenAddressing);
        }
        isds_DbOwnerInfo_free(&new_db_owner_info);
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
