#define _XOPEN_SOURCE 500   /* strdup from string.h */
#include "isds_priv.h"
#include "utils.h"
#include "gcrypt.h"
#include "ksba.h"

/* Computes hash from @input with @length and store it into @hash.
 * The hash algoritm is defined inside @hash.
 * @input is input block to hash
 * @length is @input block length in bytes
 * @hash input algoritm, output hash value and hash length */
_hidden isds_error compute_hash(const void *input, const size_t length,
        struct isds_hash *hash) {
    int g_algorithm;
    void *buffer;

    if ((length != 0 && !input) || !hash) return IE_INVAL;

    /* Select algorithm */
    switch (hash->algorithm) {
        case HASH_ALGORITHM_MD5:        g_algorithm = GCRY_MD_MD5; break;
        case HASH_ALGORITHM_SHA_1:      g_algorithm = GCRY_MD_SHA1; break;
        case HASH_ALGORITHM_SHA_256:    g_algorithm = GCRY_MD_SHA256; break;
        case HASH_ALGORITHM_SHA_512:    g_algorithm = GCRY_MD_SHA512; break;
        default:                        return IE_NOTSUP;
    }

    /* Test it's available */
    if (gcry_md_test_algo(g_algorithm)) return IE_NOTSUP;

    /* Get known the hash length and allocate buffer for hash value */
    hash->length = gcry_md_get_algo_dlen(g_algorithm);
    buffer = realloc(hash->value, hash->length);
    if (!buffer) return IE_NOMEM;
    hash->value = buffer;

    /* Compute the hash */
    gcry_md_hash_buffer(g_algorithm, hash->value, (length)?buffer:"", length);

    return IE_SUCCESS;
}

