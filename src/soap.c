#include "isds_priv.h"
#include "soap.h"
#include "utils.h"
#include <stdlib.h>
#include <string.h>
#include <strings.h>    /* strncasecmp(3) */

/* Private structure for write_body() call back */
struct soap_body {
    void *data;
    size_t length;
};

/* Private structure for write_header() call back */
struct auth_headers {
    _Bool is_complete;  /* Response has finished, next iteration is new
                           response, values become obsolete. */
    char *last_header;  /* Temporary storage for previous unfinished header */
    char *method;       /* WWW-Authenticate value */
    char *code;         /* X-Response-message-code value */
    isds_otp_resolution resolution;     /* Decoded .code member */
    char *message;      /* X-Response-message-text value */
    char *redirect;     /* Redirect URL */
};


/* Deallocate content of struct auth_headers */
static void auth_headers_free(struct auth_headers *headers) {
    zfree(headers->last_header);
    zfree(headers->method);
    zfree(headers->code);
    zfree(headers->message);
    zfree(headers->redirect);
}


/* If given @line is HTTP header of @name,
 * return pointer to the header value. Otherwise return NULL.
 * @name is header name without name---value separator, terminated with 0. */
static const char *header_value(const char *line, const char *name) {
    const char *value;
    if (line == NULL || name == NULL) return NULL;
    
    for (value = line; ; value++, name++) {
        if (*value == '\0') return NULL;        /* Line too short */
        if (*name == '\0') break;               /* Name matches */
        if (*name != *value) return NULL;       /* Name does not match */
    }

    /* Check separator. RFC2616, section 4.2 requires collon only. */
    if (*value++ != ':') return NULL;

    return value;
}


/* Try to decode header value per RFC 2047.
 * @input is zero terminated input, it's updated to point all consumed
 * input - 1.
 * @output is buffer to store decoded value, it's updated to point after last
 * written character. The buffer must be preallocated.
 * @return 0 if input has been successfully decoded, then @input and @output
 * poineres will be updated. Otherwise return non-zero value and keeps
 * argument pointers and memory unchanged. */
static int try_rfc2047_decode(const char **input, char **output) {
    const char *encoded;
    const char *charset_start, *encoding, *end;
    size_t charset_length;
    char *charset = NULL;
    /* ISDS prescribes B encoding only, but RFC 2047 requires to support Q
     * encoding too. ISDS prescribes UTF-8 charset only, RFC requiers to
     * support any MIME charset. */
    if (input == NULL || *input == NULL || output == NULL || *output == NULL)
        return -1;

    /* Start is "=?" */
    encoded = *input;
    if (encoded[0] != '=' || encoded[1] != '?')
        return -1;

    /* Then is "CHARSET?" */
    charset_start = (encoded += 2);
    while (*encoded != '?') {
        if (*encoded == '\0')
            return -1;
        if (*encoded == ' ' || *encoded == '\t' || *encoded == '\r' || *encoded == '\n')
            return -1;
        encoded++;
    }
    encoded++;

    /* Then is "ENCODING?", where ENCODING is /[BbQq]/ */
    if (*encoded == '\0') return -1;
    encoding = encoded++;
    if (*encoded != '?')
        return -1;
    encoded++;

    /* Then is "ENCODED_TEXT?=" */
    while (*encoded != '?') {
        if (*encoded == '\0')
            return -1;
        if (*encoded == ' ' || *encoded == '\t' || *encoded == '\r' || *encoded == '\n')
            return -1;
        encoded++;
    }
    end = encoded;
    if (*(++encoded) != '=') return -1;

    /* Now pointers are:
     * "=?CHARSET?E?ENCODED_TEXT?="
     *  | |       |             || 
     *  | |       |             |\- encoded
     *  | |       |             \- end
     *  | |       \- encoding
     *  | \- charset_start
     *  \- *input
     */
    
    charset_length = encoding - charset_start - 1;
    if (charset_length < 1)
        return -1;
    charset = strndup(charset_start, charset_length);
    if (charset == NULL)
        return -1;

    /* Decode encoding */
    char *bit_stream = NULL;
    size_t bit_length = 0;
    size_t encoding_length = end - encoding - 2;

    if (*encoding == 'B') {
        /* Decode Base-64 */
        char *b64_stream = NULL;
        if (NULL == (b64_stream =
                    malloc((encoding_length + 1) * sizeof(*encoding)))) {
            free(charset);
            return -1;
        }
        memcpy(b64_stream, encoding + 2, encoding_length);
        b64_stream[encoding_length] = '\0';
        bit_length = _isds_b64decode(b64_stream, (void **)&bit_stream);
        free(b64_stream);
        if (bit_length == (size_t) -1) {
            free(charset);
            return -1;
        }
    } else if (*encoding == 'Q') {
        /* Decode Quoted-printable-like */
        if (NULL == (bit_stream =
                    malloc((encoding_length) * sizeof(*encoding)))) {
            free(charset);
            return -1;
        }
        for (size_t q = 2; q < encoding_length + 2; q++) {
            if (encoding[q] == '_') {
                bit_stream[bit_length] = '\x20';
            } else if (encoding[q] == '=') {
                int ordinar;
                /* Validate "=HH", where H is hexadecimal digit */
                if (q + 2 >= encoding_length + 2 ) {
                    free(bit_stream);
                    free(charset);
                    return -1;
                }
                /* Convert =HH */
                if ((ordinar = _isds_hex2i(encoding[++q])) < 0) {
                    free(bit_stream);
                    free(charset);
                    return -1;
                }
                bit_stream[bit_length] = (ordinar << 4);
                if ((ordinar = _isds_hex2i(encoding[++q])) < 0) {
                    free(bit_stream);
                    free(charset);
                    return -1;
                }
                bit_stream[bit_length] += ordinar;
            } else {
                bit_stream[bit_length] = encoding[q];
            }
            bit_length++;
        }
    } else {
        /* Unknown encoding */
        free(charset);
        return -1;
    }
    
    /* Convert to UTF-8 */
    char *utf_stream = NULL;
    size_t utf_length;
    utf_length = _isds_any2any(charset, "UTF-8", bit_stream, bit_length,
            (void **)&utf_stream);
    free(bit_stream);
    free(charset);
    if (utf_length == (size_t) -1) {
        return -1;
    }

    /* Copy UTF-8 stream to output buffer */
    memcpy(*output, utf_stream, utf_length);
    free(utf_stream);
    *output += utf_length;

    *input = encoded;
    return 0;
}


/* Decode HTTP header value per RFC 2047.
 * @encoded_value is encoded HTTP header value terminated with NUL. It can
 * contain HTTP LWS separators that will be replaced with a space.
 * @return newly allocated decoded value without EOL, or return NULL. */
