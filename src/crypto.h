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

/* Inicialize GPGME.
 * @return IE_SUCCESS if everything is O.k. */
isds_error init_gpgme(void);

/* Free CMS data buffer allocated inside extract_cms_data().
 * This is necesary because GPGME.
 * @buffer is pointer to memory to free */
void cms_data_free(void *buffer);

/* Extract data from CMS (successor of PKCS#7)
 * @context is session context
 * @cms is input block with CMS structure
 * @cms_length is @cms block length in bytes
 * @data are automatically reallocated bit stream with data found in @cms
 * You must free them with cms_data_free().
 * @data_length is length of @data in bytes */
isds_error extract_cms_data(struct isds_ctx *context,
        const void *cms, const size_t cms_length,
        void **data, size_t *data_length);

#endif
