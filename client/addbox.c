#define _XOPEN_SOURCE 500
#include <stdlib.h>
#include <stdio.h>
#include <locale.h>
#include <time.h>
#include <string.h>
#include <isds.h>
#include "common.h"

/* Pass e-mail address where to deliver credentials as first argument. */
int main(int argc, char **argv) {
    struct isds_ctx *ctx = NULL;
    isds_error err;
    struct isds_DbOwnerInfo *db_owner_info = NULL;
    struct isds_list *users = NULL;

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

    /* Get info all users of this box */
    if (db_owner_info) {
        struct isds_list *item;
        printf("Getting users of my box with ID `%s':\n", db_owner_info->dbID);
        err = isds_GetDataBoxUsers(ctx, db_owner_info->dbID, &users);
        if (err) {
            printf("isds_GetDataBoxUsers() failed: %s: %s\n",
                    isds_strerror(err), isds_long_message(ctx));
        } else {
            printf("isds_GetDataBoxUsers() succeeded\n");
            for(item = users; item; item = item->next) {
                printf("List item:\n");
                print_DbUserInfo(item->data);
            }
        }
    }


    /* Create the same box */
    if (db_owner_info) {
        char *refnumber = NULL;
        struct isds_credentials_delivery *credentials_delivery = NULL;
        const struct isds_approval approval = {
            .approved = 1, .refference = "Me"
        };

        credentials_delivery = calloc(1, sizeof(*credentials_delivery));
        if (credentials_delivery && argv[1])
            credentials_delivery->email = strdup(argv[1]);

        if (!credentials_delivery || (argv[1] && !credentials_delivery->email))
        {
            printf("Not enough memory\n");
        } else {
            printf("Creating already existing box\n");
            err = isds_add_box(ctx, db_owner_info, users, "Former Names",
                    NULL, "CEO", credentials_delivery, &approval, &refnumber);
            if (err) {
                printf("isds_add_box() failed: %s: %s\n",
                        isds_strerror(err), isds_long_message(ctx));
            } else {
                printf("isds_add_box() succeeded:\n"
                        "\trequest #: %s\n"
                        "\tnew box ID: %s\n"
                        "\tcredentials e-mail: %s\n"
                        "\ttoken value: %s\n",
                        refnumber, db_owner_info->dbID,
                        credentials_delivery->email, credentials_delivery->token);
            }
        }
        isds_credentials_delivery_free(&credentials_delivery);
    }

    isds_DbOwnerInfo_free(&db_owner_info);
    isds_list_free(&users);


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