static char *decode_header_value(const char *encoded_value) {
    char *decoded = NULL, *decoded_cursor;
    size_t content_length;
    _Bool text_started = 0, lws_seen = 0;

    if (encoded_value == NULL) return NULL;
    content_length = strlen(encoded_value);

    /* A character can occupy up to 6 bytes in UTF-8 */
    decoded = malloc(content_length * 6 + 1);
    if (decoded == NULL) {
        /* ENOMEM */
        return NULL;
    }

    /* Decode */
    /* RFC 2616, section 4.2: Remove surrounding LWS, replace inner ones with
     * a space. */
    /* FIXME: Implement the RFC 2047 (=?UTF-8?B?...?=) */
    for (decoded_cursor = decoded; *encoded_value; encoded_value++) {
        if (*encoded_value == '\r' || *encoded_value == '\n' ||
                *encoded_value == '\t' || *encoded_value == ' ') {
            lws_seen = 1; 
            continue;
        }
        if (lws_seen) {
            lws_seen = 0;
            if (text_started) *(decoded_cursor++) = ' ';
        }
        if (!(*encoded_value == '=' &&
                !try_rfc2047_decode(&encoded_value, &decoded_cursor)))
            *(decoded_cursor++) = *encoded_value;
        text_started = 1;
    }
    *decoded_cursor = '\0';

    return decoded;
}


/* Return true, if server requests OTP authorization method that client
 * requested. Otherwise return false.
 * @client_method is method client requested
 * @server_method is value of WWW-Authenticate header */
/*static _Bool otp_method_matches(const isds_otp_method client_method,
        const char *server_method) {
    char *method_name = NULL;
    
    switch (client_method) {
        case OTP_HMAC: method_name = "hotp"; break;
        case OTP_TIME: method_name = "totp"; break;
        default: return 0;
    }

    if (!strncmp(server_method, method_name, 4) && (
                server_method[4] == '\0' || server_method[4] == ' ' ||
                server_method[4] == '\t'))
        return 1;
    return 0;
}*/


/* Convert UTF-8 @string to HTTP OTP resolution enum type.
 * @Return corresponding value or OTP_RESOLUTION_UNKNOWN if @string is not
 * defined or unknown value. */
static isds_otp_resolution string2isds_otp_resolution(const char *string) {
    if (string == NULL)
        return OTP_RESOLUTION_UNKNOWN;
    else if (!strcmp(string, "authentication.info.totpSended"))
        return OTP_RESOLUTION_TOTP_SENT;
    else if (!strcmp(string, "authentication.error.userIsNotAuthenticated"))
        return OTP_RESOLUTION_BAD_AUTHENTICATION;
    else if (!strcmp(string, "authentication.error.intruderDetected"))
        return OTP_RESOLUTION_ACCESS_BLOCKED;
    else if (!strcmp(string, "authentication.error.paswordExpired"))
        return OTP_RESOLUTION_PASSWORD_EXPIRED;
    else if (!strcmp(string, "authentication.info.cannotSendQuickly"))
        return OTP_RESOLUTION_TO_FAST;
    else if (!strcmp(string, "authentication.error.badRole"))
        return OTP_RESOLUTION_UNAUTHORIZED;
    else if (!strcmp(string, "authentication.info.totpNotSended"))
        return OTP_RESOLUTION_TOTP_NOT_SENT;
    else
        return OTP_RESOLUTION_UNKNOWN;
}


/* Close connection to server and destroy CURL handle associated
 * with @context */
_hidden isds_error _isds_close_connection(struct isds_ctx *context) {
    if (!context) return IE_INVALID_CONTEXT;

    if (context->curl) {
        curl_easy_cleanup(context->curl);
        context->curl = NULL;
        isds_log(ILF_HTTP, ILL_DEBUG, _("Connection to server %s closed\n"),
                context->url);
        return IE_SUCCESS;
    } else {
        return IE_CONNECTION_CLOSED;
    }
}


/* Remove username and password from context and CURL handle. */
static isds_error unset_http_authorization(struct isds_ctx *context) {
    isds_error error = IE_SUCCESS;

    if (context == NULL) return IE_INVALID_CONTEXT;
    if (context->curl == NULL) return IE_CONNECTION_CLOSED;

#if HAVE_DECL_CURLOPT_USERNAME /* Since curl-7.19.1 */
    if (curl_easy_setopt(context->curl, CURLOPT_USERNAME, NULL))
        error = IE_ERROR;
    if (curl_easy_setopt(context->curl, CURLOPT_PASSWORD, NULL))
        error = IE_ERROR;
#else
    if (curl_easy_setopt(context->curl, CURLOPT_USERPWD, NULL))
        error = IE_ERROR;
#endif /* not HAVE_DECL_CURLOPT_USERNAME */

    if (error) 
        isds_log(ILF_HTTP, ILL_ERR, _("Error while unsetting user name and"
                    "password from CURL handle for connection to server %s.\n"),
                context->url);
    else
        isds_log(ILF_HTTP, ILL_DEBUG, _("User name and password for server %s"
                    "have been unset.\n"), context->url);
    return error;
}


/* CURL call back function called when chunk of HTTP response body is available.
 * @buffer points to new data
 * @size * @nmemb is length of the chunk in bytes. Zero means empty body.
 * @userp is private structure.
 * Must return the length of the chunk, otherwise CURL will signal
 * CURL_WRITE_ERROR. */
static size_t write_body(void *buffer, size_t size, size_t nmemb, void *userp) {
    struct soap_body *body = (struct soap_body *) userp;
    void *new_data;

    /* FIXME: Check for (size * nmemb + body->lengt) !> SIZE_T_MAX.
     * Precompute the product then. */

    if (!body) return 0;    /* This should never happen */
    if (0 == (size * nmemb)) return 0; /* Empty body */

    new_data = realloc(body->data, body->length + size * nmemb);
    if (!new_data) return 0;

    memcpy(new_data + body->length, buffer, size * nmemb);

    body->data = new_data;
    body->length += size * nmemb;

    return (size * nmemb);
}


/* CURL call back function called when a HTTP response header is available.
 * This is called for each header even if reply consists of more responses.
 * @buffer points to new header (no zero terminator, but HTTP EOL is included)
 * @size * @nmemb is length of the header in bytes
 * @userp is private structure.
 * Must return the length of the header, otherwise CURL will signal
 * CURL_WRITE_ERROR. */
