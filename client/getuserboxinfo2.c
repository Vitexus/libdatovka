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
    struct isds_DbOwnerInfoExt2 *db_owner_info = NULL;

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
        printf("Getting info about my box:\n");
        err = isds_GetOwnerInfoFromLogin2(ctx, &db_owner_info);
        if (err) {
            printf("isds_GetOwnerInfoFromLogin2() failed: %s: %s\n",
                    isds_strerror(err), isds_long_message(ctx));
        } else {
            printf("isds_GetOwnerInfoFromLogin2() succeeded\n");
        }
        print_DbOwnerInfoExt2(db_owner_info);

    }

    {
        struct isds_list *boxes = NULL, *item;

        printf("Searching for my own box:\n");
        err = isds_FindDataBox2(ctx, db_owner_info, &boxes);
        if (err == IE_SUCCESS || err == IE_TOO_BIG) {
            if (err == IE_TOO_BIG)
                printf("isds_FindDataBox2() results truncated\n");
            printf("isds_FindDataBox2() succeeded:\n");

            for(item = boxes; item; item = item->next) {
                printf("List item:\n");
                print_DbOwnerInfoExt2(item->data);
            }
        } else {
            printf("isds_FindDataBox2() failed: %s: %s\n",
                    isds_strerror(err), isds_long_message(ctx));
        }

        isds_list_free(&boxes);
    }


    /* Get box delivery info */
    if (db_owner_info) {
        long int box_status = 0;
        printf("Getting status of my box with ID `%s'\n", db_owner_info->dbID);
        err = isds_CheckDataBox(ctx, db_owner_info->dbID, &box_status);
        if (err)
            printf("isds_CheckDataBox() failed: %s: %s\n",
                    isds_strerror(err), isds_long_message(ctx));
        else {
            printf("isds_CheckDataBox() succeeded: status = ");
            print_DbState(box_status);
        }
    }

    /* Get info all users of this box */
    if (db_owner_info) {
        struct isds_list *users = NULL, *item;
        printf("Getting users of my box with ID `%s':\n", db_owner_info->dbID);
        err = isds_GetDataBoxUsers2(ctx, db_owner_info->dbID, &users);
        if (err) {
            printf("isds_GetDataBoxUsers2() failed: %s: %s\n",
                    isds_strerror(err), isds_long_message(ctx));
        } else {
            printf("isds_GetDataBoxUsers2() succeeded\n");
            for(item = users; item; item = item->next) {
                printf("List item:\n");
                print_DbUserInfoExt2(item->data);
            }
        }
        isds_list_free(&users);
    }

    isds_DbOwnerInfoExt2_free(&db_owner_info);

    {
        /* Get info about my account */
        struct isds_DbUserInfoExt2 *db_user_info = NULL;
        printf("Getting info about my account:\n");
        err = isds_GetUserInfoFromLogin2(ctx, &db_user_info);
        if (err) {
            printf("isds_GetUserInfoFromLogin2() failed: %s: %s\n",
                    isds_strerror(err), isds_long_message(ctx));
        } else {
            printf("isds_GetUserInfoFromLogin2() succeeded\n");
            print_DbUserInfoExt2(db_user_info);
        }
        isds_DbUserInfoExt2_free(&db_user_info);
    }

    {
        /* Get password expiration time */
        struct isds_timeval *expiration = NULL;
        printf("Getting password expiration time\n");
        err = isds_get_password_expiration(ctx, &expiration);
        if (err)
            printf("isds_get_password_expiration() failed: %s: %s\n",
                    isds_strerror(err), isds_long_message(ctx));
        else {
            printf("isds_get_password_expiration() succeeded: "
                    "Password expires at: ");
            if (expiration)
                print_timeval(expiration);
            else
                printf("<Never>\n");
        }
        free(expiration);
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
