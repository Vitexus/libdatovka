#ifndef _POSIX_SOURCE
#define _POSIX_SOURCE   /* For getaddrinfo(3) */
#endif

#ifndef _BSD_SOURCE
#define _BSD_SOURCE   /* For NI_MAXHOST up to glibc-2.19 */
#endif
#ifndef _DEFAULT_SOURCE
#define _DEFAULT_SOURCE   /* For NI_MAXHOST since glibc-2.20 */
#endif

#ifndef _XOPEN_SOURCE
#define _XOPEN_SOURCE 600   /* For unsetenv(3) */
#endif

#include "../test.h"
#include "server.h"
#include "isds.h"

#define TLSDIR SRCDIR "/server/tls"
static const char *ca_certificate = TLSDIR "/ca.cert";
static char *server_certificate = TLSDIR "/server.cert";
static char *server_key = TLSDIR "/server.key";
static char *client_certificate = TLSDIR "/client.cert";
static char *client_key = TLSDIR "/client.key";
static const char *client_dn = "CN=The Client,C=CZ";
static const char *username = "douglas";
static const char *password = "42";


static int test_login(const isds_error error, struct isds_ctx *context,
        const char *url, const char *username, const char *password,
        const struct isds_pki_credentials *pki_credentials,
        struct isds_otp *otp) {
    isds_error err;

    err = isds_login(context, url, username, password, pki_credentials, otp,
            NULL);
    /* If TLSv1.3 is used, cURL reports a network error instead of a security
     * error if server refuses client's certificate (since GnuTLS 3.6.4).
     * Maybe a <https://gitlab.com/gnutls/gnutls/issues/615>. As a workaround,
     * accept IE_NETWORK if IE_SECURITY was expected. */
    if (error != err && (IE_SECURITY != error || IE_NETWORK != err))
        FAIL_TEST("Wrong return code: expected=%s, returned=%s (%s)",
                isds_strerror(error), isds_strerror(err),
                isds_long_message(context));

    isds_logout(context);
    PASS_TEST;
}

int main(void) {
    int error;
    pid_t server_process;
    struct isds_ctx *context = NULL;
    char *url = NULL;

    INIT_TEST("authentication with client certificate and username and "
            "password");

    if (unsetenv("http_proxy")) {
        ABORT_UNIT("Could not remove http_proxy variable from environment\n");
    }
    if (isds_init()) {
        isds_cleanup();
        ABORT_UNIT("isds_init() failed\n");
    }
    context = isds_ctx_create();
    if (!context) {
        isds_cleanup();
        ABORT_UNIT("isds_ctx_create() failed\n");
    }
    if (isds_set_opt(context, IOPT_TLS_CA_FILE, ca_certificate)) {
        isds_ctx_free(&context);
        isds_cleanup();
        ABORT_UNIT("Setting CA failed\n");
    }
    if (isds_set_opt(context, IOPT_TLS_VERIFY_SERVER, 0)) {
        isds_ctx_free(&context);
        isds_cleanup();
        ABORT_UNIT("Disabling server hostname verification failed\n");
    }

    {
        const struct service_configuration services[] = {
            { SERVICE_DS_Dz_DummyOperation, NULL },
            { SERVICE_END, NULL }
        };
        const struct arguments_basic_authentication server_arguments = {
            .username = username,
            .password = password,
            .isds_deviations = 1,
            .services = services
        };
        struct tls_authentication tls_arguments = {
            .authority_certificate = ca_certificate,
            .server_certificate = server_certificate,
            .server_key = server_key,
            .client_name = client_dn
        };
        struct isds_pki_credentials pki_credentials = {
            .engine = NULL,
            .certificate_format = PKI_FORMAT_PEM,
            .certificate = server_certificate,
            .key_format = PKI_FORMAT_PEM,
            .key = server_key,
            .passphrase = NULL
        };
        error = start_server(&server_process, &url,
                server_certificate_with_password_authentication,
                &server_arguments, &tls_arguments);
        if (error == -1) {
            isds_ctx_free(&context);
            isds_cleanup();
            ABORT_UNIT(server_error);
        }

        TEST("no client certificate", test_login, IE_SECURITY, context,
                url, username, password, NULL, NULL);

        TEST("wrong client certificate", test_login, IE_SECURITY, context,
                url, username, password, &pki_credentials, NULL);

        pki_credentials.certificate = client_certificate;
        pki_credentials.key = client_key;

        TEST("invalid username", test_login, IE_NOT_LOGGED_IN, context,
                url, "7777777", "nbuusr1", &pki_credentials, NULL);

        TEST("valid login", test_login, IE_SUCCESS, context,
                url, username, password, &pki_credentials, NULL);

        if (stop_server(server_process)) {
            isds_ctx_free(&context);
            isds_cleanup();
            ABORT_UNIT(server_error);
        }
    
        free(url);
        url = NULL;
    }

    {
        struct tls_authentication tls_arguments = {
            .authority_certificate = ca_certificate,
            .server_certificate = server_certificate,
            .server_key = server_key,
            .client_name = client_dn
        };
        struct isds_pki_credentials pki_credentials = {
            .engine = NULL,
            .certificate_format = PKI_FORMAT_PEM,
            .certificate = client_certificate,
            .key_format = PKI_FORMAT_PEM,
            .key = client_key,
            .passphrase = NULL
        };
        error = start_server(&server_process, &url,
                server_out_of_order, NULL, &tls_arguments);
        if (error == -1) {
            isds_ctx_free(&context);
            isds_cleanup();
            ABORT_UNIT(server_error);
        }

        TEST("log into out-of-order server", test_login, IE_SOAP, context,
                url, username, password, &pki_credentials, NULL);

        if (stop_server(server_process)) {
            isds_ctx_free(&context);
            isds_cleanup();
            ABORT_UNIT(server_error);
        }
    
        free(url);
        url = NULL;
    }

    isds_ctx_free(&context);
    isds_cleanup();
    SUM_TEST();
}
