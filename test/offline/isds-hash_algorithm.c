#include "../test.h"
#include "isds.c"

static int test_string2hashalgorithm(const xmlChar *name,
        const isds_error error, const isds_hash_algorithm type) {
    isds_error err;
    isds_hash_algorithm new_type = -1; /* ??? GCC-4.7.3 at -O3 complains on
                                          possibly undefined new_type in the
                                          `type != new_type' code. */

    err = string2isds_hash_algorithm(name, &new_type);
    if (err != error)
        FAIL_TEST("string2isds_hash_algorithm() returned unexpected code");

    if (err)
        PASS_TEST;

    if (type != new_type)
        FAIL_TEST("conversion returned wrong algorithm");

    PASS_TEST;
}

int main(void) {
    INIT_TEST("isds_hash_algorithm conversion");

    xmlChar *names[] = {
        BAD_CAST "MD5",
        BAD_CAST "SHA-1",
        BAD_CAST "SHA-224",
        BAD_CAST "SHA-256",
        BAD_CAST "SHA-384",
        BAD_CAST "SHA-512"
    };

    isds_hash_algorithm algos[] =  {
        HASH_ALGORITHM_MD5,
        HASH_ALGORITHM_SHA_1,
        HASH_ALGORITHM_SHA_224,
        HASH_ALGORITHM_SHA_256,
        HASH_ALGORITHM_SHA_384,
        HASH_ALGORITHM_SHA_512,
    };

    for (size_t i = 0; i < sizeof(algos)/sizeof(algos[0]); i++)
        TEST(names[i], test_string2hashalgorithm, names[i], IE_SUCCESS,
                algos[i]);

    TEST("X-Invalid_Type", test_string2hashalgorithm, BAD_CAST "X-Invalid_Type",
            IE_ENUM, 0);

    SUM_TEST();
}
