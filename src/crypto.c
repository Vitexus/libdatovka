#define _XOPEN_SOURCE 500   /* strdup from string.h */
#include "isds_priv.h"
#include "utils.h"
#include "gcrypt.h"

#ifdef ISDS_USE_KSBA
    #include <ksba.h>
#endif

#include <gpgme.h>
#include <locale.h>

/* Inicialize libgrcypt if not yet done by application or other library.
 * @return IE_SUCCESS if everything is O.k. */
_hidden isds_error init_gcrypt(void) {
    const char *gcrypt_version;
    
    /* Check version and initialize gcrypt */
    gcrypt_version = gcry_check_version(NULL);
    if (!gcrypt_version)  {
        isds_log(ILF_SEC, ILL_CRIT, _("Could not check gcrypt version\n"));
        return IE_ERROR;
    }

    /* Finalize initialization if not yet done */
    if (!gcry_control(GCRYCTL_INITIALIZATION_FINISHED_P)) {
        /* Disable secure memory */
        /* TODO: Allow it when implementing key authentication */
        gcry_control (GCRYCTL_DISABLE_SECMEM, 0);
        /* Finish initialization */
        gcry_control(GCRYCTL_INITIALIZATION_FINISHED, 0);
    }

    isds_log(ILF_SEC, ILL_INFO, _("gcrypt version in use: %s\n"),
            gcrypt_version);

    return IE_SUCCESS;
}


/* Computes hash from @input with @length and store it into @hash.
 * The hash algoritm is defined inside @hash.
 * @input is input block to hash
 * @length is @input block length in bytes
 * @hash input algoritm, output hash value and hash length; hash value will be
 * reallocated, it's always valid pointer or NULL (before and after call) */
