#define _XOPEN_SOURCE 500
#include <stdlib.h>
#include <stdio.h>
#include <locale.h>
#include <time.h>
#include <string.h>
#include <isds.h>
#include "common.h"

void change_password(struct isds_ctx *ctx,
        const char *old_password, const char *new_password) {
    printf("Changing password to: `%s'\n", new_password);
    if (old_password && new_password && !strcmp(old_password, new_password))
        printf("(Same as old password)\n");
    if (!old_password)
        printf("(Old password omitted)\n");
    isds_error err = isds_change_password(ctx, old_password, new_password,
            NULL, NULL);
    if (err) {
        printf("isds_change_password() failed: %s: %s\n",
                isds_strerror(err), isds_long_message(ctx));
    } else {
        printf("isds_change_password() succeeded.\n"
                "This should not happen");
    }
    printf("\n");
}

int main(void) {
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

    err = isds_login(ctx, url, username(), password(), NULL, NULL);
    if (err) {
        printf("isds_login() failed: %s: %s\n", isds_strerror(err),
                isds_long_message(ctx));
    } else {
        printf("Logged in :)\n");
    }


    /* Try some invalid invocations that should fail */
    change_password(ctx, password(), NULL);
    change_password(ctx, password(), "");
    change_password(ctx, password(), password());



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
