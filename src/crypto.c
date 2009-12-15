#define _XOPEN_SOURCE 500   /* strdup from string.h */
#include "isds_priv.h"
#include "utils.h"
#include <stdlib.h>
#include <string.h>

/* Computes hash from @input with @length and store it into @hash.
 * The hash algoritm is defined inside @hash.
 * @input is input block to hash
 * @length is @input block length in bytes
 * @hash input algoritm, output hash value and hash length */
_hidden isds_error compute_hash(const void *input, const size_t length,
        struct isds_hash *hash) {
    if (!input || !hash) return IE_INVAL;

    return IE_NOTSUP;
}

