
#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "crypto.h"
#include "utils.h"


/*
 * Testing via result comparison.
 * The only purpose of this file is to test the two cryptographic back-ends.
 * This functions are not intended to be used in production.
 * */


isds_error _isds_init_crypto_gpg(void);
isds_error _isds_init_crypto_openssl(void);

_hidden isds_error _isds_init_crypto(void)
{
    if (IE_SUCCESS != _isds_init_crypto_gpg()) {
        return IE_ERROR;
    }

    if (IE_SUCCESS != _isds_init_crypto_openssl()) {
        return IE_ERROR;
    }

    return IE_SUCCESS;
}


isds_error _isds_compute_hash_gpg(const void *input, const size_t length,
        struct isds_hash *hash);
isds_error _isds_compute_hash_openssl(const void *input, const size_t length,
        struct isds_hash *hash);

_hidden isds_error _isds_compute_hash(const void *input, const size_t length,
        struct isds_hash *hash)
{
    isds_error retval_gpg = IE_SUCCESS;
    isds_error retval_openssl = IE_SUCCESS;
    isds_error retval = IE_SUCCESS;
    void *orig_hash_val = NULL;
    size_t orig_hash_len = 0;

    retval_gpg = _isds_compute_hash_gpg(input, length, hash);

    orig_hash_val = hash->value; hash->value = NULL;
    orig_hash_len = hash->length; hash->length = 0;

    retval_openssl = _isds_compute_hash_openssl(input, length, hash);

    if (retval_gpg != retval_openssl) {
        fprintf(stderr, "%s: Return values differ.\n", __func__);
        assert(0);
        retval = IE_ERROR;
        goto fail;
    }

    if (IE_SUCCESS != retval_gpg) {
       retval = retval_gpg;
       goto fail;
    }

    if (orig_hash_len != hash->length) {
        fprintf(stderr, "%s: Hash size differs %lu %lu.\n", __func__,
            orig_hash_len, hash->length);
        assert(0);
        retval = IE_ERROR;
        goto fail;
    }

    if (orig_hash_val == hash->value) {
        fprintf(stderr, "%s: Hashes are in the same location.\n", __func__);
        assert(0);
        retval = IE_ERROR;
        goto fail;
    }

    if (0 != memcmp(orig_hash_val, hash->value, hash->length)) {
        fprintf(stderr, "%s: Hash value differ.\n", __func__);
        assert(0);
        retval = IE_ERROR;
        goto fail;
    }

    free(orig_hash_val); orig_hash_val = NULL; orig_hash_len = 0;

    return IE_SUCCESS;

fail:
    if (NULL != orig_hash_val) {
        free(orig_hash_val);
    }
    return retval;
}


void _isds_cms_data_free_gpg(void *buffer);
void _isds_cms_data_free_opnssl(void *buffer);

_hidden void _isds_cms_data_free(void *buffer)
{
    _isds_cms_data_free_gpg(buffer);
}


isds_error _isds_extract_cms_data_gpg(struct isds_ctx *context,
        const void *cms, const size_t cms_length,
        void **data, size_t *data_length);
isds_error _isds_extract_cms_data_openssl(struct isds_ctx *context,
        const void *cms, const size_t cms_length,
        void **data, size_t *data_length);

_hidden isds_error _isds_extract_cms_data(struct isds_ctx *context,
        const void *cms, const size_t cms_length,
        void **data, size_t *data_length)
{
    return _isds_extract_cms_data_gpg(context, cms, cms_length, data,
        data_length);
}
