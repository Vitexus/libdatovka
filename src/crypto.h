#ifndef __ISDS_CRYPTO_H__
#define __ISDS_CRYPTO_H__

#include "isds.h"

/* Initialise all cryptographic libraries which libisds depends on.
 * @return IE_SUCCESS if everything went all-right. */
isds_error _isds_init_crypto(void);

/* Computes hash from @input with @length and store it into @hash.
 * The hash algorithm is defined inside @hash.
 * @input is input block to hash
 * @length is @input block length in bytes
 * @hash input algorithm, output hash value and hash length; hash value will be
 * reallocated, it's always valid pointer or NULL (before and after call) */
isds_error _isds_compute_hash(const void *input, const size_t length,
        struct isds_hash *hash);

/* Free CMS data buffer allocated inside _isds_extract_cms_data().
 * This is necessary because GPGME.
 * @buffer is pointer to memory to free */
void _isds_cms_data_free(void *buffer);

/* Extract data from CMS (successor of PKCS#7)
 * The CMS' signature is is not verified.
 * @context is session context
 * @cms is input block with CMS structure
 * @cms_length is @cms block length in bytes
 * @data are automatically reallocated bit stream with data found in @cms
 * You must free them with _isds_cms_data_free().
 * @data_length is length of @data in bytes */
isds_error _isds_extract_cms_data(struct isds_ctx *context,
        const void *cms, const size_t cms_length,
        void **data, size_t *data_length);

#endif