static size_t write_header(void *buffer, size_t size, size_t nmemb, void *userp) {
    struct auth_headers *headers = (struct auth_headers *) userp;
    size_t length;
    const char *value;

    /* FIXME: Check for (size * nmemb) !> SIZE_T_MAX.
     * Precompute the product then. */
    length = size * nmemb;

    if (NULL == headers) return 0;    /* This should never happen */
    if (0 == length) {
        /* ??? Is this the empty line delimiter? */
        return 0; /* Empty headers */
    }

    /* New response, invalide authentication headers. */
    /* XXX: Chunked encoding trailer is not supported */
    if (headers->is_complete) auth_headers_free(headers);

    /* Append continuation to multi-line header */
    if (*(char *)buffer == ' ' || *(char *)buffer == '\t') {
        if (headers->last_header != NULL) {
            size_t old_length = strlen(headers->last_header);
            char *longer_header = realloc(headers->last_header, old_length + length);
            if (longer_header == NULL) {
                /* ENOMEM */
                return 0;
            }
            strncpy(longer_header + old_length, (char*)buffer + 1, length - 1);
            longer_header[old_length + length - 1] = '\0';
            headers->last_header = longer_header;
        } else {
            /* Invalid continuation without starting header will be skipped. */
            isds_log(ILF_HTTP, ILL_WARNING,
                    _("HTTP header continuation without starting header has "
                        "been encountered. Skipping invalid HTTP response "
                        "line.\n"));
        }
        goto leave;
    }

    /* Decode last header */
    value = header_value(headers->last_header, "WWW-Authenticate");
    if (value != NULL) {
       free(headers->method);
       if (NULL == (headers->method = decode_header_value(value))) {
           /* TODO: Set IE_NOMEM to context */
           return 0;
       }
       goto store;
    }

    value = header_value(headers->last_header, "X-Response-message-code");
    if (value != NULL) {
       free(headers->code);
       if (NULL == (headers->code = decode_header_value(value))) {
           /* TODO: Set IE_NOMEM to context */
           return 0;
       }
       goto store;
    }

    value = header_value(headers->last_header, "X-Response-message-text");
    if (value != NULL) {
       free(headers->message);
       if (NULL == (headers->message = decode_header_value(value))) {
           /* TODO: Set IE_NOMEM to context */
           return 0;
       }
       goto store;
    }

store:
    /* Last header decoded, free it */
    zfree(headers->last_header);

    if (!strncmp(buffer, "\r\n", length)) {
        /* Current line is header---body separator */
        headers->is_complete = 1;
        goto leave;
    } else {
        /* Current line is new header, store it */
        headers->last_header = malloc(length + 1);
        if (headers->last_header == NULL) {
            /* TODO: Set IE_NOMEM to context */
            return 0;
        }
        memcpy(headers->last_header, buffer, length);
        headers->last_header[length] = '\0';
    }

leave:
    return (length);
}


/* CURL progress callback proxy to rearrange arguments.
 * @curl_data is session context  */
static int progress_proxy(void *curl_data, double download_total,
        double download_current, double upload_total, double upload_current) {
    struct isds_ctx *context = (struct isds_ctx *) curl_data;
    int abort = 0;

    if (context && context->progress_callback) {
        abort = context->progress_callback(
                upload_total, upload_current,
                download_total, download_current,
                context->progress_callback_data);
        if (abort) {
            isds_log(ILF_HTTP, ILL_INFO,
                    _("Application aborted HTTP transfer"));
        }
    }

    return abort;
}


/* CURL call back function called when curl has something to log.
 * @curl is cURL context
 * @type is cURL log facility
 * @buffer points to log data, XXX: not zero-terminated
 * @size is length of log data
 * @userp is private structure.
 * Must return 0. */
static int log_curl(CURL *curl, curl_infotype type, char *buffer, size_t size,
        void *userp) {
    if (!buffer || 0 == size) return 0;
    if (type == CURLINFO_TEXT || type == CURLINFO_HEADER_IN ||
            type == CURLINFO_HEADER_OUT)
        isds_log(ILF_HTTP, ILL_DEBUG, "%*s", size, buffer);
    return 0;
}


/* Do HTTP request.
 * @context holds the base URL,
 * @url is a (CGI) file of SOAP URL,
 * @use_get is a false to do a POST request, true to do a GET request.
 * @request is body for POST request 
 * @request_length is length of @request in bytes
 * @reponse is automatically reallocated() buffer to fit HTTP response with
 * @response_length (does not need to match allocated memory exactly). You must
 * free() the @response.
 * @mime_type is automatically allocated MIME type send by server (*NULL if not
 * sent). Set NULL if you don't care.
 * @charset is charset of the body signaled by server. The same constrains
 * like on @mime_type apply.
 * @http_code is final HTTP code returned by server. This can be 200, 401, 500
 * or any other one. Pass NULL if you don't interest.
 * In case of error, the response memory, MIME type, charset and length will be
 * deallocated and zeroed automatically. Thus be sure they are preallocated or
 * they points to NULL.
 * @response_otp_headers is pre-allocated structure for OTP authentication
 * headers sent by server. Members must be valid pointers or NULLs.
 * Pass NULL if you don't interest.
 * Be ware that successful return value does not mean the HTTP request has
 * been accepted by the server. You must consult @http_code. OTOH, failure
 * return value means the request could not been sent (e.g. SSL error).
 * Side effect: message buffer */
