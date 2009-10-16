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
    char *login_url;

    if (!context) return IE_INVALID_CONTEXT;
    if (!response || !length) return IE_INVAL;

    login_url = astrcat(context->url, "login");
    if (!login_url) return IE_NOMEM;

    curl_err = curl_easy_setopt(context->curl, CURLOPT_URL, login_url);
    free(login_url);
    if (!curl_err) {
        curl_err = curl_easy_setopt(context->curl, CURLOPT_FAILONERROR, 1);
    }

    if (curl_err) {
        isds_log_message(context, curl_easy_strerror(curl_err));
        curl_easy_cleanup(context->curl);
        context->curl = NULL;
        return IE_NETWORK;
    }

    curl_err = curl_easy_perform(context->curl);
    if (curl_err) {
        isds_log_message(context, curl_easy_strerror(curl_err));
        curl_easy_cleanup(context->curl);
        context->curl = NULL;
        return IE_NETWORK;
    }

    *length = 0; 
    return IE_SUCCESS;
}
