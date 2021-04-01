#define _XOPEN_SOURCE 600
#include <stdlib.h>
#include <stdio.h>
#include <locale.h>
#include <time.h>
#include <string.h>
#include <libdatovka/isds.h>
#include "common.h"

#define TLS_PREFIX SRCDIR "/server/tls/"
#define NSS_DIR TLS_PREFIX "client_nss"

void usage(const char *command) {
    const char *name = NULL;
    if (command) {
        name = strrchr(command, '/');
        if (name) name++;
    }
    if (!name) name = command;

    fprintf(stderr, "Usage: %s {openssl|nss} {sw|hw ID}\n"
            "\tID\tIdentifier of cryptographic material in hardware engine\n",
            name);
    exit(EXIT_FAILURE);
}

int main(int argc, char **argv) {
    struct isds_ctx *ctx = NULL;
    isds_error err;
    struct isds_pki_credentials *pki_credentials = NULL;
    _Bool use_nss = 0;

    /* Software: OpenSSL, GnuTLS */
    struct isds_pki_credentials pki_software_ossl = {
        .engine = NULL,
        .passphrase = NULL,
        .key_format = PKI_FORMAT_PEM,
        .key = TLS_PREFIX "client.key",
        .certificate_format = PKI_FORMAT_PEM,
        .certificate = TLS_PREFIX "client.cert"
    };

    /* Software: NSS */
    struct isds_pki_credentials pki_software_nss = {
        .engine = NULL,
        .passphrase = NULL,
        .key_format = PKI_FORMAT_PEM,
        .key = NULL,
        .certificate_format = PKI_FORMAT_PEM,
        .certificate = "The Client Material"
    };

    /* Hardware engine: OpenSSL */
    struct isds_pki_credentials pki_hardware_ossl = {
        .engine = "pkcs11",
        .passphrase = NULL,
        .key_format = PKI_FORMAT_ENG,
        .key = "id_45",
        .certificate_format = PKI_FORMAT_ENG,
        .certificate = NULL
    };

    /* Hardware engine: NSS */
    struct isds_pki_credentials pki_hardware_nss = {
        .engine = NULL,
        .passphrase = NULL,
        .key_format = PKI_FORMAT_PEM,
        .key = NULL,
        .certificate_format = PKI_FORMAT_PEM,
        .certificate = "OpenSC Card (Bob Tester):Certificate"
    };

    setlocale(LC_ALL, "");

    /* Parse arguments */
    if (argc < 3 || !argv[1] || !argv[2]) usage(argv[0]);
    if (!strcmp(argv[1], "openssl")) {
        use_nss = 0;
        if (!strcmp(argv[2], "sw")) pki_credentials = &pki_software_ossl;
        else if (!strcmp(argv[2], "hw")) {
            pki_credentials = &pki_hardware_ossl;
            if (argc < 4 || !argv[3]) usage(argv[0]);
            pki_credentials->key = argv[3];
        } else usage(argv[0]);
    } else if (!strcmp(argv[1], "nss")) {
        use_nss = 1;
        if (!strcmp(argv[2], "sw")) pki_credentials = &pki_software_nss;
        else if (!strcmp(argv[2], "hw")) {
            pki_credentials = &pki_hardware_nss;
            if (argc < 4 || !argv[3]) usage(argv[0]);
            pki_credentials->certificate = argv[3];
        } else usage(argv[0]);
    } else
        usage(argv[0]);

    /* ISDS stuff */
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

    /* err = isds_set_opt(ctx, IOPT_TLS_VERIFY_SERVER, 0);
    if (err) {
        printf("isds_set_opt(IOPT_TLS_VERIFY_SERVER) failed: %s\n",
                isds_strerror(err));
    }*/


    if (use_nss) {
        if (setenv("SSL_DIR", NSS_DIR, 0)) {
            printf("setenv(\"SSL_DIR\", \"%s\") failed\n", NSS_DIR);
        }
    } else {
        err = isds_set_opt(ctx, IOPT_TLS_CA_FILE, TLS_PREFIX "ca.cert");
        if (err) {
            printf("isds_set_opt(IOPT_TLS_CA_FILE) failed: %s\n",
                    isds_strerror(err));
        }
    }

    err = isds_login(ctx, "https://localhost:1443/", username(), password(),
            pki_credentials, NULL);
    if (err) {
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
