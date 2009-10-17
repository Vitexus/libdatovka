#include "isds_priv.h"
#include "soap.h"
#include "utils.h"
#include <stdlib.h>

/* Do SOAP request.
 * @file is a (CGI) file of SOAP URL,
 * @context holds the base URL,
 * @request is XML document with request (NULL terminated). 
 * @file and @request must be NULL rather than empty strings, if the should
 * not be signaled in the SOAP request.
 * @reponse is automatically reallocated() buffer to fit SOAP response with
 * @length (does not need to match allocates memory exactly. You must free() the
 * @response. */
_hidden isds_error soap(struct isds_ctx *context, const char *file,
        const char *request, void **response, size_t *length ) {

    CURLcode curl_err;
    char *url;
    isds_error err = IE_SUCCESS;

    if (!context) return IE_INVALID_CONTEXT;
    if (!response || !length) return IE_INVAL;

    url = astrcat(context->url, file);
    if (!url) return IE_NOMEM;

    curl_err = curl_easy_setopt(context->curl, CURLOPT_URL, url);
    if (!curl_err) {
        curl_err = curl_easy_setopt(context->curl, CURLOPT_FAILONERROR, 1);
    }

    if (curl_err) {
        isds_log_message(context, curl_easy_strerror(curl_err));
        err = IE_NETWORK;
        goto leave;
    }

    curl_err = curl_easy_perform(context->curl);
    if (curl_err) {
        isds_log_message(context, curl_easy_strerror(curl_err));
        err = IE_NETWORK;
        goto leave;
    }

    *length = 0; 

leave:
    free(url);
    if (err) {
        curl_easy_cleanup(context->curl);
        context->curl = NULL;
    }
    return err;
}