_hidden isds_error compute_hash(const void *input, const size_t length,
        struct isds_hash *hash) {
    int g_algorithm;
    void *buffer;

    if ((length != 0 && !input) || !hash) return IE_INVAL;

    isds_log(ILF_SEC, ILL_DEBUG,
            _("Data hash requested, length=%zu, content:\n%*s\n"
                "End of data to hash\n"), length, length, input);

    /* Select algorithm */
    switch (hash->algorithm) {
        case HASH_ALGORITHM_MD5:        g_algorithm = GCRY_MD_MD5; break;
        case HASH_ALGORITHM_SHA_1:      g_algorithm = GCRY_MD_SHA1; break;
        case HASH_ALGORITHM_SHA_224:    g_algorithm = GCRY_MD_SHA224; break;
        case HASH_ALGORITHM_SHA_256:    g_algorithm = GCRY_MD_SHA256; break;
        case HASH_ALGORITHM_SHA_384:    g_algorithm = GCRY_MD_SHA384; break;
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
    gcry_md_hash_buffer(g_algorithm, hash->value, (length)?input:"", length);

    return IE_SUCCESS;
}


/* Inicialize GPGME.
 * @return IE_SUCCESS if everything is O.k. */
_hidden isds_error init_gpgme(void) {
    const char *gpgme_version;
    
    /* Check version and initialize GPGME */
    gpgme_version = gpgme_check_version(NULL);
    if (!gpgme_version)  {
        isds_log(ILF_SEC, ILL_CRIT, _("GPGME initialization failed\n"));
        return IE_ERROR;
    }

    isds_log(ILF_SEC, ILL_INFO, _("GPGME version in use: %s\n"),
            gpgme_version);
    /* Needed to propagate locale to remote processes like pinentry */
    gpgme_set_locale (NULL, LC_CTYPE, setlocale(LC_CTYPE, NULL));
#ifdef LC_MESSAGES
    gpgme_set_locale (NULL, LC_MESSAGES, setlocale(LC_MESSAGES, NULL));
#endif

    /* Check for engines */
    if (gpgme_engine_check_version(GPGME_PROTOCOL_CMS)) {
        isds_log(ILF_SEC, ILL_CRIT, _("GPGME does not support CMS\n"));
        return IE_ERROR;
    }

    return IE_SUCCESS;
}

/* Free CMS data buffer allocated inside extract_cms_data().
 * This is necesary because GPGME.
 * @buffer is pointer to memory to free */
_hidden void cms_data_free(void *buffer) {
#ifdef ISDS_USE_KSBA
    free(buffer);
#else
    if (buffer) gpgme_free(buffer);
#endif
}


/* Extract data from CMS (successor of PKCS#7)
 * @context is session context
 * @cms is input block with CMS structure
 * @cms_length is @cms block length in bytes
 * @data are automatically reallocated bit stream with data found in @cms
 * You must free them with cms_data_free().
 * @data_length is length of @data in bytes */
_hidden isds_error extract_cms_data(struct isds_ctx *context,
        const void *cms, const size_t cms_length,
        void **data, size_t *data_length) {
    isds_error err = IE_SUCCESS;

    if (!cms || !data || !data_length) return IE_INVAL;

    zfree(*data);
    *data_length = 0;

#ifdef ISDS_USE_KSBA
    ksba_cms_t cms_handler = NULL;
    ksba_reader_t cms_reader = NULL;
    ksba_writer_t cms_writer = NULL;
    gpg_error_t gerr;
    char gpg_error_string[128];
    ksba_stop_reason_t stop_reason;

    if (ksba_cms_new(&cms_handler)) {
        isds_log_message(context, _("Could not allocate CMS parser handler"));
        err = IE_NOMEM;
        goto leave;
    }
    if (ksba_reader_new(&cms_reader)) {
        isds_log_message(context, _("Could not allocate CMS reader"));
        err = IE_ERROR;
        goto leave;
    }
    if (ksba_reader_set_mem(cms_reader, cms, cms_length)) {
        isds_log_message(context,
                _("Could not bind CMS reader to PKCS#7 structure"));
        err = IE_ERROR;
        goto leave;
    }
    if (ksba_writer_new(&cms_writer)) {
        isds_log_message(context, _("Could not allocate CMS writer"));
        err = IE_ERROR;
        goto leave;
    }
    if (ksba_writer_set_mem(cms_writer, 0)) {
        isds_log_message(context,
                _("Could not bind CMS reader to PKCS#7 structure"));
        err = IE_ERROR;
        goto leave;
    }
    if (ksba_cms_set_reader_writer(cms_handler, cms_reader, cms_writer)) {
        isds_log_message(context,
                _("Could not register CMS reader to CMS handler"));
        err = IE_ERROR;
        goto leave;
    }


    /* FIXME: This cycle stops with: Missing action on KSBA_CT_SIGNED_DATA
     * I don't know how to program the KSBA cycle.
     * TODO: Use gpgme's verify call to extract data. The only problem is it
     * gpgme verifies signature always. We don't need it now and it's slow.
     * We should find out how to use KSBA. */
    do {
        gerr = ksba_cms_parse(cms_handler, &stop_reason);
        if (gerr) {
            gpg_strerror_r(gerr, gpg_error_string, sizeof(gpg_error_string));
            gpg_error_string[sizeof(gpg_error_string)/sizeof(char) - 1] = '\0';
            isds_printf_message(context,
                    _("Error while parsing PKCS#7 structure: %s"),
                    gpg_error_string);
            return IE_ERROR;
        }
        if (stop_reason == KSBA_SR_BEGIN_DATA) {
            isds_log(ILF_SEC, ILL_DEBUG, _("CMS: Data begining found\n"));
        }
        if (stop_reason == KSBA_SR_GOT_CONTENT) {
            char *type;
            switch (ksba_cms_get_content_type(cms_handler, 0)) {
                case KSBA_CT_NONE: type = _("uknown data"); break;
                case KSBA_CT_DATA: type = _("plain data"); break;
                case KSBA_CT_SIGNED_DATA: type = _("signed data"); break;
                case KSBA_CT_ENVELOPED_DATA:
                        type = _("encypted data by session key"); break;
                case KSBA_CT_DIGESTED_DATA: type = _("digest data"); break;
                case KSBA_CT_ENCRYPTED_DATA: type = _("encrypted data"); break;
                case KSBA_CT_AUTH_DATA: type = _("auth data"); break;
                default: type = _("other data");
            }
            isds_log(ILF_SEC, ILL_DEBUG, _("CMS: Data type: %s\n"), type);
        }
        if (stop_reason == KSBA_SR_END_DATA) {
            isds_log(ILF_SEC, ILL_DEBUG, _("CMS: Data end found\n"));
        }
    } while (stop_reason != KSBA_SR_READY);

    *data = ksba_writer_snatch_mem(cms_writer, data_length);
    if (!*data) {
        isds_log_message(context, _("Getting CMS writer buffer failed"));
        err = IE_ERROR;
        goto leave;
    }

leave:
    ksba_cms_release(cms_handler);
    ksba_writer_release(cms_writer);
    ksba_reader_release(cms_reader);
#else /* ndef ISDS_USE_KSBA */
    gpgme_ctx_t gctx = NULL;
    gpgme_error_t gerr;
    char gpgme_error_string[128];
    gpgme_data_t cms_handler = NULL, plain_handler = NULL;

#define GET_GPGME_ERROR_STRING \
    gpgme_strerror_r(gerr, gpgme_error_string, sizeof(gpgme_error_string)); \
    gpgme_error_string[sizeof(gpgme_error_string)/sizeof(char) - 1] = '\0'; \

#define FAIL_ON_GPGME_ERROR(code, message) \
    if (code) { \
        GET_GPGME_ERROR_STRING; \
        isds_printf_message(context, message, gpgme_error_string); \
        if ((code) == GPG_ERR_ENOMEM) err = IE_NOMEM; \
        else err = IE_ERROR; \
        goto leave; \
    }

    /* Create GPGME context */
    gerr = gpgme_new(&gctx);
    FAIL_ON_GPGME_ERROR(gerr, _("Could not create GPGME context: %s"));

    gerr = gpgme_set_protocol(gctx, GPGME_PROTOCOL_CMS);
    FAIL_ON_GPGME_ERROR(gerr, 
                _("Could not set CMS protocol for GPGME context: %s"));

    /* Create data handlers */
    gerr = gpgme_data_new_from_mem(&cms_handler, cms, cms_length, 0);
    FAIL_ON_GPGME_ERROR(gerr, _("Could not create data handler for "
                "signed message in CMS structure: %s"));
    gerr = gpgme_data_set_encoding(cms_handler, GPGME_DATA_ENCODING_BINARY);
    FAIL_ON_GPGME_ERROR(gerr, _("Could not explain to GPGME "
                "that CMS structure was packed in DER binary format: %s"));

    gerr = gpgme_data_new(&plain_handler);
    FAIL_ON_GPGME_ERROR(gerr, _("Could not create data handler for "
                "plain message extracted from CMS structure: %s"));

    /* Verify signature */
    gerr = gpgme_op_verify(gctx, cms_handler, NULL, plain_handler);
    if (gerr) {
        GET_GPGME_ERROR_STRING;
        isds_printf_message(context,
                _("CMS verification failed: %s"),
                gpgme_error_string);
        err = IE_ERROR;
        goto leave;
    }

    /* Get extracted plain message
     * XXX: One must free *data with gpgme_free() because of clashing
     * possibly different allocators. */
    *data = gpgme_data_release_and_get_mem(plain_handler, data_length);
    plain_handler = NULL;
    if (!*data) {
        /* No data or error occured */
        isds_printf_message(context,
                _("Could not get plain data from GPGME "
                    "after verifying CMS structure"));
        err = IE_ERROR;
        goto leave;
    }

leave:
    if (gctx) gpgme_release(gctx);
    if (plain_handler) gpgme_data_release(plain_handler);
    if (cms_handler) gpgme_data_release(cms_handler);
#undef FAIL_ON_GPGME_ERROR
#undef GET_GPGME_ERROR_STRING
#endif /* ndef ISDS_USE_KSBA */
    return err;
}