static isds_error http(struct isds_ctx *context,
        const char *url, _Bool use_get,
        const void *request, const size_t request_length,
        void **response, size_t *response_length,
        char **mime_type, char **charset, long *http_code,
        struct auth_headers *response_otp_headers) {

    CURLcode curl_err;
    isds_error err = IE_SUCCESS;
    struct soap_body body;
    char *content_type;
    struct curl_slist *headers = NULL;


    if (!context) return IE_INVALID_CONTEXT;
    if (!url) return IE_INVAL;
    if (request_length > 0 && !request) return IE_INVAL;
    if (!response || !response_length) return IE_INVAL;

    /* Clean authentication headers */

    /* Set the body here to allow deallocation in leave block */
    body.data = *response;
    body.length = 0;

    /* Set Request-URI */
    curl_err = curl_easy_setopt(context->curl, CURLOPT_URL, url);

    /* Set TLS options */
    if (!curl_err && context->tls_verify_server) {
        if (!*context->tls_verify_server)
            isds_log(ILF_SEC, ILL_WARNING,
                    _("Disabling server identity verification. "
                    "That was your decision.\n"));
        curl_err = curl_easy_setopt(context->curl, CURLOPT_SSL_VERIFYPEER,
                (*context->tls_verify_server)? 1L : 0L);
        if (!curl_err) {
            curl_err = curl_easy_setopt(context->curl, CURLOPT_SSL_VERIFYHOST,
                    (*context->tls_verify_server)? 2L : 0L);
        }
    }
    if (!curl_err && context->tls_ca_file) {
        isds_log(ILF_SEC, ILL_INFO,
                _("CA certificates will be searched in `%s' file since now\n"),
                context->tls_ca_file);
        curl_err = curl_easy_setopt(context->curl, CURLOPT_CAINFO,
                context->tls_ca_file);
    }
    if (!curl_err && context->tls_ca_dir) {
        isds_log(ILF_SEC, ILL_INFO,
                _("CA certificates will be searched in `%s' directory "
                    "since now\n"), context->tls_ca_dir);
        curl_err = curl_easy_setopt(context->curl, CURLOPT_CAPATH,
                context->tls_ca_dir);
    }
    if (!curl_err && context->tls_crl_file) {
#if HAVE_DECL_CURLOPT_CRLFILE /* Since curl-7.19.0 */
        isds_log(ILF_SEC, ILL_INFO,
                _("CRLs will be searched in `%s' file since now\n"),
                context->tls_crl_file);
        curl_err = curl_easy_setopt(context->curl, CURLOPT_CRLFILE,
                context->tls_crl_file);
#else
        isds_log(ILF_SEC, ILL_WARNING,
                _("Your curl library cannot pass certificate revocation "
                    "list to cryptographic library.\n"
                    "Make sure cryptographic library default setting "
                    "delivers proper CRLs,\n"
                    "or upgrade curl.\n"));
#endif /* not HAVE_DECL_CURLOPT_CRLFILE */
    }


    /* Set credentials */
#if HAVE_DECL_CURLOPT_USERNAME /* Since curl-7.19.1 */
    if (!curl_err && context->username) {
        curl_err = curl_easy_setopt(context->curl, CURLOPT_USERNAME,
                context->username);
    }
    if (!curl_err && context->password) {
        curl_err = curl_easy_setopt(context->curl, CURLOPT_PASSWORD,
                context->password);
    }
#else
    if (!curl_err && (context->username || context->password)) {
        char *userpwd =
            _isds_astrcat3(context->username, ":", context->password);
        if (!userpwd) {
            isds_log_message(context, _("Could not pass credentials to CURL"));
            err = IE_NOMEM;
            goto leave;
        }
        curl_err = curl_easy_setopt(context->curl, CURLOPT_USERPWD, userpwd);
        free(userpwd);
    }
#endif /* not HAVE_DECL_CURLOPT_USERNAME */

    /* Set PKI credentials */
    if (!curl_err && (context->pki_credentials)) {
        if (context->pki_credentials->engine) {
            /* Select SSL engine */
            isds_log(ILF_SEC, ILL_INFO,
                    _("Cryptographic engine `%s' will be used for "
                        "key or certificate\n"),
                    context->pki_credentials->engine);
            curl_err = curl_easy_setopt(context->curl, CURLOPT_SSLENGINE,
                    context->pki_credentials->engine);
        }

        if (!curl_err) {
            /* Select certificate format */
#if HAVE_DECL_CURLOPT_SSLCERTTYPE /* since curl-7.9.3 */
            if (context->pki_credentials->certificate_format ==
                    PKI_FORMAT_ENG) {
                /* XXX: It's valid to have certificate in engine without name.
                 * Engines can select certificate according private key and
                 * vice versa. */
                if (context->pki_credentials->certificate)
                    isds_log(ILF_SEC, ILL_INFO, _("Client `%s' certificate "
                                "will be read from `%s' engine\n"),
                            context->pki_credentials->certificate,
                            context->pki_credentials->engine);
                else
                    isds_log(ILF_SEC, ILL_INFO, _("Client certificate " 
                                "will be read from `%s' engine\n"),
                            context->pki_credentials->engine);
                curl_err = curl_easy_setopt(context->curl, CURLOPT_SSLCERTTYPE,
                        "ENG");
            } else if (context->pki_credentials->certificate) {
                isds_log(ILF_SEC, ILL_INFO, _("Client %s certificate " 
                            "will be read from `%s' file\n"),
                        (context->pki_credentials->certificate_format ==
                            PKI_FORMAT_DER) ? _("DER") : _("PEM"),
                        context->pki_credentials->certificate);
                curl_err = curl_easy_setopt(context->curl, CURLOPT_SSLCERTTYPE,
                        (context->pki_credentials->certificate_format ==
                            PKI_FORMAT_DER) ? "DER" : "PEM");
            }
#else
            if ((context->pki_credentials->certificate_format ==
                        PKI_FORMAT_ENG ||
                        context->pki_credentials->certificate))
                isds_log(ILF_SEC, ILL_WARNING,
                        _("Your curl library cannot distinguish certificate "
                            "formats. Make sure your cryptographic library\n"
                            "understands your certificate file by default, "
                            "or upgrade curl.\n"));
#endif /* not HAVE_DECL_CURLOPT_SSLCERTTYPE */
        }

        if (!curl_err && context->pki_credentials->certificate) {
            /* Select certificate */
            if (!curl_err)
                curl_err = curl_easy_setopt(context->curl, CURLOPT_SSLCERT,
                        context->pki_credentials->certificate);
        }

        if (!curl_err) {
            /* Select key format */
            if (context->pki_credentials->key_format == PKI_FORMAT_ENG) {
                if (context->pki_credentials->key)
                    isds_log(ILF_SEC, ILL_INFO, _("Client private key `%s' "
                                "from `%s' engine will be used\n"),
                            context->pki_credentials->key,
                            context->pki_credentials->engine);
                else 
                    isds_log(ILF_SEC, ILL_INFO, _("Client private key "
                                "from `%s' engine will be used\n"),
                            context->pki_credentials->engine);
                curl_err = curl_easy_setopt(context->curl, CURLOPT_SSLKEYTYPE,
                        "ENG");
            } else if (context->pki_credentials->key) {
                isds_log(ILF_SEC, ILL_INFO, _("Client %s private key will be "
                            "read from `%s' file\n"),
                        (context->pki_credentials->key_format ==
                            PKI_FORMAT_DER) ? _("DER") : _("PEM"),
                        context->pki_credentials->key);
                curl_err = curl_easy_setopt(context->curl, CURLOPT_SSLKEYTYPE,
                        (context->pki_credentials->key_format ==
                            PKI_FORMAT_DER) ? "DER" : "PEM");
            }

            if (!curl_err)
                /* Select key */
                curl_err = curl_easy_setopt(context->curl, CURLOPT_SSLKEY,
                        context->pki_credentials->key);

            if (!curl_err) {
                /* Pass key pass-phrase */
#if HAVE_DECL_CURLOPT_KEYPASSWD /* since curl-7.16.5 */
                curl_err = curl_easy_setopt(context->curl,
                        CURLOPT_KEYPASSWD,
                        context->pki_credentials->passphrase);
#elif HAVE_DECL_CURLOPT_SSLKEYPASSWD /* up to curl-7.16.4 */
                curl_err = curl_easy_setopt(context->curl,
                        CURLOPT_SSLKEYPASSWD,
                        context->pki_credentials->passphrase);
#else /* up to curl-7.9.2 */
                curl_err = curl_easy_setopt(context->curl,
                        CURLOPT_SSLCERTPASSWD,
                        context->pki_credentials->passphrase);
#endif
            }
        }
    }

    /* Set authorization cookie for OTP session */
    if (!curl_err && context->otp != NULL) {
        isds_log(ILF_SEC, ILL_INFO,
                _("Cookies will be stored and send "
                    "because context has been authorized by OTP.\n"));
        curl_err = curl_easy_setopt(context->curl, CURLOPT_COOKIEFILE, "");
    }

    /* Set timeout */
    if (!curl_err) {
        curl_err = curl_easy_setopt(context->curl, CURLOPT_NOSIGNAL, 1);
    }
    if (!curl_err && context->timeout) {
#if HAVE_DECL_CURLOPT_TIMEOUT_MS /* Since curl-7.16.2 */
        curl_err = curl_easy_setopt(context->curl, CURLOPT_TIMEOUT_MS,
                context->timeout);
#else
        curl_err = curl_easy_setopt(context->curl, CURLOPT_TIMEOUT,
                context->timeout / 1000);
#endif /* not HAVE_DECL_CURLOPT_TIMEOUT_MS */
    }

    /* Register callback */
    if (context->progress_callback) {
        if (!curl_err) {
            curl_err = curl_easy_setopt(context->curl, CURLOPT_NOPROGRESS, 0);
        }
        if (!curl_err) {
            curl_err = curl_easy_setopt(context->curl,
                    CURLOPT_PROGRESSFUNCTION, progress_proxy);
        }
        if (!curl_err) {
            curl_err = curl_easy_setopt(context->curl, CURLOPT_PROGRESSDATA,
                    context);
        }
    }

    /* Set other CURL features */
    if (!curl_err) {
        curl_err = curl_easy_setopt(context->curl, CURLOPT_FAILONERROR, 0);
    }

    /* Set get-response function */
    if (!curl_err) {
        curl_err = curl_easy_setopt(context->curl, CURLOPT_WRITEFUNCTION,
                write_body);
    }
    if (!curl_err) {
        curl_err = curl_easy_setopt(context->curl, CURLOPT_WRITEDATA, &body);
    }

    /* Set get-response-headers function if needed.
     * XXX: Both CURLOPT_HEADERFUNCTION and CURLOPT_WRITEHEADER must be set or
     * unset at the same time (see curl_easy_setopt(3)) ASAP, otherwise old
     * invalid CURLOPT_WRITEHEADER value could be derefenced. */
    if (!curl_err) {
        curl_err = curl_easy_setopt(context->curl, CURLOPT_HEADERFUNCTION,
                (response_otp_headers == NULL) ? NULL: write_header);
    }
    if (!curl_err) {
        curl_err = curl_easy_setopt(context->curl, CURLOPT_WRITEHEADER,
                response_otp_headers);
    }

    /* Set MIME types and headers requires by SOAP 1.1.
     * SOAP 1.1 requires text/xml, SOAP 1.2 requires application/soap+xml */
    if (!curl_err) {
        headers = curl_slist_append(headers,
                "Accept: application/soap+xml,application/xml,text/xml");
        if (!headers) {
            err = IE_NOMEM;
            goto leave;
        }
        headers = curl_slist_append(headers, "Content-Type: text/xml");
        if (!headers) {
            err = IE_NOMEM;
            goto leave;
        }
        headers = curl_slist_append(headers, "SOAPAction: ");
        if (!headers) {
            err = IE_NOMEM;
            goto leave;
        }
        curl_err = curl_easy_setopt(context->curl, CURLOPT_HTTPHEADER, headers);
    }
    if (!curl_err) {
        /* Set user agent identification */
        curl_err = curl_easy_setopt(context->curl, CURLOPT_USERAGENT,
                "libisds/" PACKAGE_VERSION);
    }

    if (use_get) {
        /* Set GET request */
        if (!curl_err) {
            curl_err = curl_easy_setopt(context->curl, CURLOPT_HTTPGET, 1);
        }
    } else {
        /* Set POST request body */
        if (!curl_err) {
            curl_err = curl_easy_setopt(context->curl, CURLOPT_POST, 1);
        }
        if (!curl_err) {
            curl_err = curl_easy_setopt(context->curl, CURLOPT_POSTFIELDS, request);
        }
        if (!curl_err) {
            curl_err = curl_easy_setopt(context->curl, CURLOPT_POSTFIELDSIZE,
                    request_length);
        }
    }

    /* Check for errors so far */
    if (curl_err) {
        isds_log_message(context, curl_easy_strerror(curl_err));
        err = IE_NETWORK;
        goto leave;
    }

    isds_log(ILF_HTTP, ILL_DEBUG, _("Sending %s request to <%s>\n"),
            use_get ? "GET" : "POST", url);
    if (!use_get) {
        isds_log(ILF_HTTP, ILL_DEBUG,
                _("POST body length: %zu, content follows:\n"), request_length);
        isds_log(ILF_HTTP, ILL_DEBUG, "%.*s\n", request_length, request);
        isds_log(ILF_HTTP, ILL_DEBUG, _("End of POST body\n"));
    }
    if ((log_facilities & ILF_HTTP) && (log_level >= ILL_DEBUG) ) {
        curl_easy_setopt(context->curl, CURLOPT_VERBOSE, 1);
        curl_easy_setopt(context->curl, CURLOPT_DEBUGFUNCTION, log_curl);
    } else {
        curl_easy_setopt(context->curl, CURLOPT_VERBOSE, 0);
        curl_easy_setopt(context->curl, CURLOPT_DEBUGFUNCTION, NULL);
    }
    

    /*  Do the request */
    curl_err = curl_easy_perform(context->curl);

    if (!curl_err)
        curl_err = curl_easy_getinfo(context->curl, CURLINFO_CONTENT_TYPE,
            &content_type);

    if (curl_err) {
        /* TODO: CURL is not internationalized yet. Collect CURL messages for
         * I18N. */
        isds_printf_message(context,
                _("%s: %s"), url, _(curl_easy_strerror(curl_err)));
        if (curl_err == CURLE_ABORTED_BY_CALLBACK)
            err = IE_ABORTED;
        else
            err = IE_NETWORK;
        goto leave;
    }

    isds_log(ILF_HTTP, ILL_DEBUG, _("Final response to %s received\n"), url);
    isds_log(ILF_HTTP, ILL_DEBUG,
            _("Response body length: %zu, content follows:\n"),
            body.length);
    isds_log(ILF_HTTP, ILL_DEBUG, "%.*s\n", body.length, body.data);
    isds_log(ILF_HTTP, ILL_DEBUG, _("End of response body\n"));


    /* Extract MIME type and charset */
    if (content_type) {
        char *sep;
        size_t offset;

        sep = strchr(content_type, ';');
        if (sep) offset = (size_t) (sep - content_type);
        else offset = strlen(content_type);

        if (mime_type) {
            *mime_type = malloc(offset + 1);
            if (!*mime_type) {
                err = IE_NOMEM;
                goto leave;
            }
            memcpy(*mime_type, content_type, offset);
            (*mime_type)[offset] = '\0';
        }

        if (charset) {
            if (!sep) {
               *charset = NULL;
            } else {
                sep = strstr(sep, "charset=");
                if (!sep) {
                    *charset = NULL;
                } else {
                    *charset = strdup(sep + 8);
                    if (!*charset) {
                        err = IE_NOMEM;
                        goto leave;
                    }
                }
            }
        }
    }

    /* Get HTTP response code */
    if (http_code) {
        curl_err = curl_easy_getinfo(context->curl,
                CURLINFO_RESPONSE_CODE, http_code);
        if (curl_err) {
            err = IE_ERROR;
            goto leave;
        }
    }

    /* Store OTP authentication results */
    if (response_otp_headers && response_otp_headers->is_complete) {
        isds_log(ILF_SEC, ILL_DEBUG,
                _("OTP authentication headers received: "
                    "method=%s, code=%s, message=%s\n"),
                response_otp_headers->method, response_otp_headers->code,
                response_otp_headers->message);

        /* XXX: Don't make unknown code fatal. Missing code can be succcess if
         * HTTP code is 302. This is checked in _isds_soap(). */
        response_otp_headers->resolution =
            string2isds_otp_resolution(response_otp_headers->code);
        
        if (response_otp_headers->message != NULL) {
            char *message_locale = _isds_utf82locale(response_otp_headers->message);
            /* _isds_utf82locale() return NULL on inconverable string. Do not
             * panic on it.
             * TODO: Escape such characters.
             * if (message_locale == NULL) {
                err = IE_NOMEM;
                goto leave;
            }*/
            isds_printf_message(context,
                    _("Server returned OTP authentication message: %s"),
                    message_locale);
            free(message_locale);
        }

        char *next_url = NULL; /* Weak pointer managed by cURL */
        curl_err = curl_easy_getinfo(context->curl, CURLINFO_REDIRECT_URL,
                &next_url);
        if (curl_err) {
            err = IE_ERROR;
            goto leave;
        }
        if (next_url != NULL) {
            isds_log(ILF_SEC, ILL_DEBUG,
                    _("OTP authentication headers redirect to: <%s>\n"),
                    next_url);
            free(response_otp_headers->redirect);
            response_otp_headers->redirect = strdup(next_url);
            if (response_otp_headers->redirect == NULL) {
                err = IE_NOMEM;
                goto leave;
            }
        }
    }
leave:
    curl_slist_free_all(headers);

    if (err) {
        free(body.data);
        body.data = NULL;
        body.length = 0;

        if (mime_type) {
            free(*mime_type);
            *mime_type = NULL;
        }
        if (charset) {
            free(*charset);
            *charset = NULL;
        }

        if (err != IE_ABORTED) _isds_close_connection(context);
    }

    *response = body.data;
    *response_length = body.length;

    return err;
}


