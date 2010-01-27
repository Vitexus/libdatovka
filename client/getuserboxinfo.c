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
        /* Current server implementation (2009-11-17) does not allow to find
         * myself. Previous version allowed it. */
        struct isds_list *boxes = NULL, *item;

        printf("Searching for my own box:\n");
        err = isds_FindDataBox(ctx, db_owner_info, &boxes);
        if (err == IE_SUCCESS || err == IE_2BIG) {
            if (err == IE_2BIG) 
                printf("isds_FindDataBox() results truncated\n");
            printf("isds_FindDataBox() succeeded:\n");

            for(item = boxes; item; item = item->next) {
                printf("List item:\n");
                print_DbOwnerInfo(item->data);
            }
        } else {
            printf("isds_FindDataBox() failed: %s: %s\n",
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

    isds_DbOwnerInfo_free(&db_owner_info);

    {
        /* Get password expiration time */
        struct timeval *expiration = NULL;
        printf("Getting password expiration time\n");
        err = isds_get_password_expiration(ctx, &expiration);
        if (err)
            printf("isds_get_password_expiration() failed: %s: %s\n",
                    isds_strerror(err), isds_long_message(ctx));
        else {
            printf("isds_get_password_expiration() succeeded: "
                    "Password expires at: ");
            print_timeval(expiration);
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
