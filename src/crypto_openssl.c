
#include <assert.h>
#include <locale.h>
#include <openssl/bio.h>
#include <openssl/cms.h>
#include <openssl/crypto.h>
#include <openssl/err.h>
#include <openssl/evp.h>
#include <openssl/pkcs7.h>
#include <openssl/x509.h>

#include "isds_priv.h"
#include "utils.h"


#ifndef SHA1_DIGEST_LENGTH
#  define SHA1_DIGEST_LENGTH 20
#endif /* !SHA1_DIGEST_LENGTH */

#if OPENSSL_VERSION_NUMBER < 0x10100000L
static EVP_MD_CTX *EVP_MD_CTX_new(void) {
    EVP_MD_CTX *mdctx = malloc(sizeof(*mdctx));
    if (NULL != mdctx) {
        EVP_MD_CTX_init(mdctx);
    }
    return mdctx;
}

static void EVP_MD_CTX_free(EVP_MD_CTX *mdctx) {
    EVP_MD_CTX_cleanup(mdctx);
    free(mdctx);
}
#endif

/* Initialise all cryptographic libraries which this library depends on.
 * @return IE_SUCCESS if everything went all-right. */
_hidden isds_error _isds_init_crypto(void)
{
    OpenSSL_add_all_digests(); /* Loads all digest algorithms. */

    ERR_load_crypto_strings();
    //ERR_load_CMS_strings();

    version_openssl = SSLeay_version(SSLEAY_VERSION);

    return IE_SUCCESS;
}

/* Computes hash from @input with @length and store it into @hash.
 * The hash algorithm is defined inside @hash.
 * @input is input block to hash
 * @length is @input block length in bytes
 * @hash input algorithm, output hash value and hash length; hash value will be
 * reallocated, it's always valid pointer or NULL (before and after call) */
_hidden isds_error _isds_compute_hash(const void *input,
    const size_t length, struct isds_hash *hash)
{
    isds_error retval = IE_SUCCESS;
    void *hash_buf = NULL;
    size_t hash_len = 0;
    EVP_MD_CTX *mdctx = NULL;
    const char *hash_name = NULL;
    const EVP_MD *md;
    unsigned int md_len;

    if (((0 != length) && (NULL == input)) || (NULL == hash)) {
        return IE_INVAL;
    }

    isds_log(ILF_SEC, ILL_DEBUG,
            _("Data hash requested, length=%zu, content:\n%*s\n"
                "End of data to hash\n"), length, length, input);

    /* Select algorithm */
    switch (hash->algorithm) {
        case HASH_ALGORITHM_MD5: hash_name = "MD5"; break;
        case HASH_ALGORITHM_SHA_1: hash_name = "SHA1"; break;
        case HASH_ALGORITHM_SHA_224: hash_name = "SHA224"; break;
        case HASH_ALGORITHM_SHA_256: hash_name = "SHA256"; break;
        case HASH_ALGORITHM_SHA_384: hash_name = "SHA384"; break;
        case HASH_ALGORITHM_SHA_512: hash_name = "SHA512"; break;
        default:
            retval = IE_NOTSUP;
            goto fail;
    }

    md = EVP_get_digestbyname(hash_name);
    if (NULL == md) {
        retval = IE_NOTSUP;
        goto fail;
    }

    mdctx = EVP_MD_CTX_new();
    if (NULL == mdctx) {
        retval = IE_NOMEM;
        goto fail;
    }
    if (!EVP_DigestInit(mdctx, md)) {
        retval = IE_ERROR;
        goto fail;
    }
    if (!EVP_DigestUpdate(mdctx, input, length)) {
        retval = IE_ERROR;
        goto fail;
    }

    hash_len = EVP_MD_size(md);
    hash_buf = realloc(hash->value, hash_len);
    if (NULL == hash_buf) {
        retval = IE_NOMEM;
        goto fail;
    }
    hash->value = hash_buf;
    hash->length = hash_len;

    if (!EVP_DigestFinal_ex(mdctx, hash->value, &md_len)) {
        retval = IE_ERROR;
        goto fail;
    }

    EVP_MD_CTX_free(mdctx);
    mdctx = NULL;

    return IE_SUCCESS;

fail:
    if (NULL != mdctx) {
        EVP_MD_CTX_free(mdctx);
    }
    return retval;
}