/* Do SOAP request.
 * @context holds the base URL,
 * @file is a (CGI) file of SOAP URL,
 * @request is XML node set with SOAP request body. 
 * @file must be NULL, @request should be NULL rather than empty, if they should
 * not be signaled in the SOAP request.
 * @reponse is automatically allocated() node set with SOAP response body.
 * You must xmlFreeNodeList() it. This is literal body, empty (NULL), one node
 * or more nodes can be returned.
 * @raw_response is automatically allocated bit stream with response body. Use
 * NULL if you don't care
 * @raw_response_length is size of @raw_response in bytes
 * In case of error the response will be deallocated automatically.
 * Side effect: message buffer */
_hidden isds_error _isds_soap(struct isds_ctx *context, const char *file,
        const xmlNodePtr request, xmlNodePtr *response,
        void **raw_response, size_t *raw_response_length) {

    isds_error err = IE_SUCCESS;
    char *url = NULL;
    char *mime_type = NULL;
    long http_code = 0;
    struct auth_headers response_otp_headers;
    xmlBufferPtr http_request = NULL;
    xmlSaveCtxtPtr save_ctx = NULL;
    xmlDocPtr request_soap_doc = NULL;
    xmlNodePtr request_soap_envelope = NULL, request_soap_body = NULL;
    xmlNsPtr soap_ns = NULL;
    void *http_response = NULL;
    size_t response_length = 0;
    xmlDocPtr response_soap_doc = NULL;
    xmlNodePtr response_root = NULL;
    xmlXPathContextPtr xpath_ctx = NULL;
    xmlXPathObjectPtr response_soap_headers = NULL, response_soap_body = NULL,
                      response_soap_fault = NULL;


    if (!context) return IE_INVALID_CONTEXT;
    if (!response) return IE_INVAL;
    if (!raw_response_length && raw_response) return IE_INVAL;

    xmlFreeNodeList(*response);
    *response = NULL;
    if (raw_response) *raw_response = NULL;

    url = _isds_astrcat(context->url, file);
    if (!url) return IE_NOMEM;

    /* Build SOAP request envelope */
    request_soap_doc = xmlNewDoc(BAD_CAST "1.0");
    if (!request_soap_doc) {
        isds_log_message(context, _("could not build soap request document"));
        err = IE_ERROR;
        goto leave;
    }
    request_soap_envelope = xmlNewNode(NULL, BAD_CAST "Envelope");
    if (!request_soap_envelope) {
        isds_log_message(context, _("Could not build SOAP request envelope"));
        err = IE_ERROR;
        goto leave;
    }
    xmlDocSetRootElement(request_soap_doc, request_soap_envelope);
    /* Only this way we get namespace definition as @xmlns:soap,
     * otherwise we get namespace prefix without definition */
    soap_ns = xmlNewNs(request_soap_envelope, BAD_CAST SOAP_NS, NULL);
    if(!soap_ns) {
        isds_log_message(context, _("Could not create SOAP name space"));
        err = IE_ERROR;
        goto leave;
    }
    xmlSetNs(request_soap_envelope, soap_ns);
    request_soap_body = xmlNewChild(request_soap_envelope, NULL,
            BAD_CAST "Body", NULL);
    if (!request_soap_body) {
        isds_log_message(context,
                _("Could not add Body to SOAP request envelope"));
        err = IE_ERROR;
        goto leave;
    }

    /* Append request XML node set to SOAP body if request is not empty */
    /* XXX: Copy of request must be used, otherwise xmlFreeDoc(request_soap_doc)
     * would destroy this outer structure. */
    if (request) {
        xmlNodePtr request_copy = xmlCopyNodeList(request);
        if (!request_copy) {
            isds_log_message(context,
                    _("Could not copy request content"));
            err = IE_ERROR;
            goto leave;
        }
        if (!xmlAddChildList(request_soap_body, request_copy)) {
            xmlFreeNodeList(request_copy);
            isds_log_message(context,
                    _("Could not add request content to SOAP "
                        "request envelope"));
            err = IE_ERROR;
            goto leave;
        }
    }


    /* Serialize the SOAP request into HTTP request body */
    http_request = xmlBufferCreate();
    if (!http_request) {
        isds_log_message(context,
                _("Could not create xmlBuffer for HTTP request body"));
        err = IE_ERROR;
        goto leave;
    }
    /* Last argument 1 means format the XML tree. This is pretty but it breaks
     * XML document transport as it adds text nodes (indentiation) between
     * elements. */
    save_ctx = xmlSaveToBuffer(http_request, "UTF-8", 0);
    if (!save_ctx) {
        isds_log_message(context,
                _("Could not create XML serializer"));
        err = IE_ERROR;
        goto leave;
    }
    /* XXX: According LibXML documentation, this function does not return
     * meaningful value yet */
    xmlSaveDoc(save_ctx, request_soap_doc);
    if (-1 == xmlSaveFlush(save_ctx)) {
        isds_log_message(context,
                _("Could not serialize SOAP request to HTTP request body"));
        err = IE_ERROR;
        goto leave;
    }

    if (context->otp != NULL)
        memset(&response_otp_headers, 0, sizeof(response_otp_headers));
redirect:
    if (context->otp != NULL) auth_headers_free(&response_otp_headers);
    isds_log(ILF_SOAP, ILL_DEBUG,
            _("SOAP request to sent to %s:\n%.*s\nEnd of SOAP request\n"),
            url, http_request->use, http_request->content);

    err = http(context, url, 0, http_request->content, http_request->use,
            &http_response, &response_length,
            &mime_type, NULL, &http_code,
            (context->otp == NULL) ? NULL: &response_otp_headers);

    /* TODO: HTTP binding for SOAP prescribes non-200 HTTP return codes
     * to be processed too. */

    if (err) {
        goto leave;
    }

    /* Check for HTTP return code */
    isds_log(ILF_SOAP, ILL_DEBUG, _("Server returned %ld HTTP code\n"),
            http_code);
    switch (http_code) {
        /* XXX: We must see which code is used for not permitted ISDS
         * operation like downloading message without proper user
         * permissions. In that case we should keep connection opened. */
        case 302:
            if (context->otp) {
                if (response_otp_headers.resolution == OTP_RESOLUTION_UNKNOWN)
                    context->otp->resolution = OTP_RESOLUTION_SUCCESS;
                else
                    context->otp->resolution = response_otp_headers.resolution;
                err = IE_PARTIAL_SUCCESS;
                isds_printf_message(context,
                        _("Server redirects on <%s> because OTP authentication "
                            "succeeded."),
                        url);
                if (context->otp->otp_code != NULL &&
                        response_otp_headers.redirect != NULL) {
                    /* XXX: If OTP code is known, this must be second OTP phase, so
                     * send final POST request and unset Basic authentication
                     * from cURL context as cookie is used instead. */
                    free(url);
                    url = response_otp_headers.redirect;
                    response_otp_headers.redirect = NULL;
                    _isds_discard_credentials(context);
                    err = unset_http_authorization(context);
                    if (err) {
                        isds_log_message(context, _("Could not remove "
                                    "credentials from CURL handle."));
                        goto leave;
                    }
                    goto redirect;
                } else {
                    /* XXX: Otherwise bail out to ask application for OTP code. */
                    goto leave;
                }
            } else {
                err = IE_HTTP;
                isds_printf_message(context,
                        _("Code 302: Server redirects on <%s> request. "
                            "Redirection is forbidden in stateless mode."),
                        url);
                goto leave;
            }
            break;
        case 401:   /* ISDS server returns 401 even if Authorization
                       presents. */
        case 403:   /* HTTP/1.0 prescribes 403 if Authorization presents. */
            if (context->otp)
                context->otp->resolution = response_otp_headers.resolution;
            err = IE_NOT_LOGGED_IN;
            isds_log_message(context, _("Authentication failed"));
            goto leave;
            break;
        case 404:
            err = IE_HTTP;
            isds_printf_message(context,
                    _("Code 404: Document (%s) not found on server"), url);
            goto leave;
            break;
        /* 500 should return standard SOAP message */
    }

    /* Check for Content-Type: text/xml.
     * Do it after HTTP code check because 401 Unauthorized returns HTML web
     * page for browsers. */
    if (mime_type && strcmp(mime_type, "text/xml")
            && strcmp(mime_type, "application/soap+xml")
            && strcmp(mime_type, "application/xml")) {
        char *mime_type_locale = _isds_utf82locale(mime_type);
        isds_printf_message(context,
                _("%s: bad MIME type sent by server: %s"), url,
                mime_type_locale);
        free(mime_type_locale);
        err = IE_SOAP;
        goto leave;
    }
    
    /* TODO: Convert returned body into XML default encoding */

    /* Parse the HTTP body as XML */
    response_soap_doc = xmlParseMemory(http_response, response_length);
    if (!response_soap_doc) {
        err = IE_XML;
        goto leave;
    }

    xpath_ctx = xmlXPathNewContext(response_soap_doc);
    if (!xpath_ctx) {
        err = IE_ERROR;
        goto leave;
    }

    if (_isds_register_namespaces(xpath_ctx, MESSAGE_NS_UNSIGNED)) {
        err = IE_ERROR;
        goto leave;
    }

    isds_log(ILF_SOAP, ILL_DEBUG,
            _("SOAP response received:\n%.*s\nEnd of SOAP response\n"),
            response_length, http_response);


    /* Check for SOAP version */
    response_root = xmlDocGetRootElement(response_soap_doc);
    if (!response_root) {
        isds_log_message(context, "SOAP response has no root element");
        err = IE_SOAP;
        goto leave;
    }
    if (xmlStrcmp(response_root->name, BAD_CAST "Envelope") ||
            xmlStrcmp(response_root->ns->href, BAD_CAST SOAP_NS)) {
        isds_log_message(context, "SOAP response is not SOAP 1.1 document");
        err = IE_SOAP;
        goto leave;
    }

    /* Check for SOAP Headers */
    response_soap_headers = xmlXPathEvalExpression(
            BAD_CAST "/soap:Envelope/soap:Header/"
            "*[@soap:mustUnderstand/text() = true()]", xpath_ctx);
    if (!response_soap_headers) {
        err = IE_ERROR;
        goto leave;
    }
    if (!xmlXPathNodeSetIsEmpty(response_soap_headers->nodesetval)) {
        isds_log_message(context,
                _("SOAP response requires unsupported feature"));
        /* TODO: log the headers 
         * xmlChar *fragment = NULL;
         * fragment = xmlXPathCastNodeSetToSting(response_soap_headers->nodesetval);*/
        err = IE_NOTSUP;
        goto leave;
    }

    /* Get SOAP Body */
    response_soap_body = xmlXPathEvalExpression(
            BAD_CAST "/soap:Envelope/soap:Body", xpath_ctx);
    if (!response_soap_body) {
        err = IE_ERROR;
        goto leave;
    }
    if (xmlXPathNodeSetIsEmpty(response_soap_body->nodesetval)) {
        isds_log_message(context,
                _("SOAP response does not contain SOAP Body element"));
        err = IE_SOAP;
        goto leave;
    }
    if (response_soap_body->nodesetval->nodeNr > 1) {
        isds_log_message(context,
                _("SOAP response has more than one Body element"));
        err = IE_SOAP;
        goto leave;
    }

    /* Check for SOAP Fault */
    response_soap_fault = xmlXPathEvalExpression(
            BAD_CAST "/soap:Envelope/soap:Body/soap:Fault", xpath_ctx);
    if (!response_soap_fault) {
        err = IE_ERROR;
        goto leave;
    }
    if (!xmlXPathNodeSetIsEmpty(response_soap_fault->nodesetval)) {
        /* Server signals Fault. Gather error message and croak. */
        /* XXX: Only first message is passed */
        char *message = NULL, *message_locale = NULL;
        xpath_ctx->node = response_soap_fault->nodesetval->nodeTab[0];
        xmlXPathFreeObject(response_soap_fault);
        /* XXX: faultstring and faultcode are in no name space according
         * ISDS specification */
        /* First more verbose faultstring */
        response_soap_fault = xmlXPathEvalExpression(
                BAD_CAST "faultstring[1]/text()", xpath_ctx);
        if (response_soap_fault &&
                !xmlXPathNodeSetIsEmpty(response_soap_fault->nodesetval)) {
            message = (char *)
                xmlXPathCastNodeSetToString(response_soap_fault->nodesetval);
            message_locale = _isds_utf82locale(message);
        }
        /* If not available, try shorter faultcode */
        if (!message_locale) {
            free(message);
            xmlXPathFreeObject(response_soap_fault);
            response_soap_fault = xmlXPathEvalExpression(
                    BAD_CAST "faultcode[1]/text()", xpath_ctx);
            if (response_soap_fault &&
                    !xmlXPathNodeSetIsEmpty(response_soap_fault->nodesetval)) {
                message = (char *)
                    xmlXPathCastNodeSetToString(
                            response_soap_fault->nodesetval);
                message_locale = _isds_utf82locale(message);
            }
        }

        /* Croak */
        if (message_locale) 
            isds_printf_message(context, _("SOAP response signals Fault: %s"),
                    message_locale);
        else
            isds_log_message(context, _("SOAP response signals Fault"));

        free(message_locale);
        free(message);

        err = IE_SOAP;
        goto leave;
    }


    /* Extract XML Tree with ISDS response from SOAP envelope and return it.
     * XXX: response_soap_body is Body, we need children which may not exist
     * (i.e. empty Body). */
    /* TODO: Destroy SOAP response but Body children. This is more memory
     * friendly than copying (potentially) fat body */
    if (response_soap_body->nodesetval->nodeTab[0]->children) {
        *response = xmlDocCopyNodeList(response_soap_doc,
                response_soap_body->nodesetval->nodeTab[0]->children);
        if (!*response) {
            err = IE_NOMEM;
            goto leave;
        }
    } else *response = NULL;

    /* Save raw response */
    if (raw_response) {
        *raw_response = http_response;
        *raw_response_length = response_length;
        http_response = NULL;
    }


leave:
    if (err) {
        xmlFreeNodeList(*response);
        *response = NULL;
    }

    xmlXPathFreeObject(response_soap_fault);
    xmlXPathFreeObject(response_soap_body);
    xmlXPathFreeObject(response_soap_headers);
    xmlXPathFreeContext(xpath_ctx);
    xmlFreeDoc(response_soap_doc);
    if (context->otp != NULL) auth_headers_free(&response_otp_headers);
    free(mime_type);
    free(http_response);
    xmlSaveClose(save_ctx);
    xmlBufferFree(http_request);
    xmlFreeDoc(request_soap_doc); /* recursive, frees request_body, soap_ns*/
    free(url);

    return err;
}


