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
    const char *request_url = url;
    const char *request_username = username();
    const char *request_password = password();
    _Bool use_otp = 0;
    struct isds_otp otp;
    
    setlocale(LC_ALL, "");

    if (argc > 1) {
        if (argc < 4 || argc > 6) {
            printf("Usage: %s [URL LOGIN PASSWORD [{hotp OTP_CODE | totp [OTP_CODE]]\n",
                    (argv[0])?argv[0]: "");
            exit(EXIT_FAILURE);
        }

        request_url = argv[1];
        request_username = argv[2];
        request_password = argv[3];

        if (argc > 4) {
            use_otp = 1;
            if (!strcmp(argv[4], "hotp")) otp.method = OTP_HASH;
            else if (!strcmp(argv[4], "totp")) otp.method = OTP_TIME;
            else {
                printf("Bad invocation: %s: Unknown OTP method\n", argv[4]);
                exit(EXIT_FAILURE);
            }
            if (argc > 5) otp.otp_code = argv[5];
            else otp.otp_code = NULL;
        }
    }


    err = isds_init();
    if (err) {
        printf("isds_init() failed: %s\n", isds_strerror(err));
        exit(EXIT_FAILURE);
    }

    isds_set_logging(ILF_ALL, ILL_ALL);

    ctx = isds_ctx_create();
    if (!ctx) {
        printf("isds_ctx_create() failed");
    }

    err = isds_set_timeout(ctx, 10000);
    if (err) {
        printf("isds_set_timeout() failed: %s\n", isds_strerror(err));
    }

    err = isds_login(ctx, request_url, request_username, request_password,
            NULL, (use_otp) ? &otp : NULL);
    if (err == IE_PARTIAL_SUCCESS) {
        printf("isds_login() partially succeeded: %s: %s\n", isds_strerror(err),
                isds_long_message(ctx));
        if (use_otp && otp.resolution == OTP_RESOLUTION_TOTP_SENT)
            printf("Redo log-in with OTP code to finish log-in procedure.\n");

    } else if (err) {
        printf("isds_login() failed: %s: %s\n", isds_strerror(err),
                isds_long_message(ctx));
    } else {
        printf("Logged in :)\n");
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
