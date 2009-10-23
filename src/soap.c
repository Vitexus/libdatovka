#define _XOPEN_SOURCE 500   /* strdup from string.h */
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


/* Do HTTP request.
 * @context holds the base URL,
 * @url is a (CGI) file of SOAP URL,
 * @request is body for POST request 
 * @request_length is length of @request in bytes
 * @reponse is automatically reallocated() buffer to fit HTTP response with
 * @response_length (does not need to match allocatef memory exactly). You must
 * free() the @response.
 * @mime_type is automatically allocated MIME type send by server (*NULL if not
 * sent). Set NULL if you don't care.
 * @charset is charset of the body signaled by server. The same constrains
 * like on @mime_type apply.
 * In case of error, the response memory, MIME type, charset and lenght will be
 * deallocated and zerod automatically. Thus be sure they are preallocated or
 * they points to NULL.
 * Side effect: message buffer */
static isds_error http(struct isds_ctx *context, const char *url,
        const void *request, const size_t request_length,
        void **response, size_t *response_length,
        char **mime_type, char**charset) {

    CURLcode curl_err;
    isds_error err = IE_SUCCESS;
    struct soap_body body;
    char *content_type;
    struct curl_slist *headers = NULL;


    if (!context) return IE_INVALID_CONTEXT;
    if (!url) return IE_INVAL;
    if (request_length > 0 && !request) return IE_INVAL;
    if (!response || !response_length) return IE_INVAL;

    /* Set the body here to allow deallocataion in leave block */
    body.data = *response;
    body.length = 0;

    /* Set Request-URI */
    curl_err = curl_easy_setopt(context->curl, CURLOPT_URL, url);
    if (!curl_err && context->username) {
        curl_err = curl_easy_setopt(context->curl, CURLOPT_USERNAME,
                context->username);
    }

    /* Set credentials */
    if (!curl_err && context->password) {
        curl_err = curl_easy_setopt(context->curl, CURLOPT_PASSWORD,
                context->password);
    }
    if (!curl_err) {
        curl_err = curl_easy_setopt(context->curl, CURLOPT_FAILONERROR, 1);
    }

    /* Set get-response function */
    if (!curl_err) {
        curl_err = curl_easy_setopt(context->curl, CURLOPT_WRITEFUNCTION,
                write_body);
    }
    if (!curl_err) {
        curl_err = curl_easy_setopt(context->curl, CURLOPT_WRITEDATA, &body);
    }

    /* Set MIME types and user agent identification */
    if (!curl_err) {
        headers = curl_slist_append(headers, "Accept: application/soap+xml");
        if (!headers) {
            err = IE_NOMEM;
            goto leave;
        }
        headers = curl_slist_append(headers, "Content-Type: application/soap+xml");
        if (!headers) {
            err = IE_NOMEM;
            goto leave;
        }
        curl_err = curl_easy_setopt(context->curl, CURLOPT_HTTPHEADER, headers);
    }
    if (!curl_err) {
        /* TODO: Present library version, curl etc. in User-Agent */
        curl_err = curl_easy_setopt(context->curl, CURLOPT_USERAGENT, "libisds");
    }

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

    /* Check for errors so far */
    if (curl_err) {
        isds_log_message(context, curl_easy_strerror(curl_err));
        err = IE_NETWORK;
        goto leave;
    }

    /*  Do the request */
    curl_err = curl_easy_perform(context->curl);

    if (!curl_err)
        curl_err = curl_easy_getinfo(context->curl, CURLINFO_CONTENT_TYPE,
            &content_type);

    if (curl_err) {
        isds_log_message(context, url);
        isds_append_message(context, _(": "));
        isds_append_message(context, curl_easy_strerror(curl_err));
        err = IE_NETWORK;
        goto leave;
    }

    /* Extract MIME type and charset */
    if (content_type) {
        char *sep = strchr(content_type, ';');
        size_t offset = (sep) ? (size_t) (sep - content_type) : 0;
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

leave:
    free(headers);

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

        curl_easy_cleanup(context->curl);
        context->curl = NULL;
    }

    *response = body.data;
    *response_length = body.length;

    return err;
}


/* Do SOAP request.
 * @context holds the base URL,
 * @file is a (CGI) file of SOAP URL,
 * @request is XML document with request. 
 * @request_length is lenght of @request in bytes
 * @file and @request must be NULL rather than empty strings, if the should
 * not be signaled in the SOAP request.
 * @reponse is automatically reallocated() buffer to fit SOAP response with
 * @response_length (does not need to match allocatef memory exactly). You must
 * free() the @response. In case of error the response memory and lenght will
 * be deallocated and zerod automatically.
 * Side effect: message buffer */
_hidden isds_error soap(struct isds_ctx *context, const char *file,
        const void *request, const size_t request_length,
        void **response, size_t *response_length) {

    char *url;
    isds_error err = IE_SUCCESS;
    char *mime_type = NULL;


    if (!context) return IE_INVALID_CONTEXT;
    if (request_length > 0 && !request) return IE_INVAL;
    if (!response || !response_length) return IE_INVAL;

    url = astrcat(context->url, file);
    if (!url) return IE_NOMEM;

    /* TODO: Wrap the request into SOAP envelope */

    err = http(context, url, request, request_length,
            response, response_length,
            &mime_type, NULL);

    if (err) {
        goto leave;
    }

    /* TODO: Check for Content-Type: application/soap+xml */
    if (mime_type && strcmp(mime_type, "application/soap+xml")
            && strcmp(mime_type, "application/xml")
            && strcmp(mime_type, "text/xml")) {
        isds_log_message(context, url);
        isds_append_message(context, _(": bad MIME type sent by server: "));
        isds_append_message(context, mime_type);
        err = IE_SOAP;
        goto leave;
    }
    /* TODO: Convert returned body into XML default encoding */
    /* TODO: Extract XML Tree with ISDS response from SOAP envelope and return
     * it*/

leave:
    free(url);

    if (err) {
        free(*response);
        *response =  NULL;
        *response_length = 0;
        free(mime_type);
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
