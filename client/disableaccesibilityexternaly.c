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

    err = isds_login(ctx, url, username(), password(), NULL, NULL);
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


    /* Disable access to my box externally. It should fail. */
    if (db_owner_info) {
        struct isds_DbOwnerInfo *new_db_owner_info = NULL;
        struct tm date = {
            .tm_year = 103,
            .tm_mon = 1,
            .tm_mday = 1
        };
        char *refnumber = NULL;

        printf("Disabling access to my box externally since: ");
        print_date(&date);
        err = isds_disable_box_accessibility_externaly(ctx,
                db_owner_info, &date, NULL, &refnumber);
        if (err)
            printf("isds_disable_box_accessibility_externaly() failed: "
                    "%s: %s\n", isds_strerror(err), isds_long_message(ctx));
        else {
            printf("isds_disable_box_accessibility_externaly() "
                    "succeeded as request #%s\n", refnumber);
        }
        free(refnumber);

        printf("Verifying info about my box:\n");
        err = isds_GetOwnerInfoFromLogin(ctx, &new_db_owner_info);
        if (err) {
            printf("isds_GetOwnerInfoFromLogin() failed: %s: %s\n",
                    isds_strerror(err), isds_long_message(ctx));
        } else {
            printf("isds_GetOwnerInfoFromLogin() succeeded\n");
            printf("New box status is: ");
            print_longint(new_db_owner_info->dbState);
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