/* Invalidate session cookie for otp authenticated @context */
_hidden isds_error _isds_invalidate_otp_cookie(struct isds_ctx *context) {
    isds_error err;
    char *url = NULL;
    int length, slashes;
    long http_code;
    void *response = NULL;
    size_t response_length;

    if (context == NULL || context->otp == NULL) return IE_INVALID_CONTEXT;
    if (context->curl == NULL) return IE_CONNECTION_CLOSED;

    /* Find length of base URL from context URL */
    if (context->url == NULL) {
        isds_log_message(context, _("Base URL could not have been determined "
                    "from context URL"));
        return IE_ERROR;
    }
    for (length = 0, slashes = 0; context->url[length] != '\0'; length++) {
        if (context->url[length] == '/') slashes++;
        if (slashes == 3) break;
    }
    if (slashes != 3) {
        isds_log_message(context, _("Base URL could not have been determined "
                    "from context URL"));
        return IE_ERROR;
    }
    length++;
    
    /* Build logout URL */
    /*"https://DOMAINNAME/as/processLogout?uri=https://DOMAINNAME/apps/DS/WEB_SERVICE_ENDPOINT"*/
    if (-1 == isds_asprintf(&url, "%1$.*2$sas/processLogout?uri=%1$sDS/dz",
                context->url, length))
        return IE_NOMEM;

    /* Invalidate the cookie by GET request */
    err = http(context,
            url, 1,
            NULL, 0,
            &response, &response_length,
            NULL, NULL, &http_code,
            NULL);
    free(response);
    free(url);
    if (err) {
        /* long message set by http() */
    } else if (http_code != 200) {
        /* TODO: Specification does not define response for this request.
         * Especially it does not state whether direct 200 or 302 redirect is
         * sent. We need to check real implementation. */
        err = IE_ISDS;
        isds_printf_message(context, _("Cookie for OTP authenticated "
                    "connection to <%s> could not been invalidated"),
                context->url);
    } else {
        isds_log(ILF_SEC, ILL_DEBUG, _("Cookie for OTP authenticated "
                    "connection to <%s> has been invalidated.\n"),
                context->url);
    }
    return err;
}


