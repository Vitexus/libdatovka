#include "test.h"
#include "isds.h"

static int test_isds_hash_free(
        struct isds_hash **hash) {
    isds_hash_free(hash);
    if (!hash) PASS_TEST;

    if (*hash)
        FAIL_TEST("isds_hash_free() did not null pointer");

    PASS_TEST;
}


int main(int argc, char **argv) {

    INIT_TEST("isds_hash_free()");
    if (isds_init())
        ABORT_UNIT("isds_init() failed");
    
    struct isds_hash *hash = NULL;
    TEST("NULL", test_isds_hash_free, NULL);
    TEST("*NULL", test_isds_hash_free, &hash);

    hash = calloc(1, sizeof(*hash));
    if (!hash) ABORT_UNIT("No enough memory");
    TEST("Empty structure", test_isds_hash_free, &hash);

    hash = calloc(1, sizeof(*hash));
    if (!hash) ABORT_UNIT("No enough memory");
    TEST_FILL_STRING(hash->value);
    TEST("Full structure", test_isds_hash_free, &hash);

    isds_cleanup();
    SUM_TEST();
}