/* Free CMS data buffer allocated inside _isds_extract_cms_data().
 * This is necessary because GPGME.
 * @buffer is pointer to memory to free */
_hidden void _isds_cms_data_free(void *buffer)
{
    free(buffer);
}

/* Extract data from CMS (successor of PKCS#7)
 * The CMS' signature is is not verified.
 * @context is session context
 * @cms is input block with CMS structure
 * @cms_length is @cms block length in bytes
 * @data are automatically reallocated bit stream with data found in @cms
 * You must free them with _isds_cms_data_free().
 * @data_length is length of @data in bytes */
_hidden isds_error _isds_extract_cms_data(struct isds_ctx *context,
        const void *cms, const size_t cms_length,
        void **data, size_t *data_length)
{
    unsigned long err;
    isds_error retval = IE_SUCCESS;
    BIO *bio = NULL;
    CMS_ContentInfo *cms_ci = NULL;
    const ASN1_OBJECT *asn1_obj;
    ASN1_OCTET_STRING **pos;
    int nid;
    char *locale_str;

    assert(NULL != context);

    if ((NULL == cms) || (0 == cms_length) ||
        (NULL == data) || (NULL == data_length)) {
        return IE_INVAL;
    }

    zfree(*data);
    *data_length = 0;

    bio = BIO_new_mem_buf((void *) cms, cms_length);
    if (NULL == bio) {
        isds_log_message(context, _("Creating CMS reader BIO failed"));
        while (0 != (err = ERR_get_error())) {
            locale_str = _isds_utf82locale(ERR_error_string(err, NULL));
            if (NULL != locale_str) {
                isds_log_message(context, locale_str);
                free(locale_str);
            }
        }
        retval = IE_ERROR;
        goto fail;
    }

    cms_ci = d2i_CMS_bio(bio, NULL);
    if (NULL == cms_ci) {
        isds_log_message(context, _("Cannot parse CMS"));
        while (0 != (err = ERR_get_error())) {
            locale_str = _isds_utf82locale(ERR_error_string(err, NULL));
            if (NULL != locale_str) {
                isds_log_message(context, locale_str);
                free(locale_str);
            }
        }
        retval = IE_ERROR;
        goto fail;
    }

    BIO_free(bio); bio = NULL;

    asn1_obj = CMS_get0_type(cms_ci);
    nid = OBJ_obj2nid(asn1_obj);
    switch (nid) {
        case NID_pkcs7_data:
        case NID_id_smime_ct_compressedData:
        case NID_id_smime_ct_authData:
        case NID_pkcs7_enveloped:
        case NID_pkcs7_encrypted:
        case NID_pkcs7_digest:
            assert(0);
            retval = IE_ERROR;
            goto fail;
            break;
        case NID_pkcs7_signed:
            break;
        default:
            assert(0);
            retval = IE_ERROR;
            goto fail;
            break;
    }

    pos = CMS_get0_content(cms_ci);
    if ((NULL == pos) || (NULL == *pos)) {
        assert(0);
        retval = IE_ERROR;
        goto fail;
    }

    *data = malloc((*pos)->length);
    if (NULL == *data) {
        retval = IE_NOMEM;
        goto fail;
    }
    *data_length = (*pos)->length;
    memcpy(*data, (*pos)->data, (*pos)->length);

    CMS_ContentInfo_free(cms_ci); cms_ci = NULL;

    return IE_SUCCESS;

fail:
    if (NULL != bio) {
        BIO_free(bio);
    }
    if (NULL != cms_ci) {
        CMS_ContentInfo_free(cms_ci);
    }
    return retval;
}
