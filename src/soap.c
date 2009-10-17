#include "isds_priv.h"
#include "soap.h"
#include "utils.h"
#include <stdlib.h>
#include <string.h>

/* Private structure for write_body() call back */
struct soap_body {
    void *data;
    size_t length;
};


/* CURL call back function called when chunk of HTTP reponse body is available.
 * @buffer points to new data
 * @size * @nmemb is length of the chunk in bytes. Zero means empty body.
 * @userp is private structure.
 * Must reuturn the length of the chunk, otherwise CURL will signal
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
        const char *request, void **response, size_t *length) {

    CURLcode curl_err;
    char *url;
    isds_error err = IE_SUCCESS;
    struct soap_body body;


    if (!context) return IE_INVALID_CONTEXT;
    if (!response || !length) return IE_INVAL;

    url = astrcat(context->url, file);
    if (!url) return IE_NOMEM;

    curl_err = curl_easy_setopt(context->curl, CURLOPT_URL, url);
    if (!curl_err) {
        curl_err = curl_easy_setopt(context->curl, CURLOPT_FAILONERROR, 1);
    }
    if (!curl_err) {
        curl_err = curl_easy_setopt(context->curl, CURLOPT_WRITEFUNCTION,
                write_body);
    }
    if (!curl_err) {
        body.data = NULL;
        body.length = 0;
        curl_err = curl_easy_setopt(context->curl, CURLOPT_WRITEDATA, &body);
    }

    if (curl_err) {
        isds_log_message(context, curl_easy_strerror(curl_err));
        err = IE_NETWORK;
        goto leave;
    }

    curl_err = curl_easy_perform(context->curl);
    if (curl_err) {
        isds_log_message(context, url);
        isds_append_message(context, _(": "));
        isds_append_message(context, curl_easy_strerror(curl_err));
        err = IE_NETWORK;
        goto leave;
    }

    *response = body.data;
    *length = body.length;

leave:
    free(url);
    if (err) {
        curl_easy_cleanup(context->curl);
        context->curl = NULL;
    }
    return err;
}


