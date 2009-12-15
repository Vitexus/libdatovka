#ifndef __ISDS_CRYPTO_H__
#define __ISDS_CRYPTO_H__

#include "isds.h"

/* Computes hash from @input with @length and store it into @hash.
 * The hash algoritm is defined inside @hash.
 * @input is input block to hash
 * @length is @input block length in bytes
 * @hash input algoritm, output hash value and hash length */
isds_error compute_hash(const void *input, const size_t length,
        struct isds_hash *hash);

#endif
