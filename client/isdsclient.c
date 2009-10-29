#include <stdlib.h>
#include <stdio.h>
#include <locale.h>
#include <isds.h>

char url[] = "https://www.czebox.cz/DS/";
char username[] = "jrfh7i";
char password[] = "Ab123456";
struct isds_ctx *ctx;


int main(int argc, char **argv) {
    isds_error err;
    
    setlocale(LC_ALL, "");

    err = isds_init();
    if (err) {
        printf("isds_init() failed: %s\n", isds_strerror(err));
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

    err = isds_login(ctx, url, username, password, NULL, NULL);
    if (err) {
        printf("isds_login() failed: %s: %s\n", isds_strerror(err),
                isds_long_message(ctx));
    } else {
        printf("Logged in :)\n");
    }

    err = isds_ping(ctx);
    if (err) {
        printf("isds_ping() failed: %s\n", isds_strerror(err));
    } else {
        printf("isds_ping() succeeded\n");
    }

    printf("Sending bogus request\n");
    err = isds_bogus_request(ctx);
    if (err) {
        printf("isds_bogus_request() failed: %s: %s\n", isds_strerror(err),
                isds_long_message(ctx));
    } else {
        printf("isds_bogus_request() succeeded\n");
    }

    {
        struct isds_DbOwnerInfo *db_owner_info = NULL;

        err = isds_GetOwnerInfoFromLogin(ctx, &db_owner_info);
        if (err) {
            printf("isds_GetOwnerInfoFromLogin() failed: %s: %s\n",
                    isds_strerror(err), isds_long_message(ctx));
        } else {
            printf("isds_GetOwnerInfoFromLogin() succeeded\n");
        }

        isds_DbOwnerInfo_free(&db_owner_info);
    }


    err = isds_logout(ctx);
    if (err) {
        printf("isds_logout() failed: %s\n", isds_strerror(err));
    }

    printf("Ping after logout should fail\n");
    err = isds_ping(ctx);
    if (err) {
        printf("isds_ping() failed: %s\n", isds_strerror(err));
    } else {
        printf("isds_ping() succeeded\n");
    }

    printf("Sending bogus request after logout\n");
    err = isds_bogus_request(ctx);
    if (err) {
        printf("isds_bogus_request() failed: %s: %s\n", isds_strerror(err),
                isds_long_message(ctx));
    } else {
        printf("isds_bogus_request() succeeded\n");
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
