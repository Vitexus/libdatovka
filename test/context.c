#include "test.h"
#include "isds.h"

static int test_create(struct isds_ctx **context) {
    if (!context)
        FAIL_TEST("Bad invocation");

    *context = isds_ctx_create();
    if (!*context) 
        FAIL_TEST("isds_ctx_create() failed");

    PASS_TEST;
}

static int test_free(const isds_error error, struct isds_ctx **context) {
    if (!context)
        FAIL_TEST("Bad invocation");

    if (error != isds_ctx_free(context))
        FAIL_TEST("Wrong return code");

    if (*context) 
        FAIL_TEST("context not NULLed");

    PASS_TEST;
}


int main(int argc, char **argv) {
    INIT_TEST("context");
    
    struct isds_ctx *context = NULL;

    if (isds_init()) {
        printf("isds_init() failed\n");
        exit(EXIT_FAILURE);
    }

    TEST("create", test_create, &context);
    TEST("free valid context", test_free, IE_SUCCESS, &context);

    context = NULL;
    TEST("free invalid context", test_free, IE_INVALID_CONTEXT, &context);

    isds_cleanup();

    SUM_TEST();
}