/* LibXML functions:
 *
 * void xmlInitParser(void)
 *  Initialization function for the XML parser. This is not reentrant.  Call
 *  once before processing in case of use in multithreaded programs.
 *
 * int xmlInitParserCtxt(xmlParserCtxtPtr ctxt)
 *  Initialize a parser context
 *
 * xmlDocPtr xmlCtxtReadDoc(xmlParserCtxtPtr ctxt, const xmlChar * cur,
 *  const * char URL, const char * encoding, int options);
 *  Parse in-memory NULL-terminated document @cur.
 *
 * xmlDocPtr xmlParseMemory(const char * buffer, int size)
 *  Parse an XML in-memory block and build a tree.
 *
 * xmlParserCtxtPtr xmlCreateMemoryParserCtxt(const char * buffer, int
 *  size);
 *  Create a parser context for an XML in-memory document.
 *
 * xmlParserCtxtPtr xmlCreateDocParserCtxt(const xmlChar * cur)
 *  Creates a parser context for an XML in-memory document.
 *
 * xmlDocPtr xmlCtxtReadMemory(xmlParserCtxtPtr ctxt,
 *  const char * buffer, int size, const char * URL, const char * encoding,
 *  int options)
 *  Parse an XML in-memory document and build a tree. This reuses the existing
 *  @ctxt parser context.

 * void xmlCleanupParser(void)
 *  Cleanup function for the XML library. It tries to reclaim all parsing
 *  related glob document related memory. Calling this function should not
 *  prevent reusing the libr finished using the library or XML document built
 *  with it.
 *
 * void xmlClearParserCtxt(xmlParserCtxtPtr ctxt)
 *  Clear (release owned resources) and reinitialize a parser context.
 *
 * void  xmlCtxtReset(xmlParserCtxtPtr ctxt)
 *  Reset a parser context
 *
 * void  xmlFreeParserCtxt(xmlParserCtxtPtr ctxt)
 *  Free all the memory used by a parser context. However the parsed document
 *  in ctxt->myDoc is not freed.
 *
 * void xmlFreeDoc(xmlDocPtr cur)
 *  Free up all the structures used by a document, tree included.
 */
