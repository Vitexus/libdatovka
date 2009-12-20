#include "test.h"
#include "crypto.h"
#include <string.h>

static int cmp_hash (const struct isds_hash *h1, const struct isds_hash *h2) {
    if (h1 == NULL && h2 == NULL) PASS_TEST;
    if (!(h1 != NULL && h2 != NULL))
        FAIL_TEST("Hash structure is NULL and shouldn't be or vice versa");
    if (h1->algorithm != h2->algorithm)
        FAIL_TEST("Wrong hash algorithm");

    if (h1->length != h2->length) 
        FAIL_TEST("Wrong hash length");
    for (int i = 0; i < h1->length; i++) {
        if (((uint8_t *) (h1->value))[i] != ((uint8_t *) (h2->value))[i])
            FAIL_TEST("Wrong hash value");
    }
    PASS_TEST;
}

static int test_compute_hash(const isds_error error,
        const struct isds_hash *correct, struct isds_hash *test, const void *input,
        const size_t input_length) {
    isds_error err;
    int status;

    if (!correct || !test) return 1;

    err = compute_hash(input, input_length, test);
    if (err != error) {
        free(test->value); test->value = NULL;
        FAIL_TEST("Wrong return value");
    }

    if (!err) {
        status = cmp_hash(correct, test);
    } else {
        status = 0;
    }

    free(test->value); test->value = NULL;
    return (status);
}

int main(int argc, char **argv) {

    INIT_TEST("compute_hash");

    if (init_gcrypt())
        ABORT_UNIT("init_gcrypt() failed");

    char input[] = "42";
    struct isds_hash test = {
        .algorithm = HASH_ALGORITHM_SHA_1,
        .length = 0,
        .value = NULL
    };
    struct isds_hash correct = {
        .algorithm = HASH_ALGORITHM_SHA_1,
        .length = 20,
        .value = (uint8_t[]) {
            0x27, 0x45, 0x34, 0x43, 0x27, 0x4a, 0x2d, 0x9c, 0xe2, 0xca,
            0xfe, 0x6f, 0xe0, 0x16, 0x3a, 0x4a, 0x32, 0x74, 0x62, 0xaa
        }
    };
    TEST("standard SHA1", test_compute_hash, IE_SUCCESS, &correct, &test,
            input, sizeof(input));

    TEST("NULL input", test_compute_hash, IE_INVAL, &correct, &test, NULL, 1);

    correct.value = (uint8_t[]) {
        0xda, 0x39, 0xa3, 0xee, 0x5e, 0x6b, 0x4b, 0x0d, 0x32, 0x55,
        0xbf, 0xef, 0x95, 0x60, 0x18, 0x90, 0xaf, 0xd8, 0x07, 0x09
    };
    test.algorithm = HASH_ALGORITHM_SHA_1;
    TEST("zero length with NULL input",
            test_compute_hash, IE_SUCCESS, &correct, &test, NULL, 0);
    
    correct.algorithm = HASH_ALGORITHM_MD5;
    correct.length = 16;
    correct.value = (uint8_t[]) {
        0x67, 0x54, 0x80, 0xc6, 0x5a, 0x65, 0x57, 0x6e,
        0xdf, 0xf9, 0x26, 0xa7, 0xf0, 0xa8, 0x25, 0x70
    };
    test.algorithm = HASH_ALGORITHM_MD5;
    TEST("MD5", test_compute_hash, IE_SUCCESS, &correct, &test, input,
            sizeof(input));

    correct.algorithm = HASH_ALGORITHM_SHA_256;
    correct.length = 32;
    correct.value = (uint8_t[]) {
        0xe0, 0x8d, 0xe2, 0x65, 0x65, 0x32, 0xc1, 0x05,
        0x88, 0xb5, 0xbf, 0x37, 0x06, 0x0f, 0x4c, 0xc6,
        0xec, 0xff, 0x71, 0x7a, 0xa3, 0x93, 0x54, 0x59,
        0x4e, 0x92, 0x57, 0xc1, 0x8b, 0x47, 0xb7, 0x5a
    };
    test.algorithm = HASH_ALGORITHM_SHA_256;
    TEST("SHA-256", test_compute_hash, IE_SUCCESS, &correct, &test, input,
            sizeof(input));

    correct.algorithm = HASH_ALGORITHM_SHA_512;
    correct.length = 64;
    correct.value = (uint8_t[]) {
        0x37, 0x39, 0x43, 0x2c, 0x23, 0x33, 0x2b, 0x88,
        0x8d, 0x09, 0xfb, 0xab, 0xf5, 0x51, 0x8f, 0xb5,
        0x0b, 0xf1, 0xc8, 0x7a, 0x7f, 0x28, 0x23, 0xcb,
        0xf2, 0x64, 0xf3, 0xae, 0x73, 0xa8, 0xf1, 0x75,
        0x88, 0xc6, 0x3f, 0xc4, 0xfd, 0x25, 0x5b, 0x4c,
        0xbc, 0xdb, 0x0d, 0x0d, 0xb1, 0x05, 0x15, 0x4f,
        0x7c, 0x63, 0xd8, 0xb0, 0xe2, 0x6f, 0x31, 0x81,
        0x41, 0xec, 0xec, 0xd1, 0x02, 0x45, 0x6b, 0xf6
    };
    test.algorithm = HASH_ALGORITHM_SHA_512;
    TEST("SHA-512", test_compute_hash, IE_SUCCESS, &correct, &test, input,
            sizeof(input));

    SUM_TEST();
}
