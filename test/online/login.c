#include "../test.h"
#include "libdatovka/isds.h"
#include "common.h"

static int test_login(const isds_error error, struct isds_ctx *context,
        const char *url, const char *username, const char *password,
        const struct isds_pki_credentials *pki_credentials,
        struct isds_otp *otp) {
    isds_error err;

    err = isds_login(context, url, username, password, pki_credentials, otp);
    if (error != err)
        FAIL_TEST("Wrong return code: expected=%s, returned=%s (%s)",
                isds_strerror(error), isds_strerror(err),
                isds_long_message(context));

    isds_logout(context);
    PASS_TEST;
}

static int test_login2(const isds_error error1, const isds_error error2,
        struct isds_ctx *context,
        const char *url, const char *username, const char *password,
        const struct isds_pki_credentials *pki_credentials,
        struct isds_otp *otp) {

    isds_error err =
            isds_login(context, url, username, password, pki_credentials, otp);
    if (err != error1 && err != error2)
        FAIL_TEST("Wrong return code: must_differ=%s, must_differ=%s, "
                "returned=%s (%s)",
                isds_strerror(error1), isds_strerror(error2),
                isds_strerror(err), isds_long_message(context));

    isds_logout(context);
    PASS_TEST;
}


int main(void) {
    INIT_TEST("login");

    struct isds_ctx *context = NULL;
    const char *url = isds_testing_locator;

    if (isds_init())
        ABORT_UNIT("isds_init() failed\n");
    context = isds_ctx_create();
    if (!context)
        ABORT_UNIT("isds_ctx_create() failed\n");


    TEST("invalid context", test_login, IE_INVALID_CONTEXT, NULL,
            url, username(), password(), NULL, NULL);
    TEST("NULL url with invalid credentials", test_login, IE_NOT_LOGGED_IN,
            context, NULL, username(), password(), NULL, NULL);
    TEST("NULL username", test_login, IE_INVAL, context,
            url, NULL, password(), NULL, NULL);
    TEST("NULL password", test_login, IE_INVAL, context,
            url, username(), NULL, NULL, NULL);

    TEST("invalid URL", test_login, IE_NETWORK, context,
            "invalid://", username(), password(), NULL, NULL);
    /* Direct connection fails on local resolution, connection trough proxy
     * fails on HTTP code */
    TEST("unresolvable host name", test_login2, IE_NETWORK, IE_HTTP, context,
            "http://unresolvable.example.com/", username(), password(),
            NULL, NULL);

    TEST("invalid credentials", test_login, IE_NOT_LOGGED_IN, context,
            url, "7777777", "nbuusr1", NULL, NULL);

    TEST("valid login", test_login, IE_SUCCESS, context,
            url, username(), password(), NULL, NULL);

    isds_ctx_free(&context);
    isds_cleanup();

    SUM_TEST();
}
