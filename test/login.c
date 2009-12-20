#include "test.h"
#include "isds.h"

static int test_login(const isds_error error, struct isds_ctx *context,
        const char *url, const char *username, const char *password,
        const char *certificate, const char *key) {

    if (error !=
            isds_login(context, url, username, password, certificate, key))
        FAIL_TEST("Wrong return code");

    isds_logout(context);
    PASS_TEST;
}

static int test_login2(const isds_error error1, const isds_error error2,
        struct isds_ctx *context,
        const char *url, const char *username, const char *password,
        const char *certificate, const char *key) {
    
    isds_error err =
            isds_login(context, url, username, password, certificate, key);
    if (err != error1 && err != error2)
        FAIL_TEST("Wrong return code");

    isds_logout(context);
    PASS_TEST;
}


int main(int argc, char **argv) {
    INIT_TEST("login");
    
    struct isds_ctx *context = NULL;
    char url[] = "https://www.czebox.cz/DS/";
    char username[] = "jrfh7i";
    char password[] = "Ab123456";

    if (isds_init()) {
        printf("isds_init() failed\n");
        exit(EXIT_FAILURE);
    }
    context = isds_ctx_create();
    if (!context) {
        printf("isds_ctx_create() failed\n");
        exit(EXIT_FAILURE);
    }


    TEST("invalid context", test_login, IE_INVALID_CONTEXT, NULL,
            url, username, password, NULL, NULL);
    TEST("NULL url", test_login, IE_INVAL, context,
            NULL, username, password, NULL, NULL);
    TEST("NULL username", test_login, IE_INVAL, context,
            url, NULL, password, NULL, NULL);
    TEST("NULL password", test_login, IE_INVAL, context,
            url, username, NULL, NULL, NULL);

    TEST("invalid URL", test_login, IE_NETWORK, context,
            "invalid://", username, password, NULL, NULL);
    /* Direct connection fails on local resolution, connection trough proxy
     * failes on HTTP code */
    TEST("unresolvable host name", test_login2, IE_NETWORK, IE_SOAP, context,
            "http://unresolvable.example.com/", username, password,
            NULL, NULL);

    TEST("invalid credentials", test_login, IE_NOT_LOGGED_IN, context,
            url, "7777777", "nbuusr1", NULL, NULL);

    TEST("valid login", test_login, IE_SUCCESS, context,
            url, username, password, NULL, NULL);

    isds_ctx_free(&context);
    isds_cleanup();

    SUM_TEST();
}
