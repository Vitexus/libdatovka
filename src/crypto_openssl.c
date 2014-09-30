
#include <assert.h>
#include <locale.h>
#include <openssl/err.h>
#include <openssl/md5.h>
#include <openssl/sha.h>

#include "isds_priv.h"
#include "utils.h"


#ifndef SHA1_DIGEST_LENGTH
#  define SHA1_DIGEST_LENGTH 20
#endif /* !SHA1_DIGEST_LENGTH */


/* Initialise all cryptographic libraries which libisds depends on.
 * @return IE_SUCCESS if everything went all-right. */
_hidden isds_error _isds_init_crypto_openssl(void)
{
    ERR_load_crypto_strings();

    return IE_SUCCESS;
}

/* Computes hash from @input with @length and store it into @hash.
 * The hash algorithm is defined inside @hash.
 * @input is input block to hash
 * @length is @input block length in bytes
 * @hash input algorithm, output hash value and hash length; hash value will be
 * reallocated, it's always valid pointer or NULL (before and after call) */
_hidden isds_error _isds_compute_hash_openssl(const void *input, const size_t length,
        struct isds_hash *hash)
{
    void *hash_buf = NULL;
    size_t hash_len = 0;
    unsigned char * (*hash_func)(const unsigned char *, size_t n,
        unsigned char *) = NULL;

    if (((0 != length) && (NULL == input)) || (NULL == hash)) {
        return IE_INVAL;
    }

    isds_log(ILF_SEC, ILL_DEBUG,
            _("Data hash requested, length=%zu, content:\n%*s\n"
                "End of data to hash\n"), length, length, input);

    /* Select algorithm */
    switch (hash->algorithm) {
    case HASH_ALGORITHM_MD5:
        hash_len = MD5_DIGEST_LENGTH;
        hash_func = MD5;
        break;
    case HASH_ALGORITHM_SHA_1:
        hash_len = SHA1_DIGEST_LENGTH;
        hash_func = SHA1;
        break;
    case HASH_ALGORITHM_SHA_224:
        hash_len = SHA224_DIGEST_LENGTH;
        hash_func = SHA224;
        break;
    case HASH_ALGORITHM_SHA_256:
        hash_len = SHA256_DIGEST_LENGTH;
        hash_func = SHA256;
        break;
    case HASH_ALGORITHM_SHA_384:
        hash_len = SHA384_DIGEST_LENGTH;
        hash_func = SHA384;
        break;
    case HASH_ALGORITHM_SHA_512:
        hash_len = SHA512_DIGEST_LENGTH;
        hash_func = SHA512;
        break;
    default:
        return IE_NOTSUP;
    }

    assert(0 != hash_len);

    /* Get known the hash length and allocate buffer for hash value */
    hash->length = hash_len;
    hash_buf = realloc(hash->value, hash->length);
    if (NULL == hash_buf) {
        return IE_NOMEM;
    }
    hash->value = hash_buf;

    assert(NULL != hash->value);
    assert(NULL != hash_func);

    /* Compute the hash */
    hash_func(input, length, hash->value);

    return IE_SUCCESS;
}

/* Free CMS data buffer allocated inside _isds_extract_cms_data().
 * This is necessary because GPGME.
 * @buffer is pointer to memory to free */
_hidden void _isds_cms_data_free_openssl(void *buffer)
{
}

/* Extract data from CMS (successor of PKCS#7)
 * @context is session context
 * @cms is input block with CMS structure
 * @cms_length is @cms block length in bytes
 * @data are automatically reallocated bit stream with data found in @cms
 * You must free them with _isds_cms_data_free().
 * @data_length is length of @data in bytes */
_hidden isds_error _isds_extract_cms_data_openssl(struct isds_ctx *context,
        const void *cms, const size_t cms_length,
        void **data, size_t *data_length)
{
    assert(NULL != context);

    if ((NULL == cms) || (0 == cms_length) ||
        (NULL == data) || (NULL == data_length)) {
        return IE_INVAL;
    }

    /* TODO */

    return IE_ERROR;
}
