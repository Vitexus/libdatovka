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


/* Close connection to server and destroy CURL handle associated
 * with @context */
_hidden isds_error close_connection(struct isds_ctx *context) {
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
 * @http_code is final HTTP code returned by server. This can be 200, 401, 500
 * or any other one. Pass NULL if you don't interrest.
 * In case of error, the response memory, MIME type, charset and lenght will be
 * deallocated and zerod automatically. Thus be sure they are preallocated or
 * they points to NULL.
 * Be ware that successful return value does not mean the HTTP request has
 * been accepted by the server. You must cosult @http_code. OTOH, failure
 * return value means the request could not been sent (e.g. SSL error).
 * Side effect: message buffer */
static isds_error http(struct isds_ctx *context, const char *url,
        const void *request, const size_t request_length,
        void **response, size_t *response_length,
        char **mime_type, char **charset, long *http_code) {

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
                    "since now\n"), context->tls_ca_file);
        curl_err = curl_easy_setopt(context->curl, CURLOPT_CAINFO,
                context->tls_ca_file);
    }


    /* Set credentials */
    if (!curl_err && context->username) {
        curl_err = curl_easy_setopt(context->curl, CURLOPT_USERNAME,
                context->username);
    }
    if (!curl_err && context->password) {
        curl_err = curl_easy_setopt(context->curl, CURLOPT_PASSWORD,
                context->password);
    }

    /* Set timeout */
    if (!curl_err) {
        curl_err = curl_easy_setopt(context->curl, CURLOPT_NOSIGNAL, 1);
    }
    if (!curl_err && context->timeout) {
        curl_err = curl_easy_setopt(context->curl, CURLOPT_TIMEOUT_MS,
                context->timeout);
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
    if (!curl_err) {
        curl_err = curl_easy_setopt(context->curl, CURLOPT_FOLLOWLOCATION, 1);
    }
    if (!curl_err) {
        /* TODO: Make the redirect depth configurable */
        curl_err = curl_easy_setopt(context->curl, CURLOPT_MAXREDIRS, 8);
    }
    if (!curl_err) {
        curl_err = curl_easy_setopt(context->curl,
                CURLOPT_UNRESTRICTED_AUTH, 1);
    }
    if (!curl_err) {
        curl_err = curl_easy_setopt(context->curl, CURLOPT_COOKIEFILE, "");
    }

    /* Set get-response function */
    if (!curl_err) {
        curl_err = curl_easy_setopt(context->curl, CURLOPT_WRITEFUNCTION,
                write_body);
    }
    if (!curl_err) {
        curl_err = curl_easy_setopt(context->curl, CURLOPT_WRITEDATA, &body);
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

    isds_log(ILF_HTTP, ILL_DEBUG, _("Sending POST request to %s\n"), url);
    isds_log(ILF_HTTP, ILL_DEBUG,
            _("POST body length: %zu, content follows:\n"), request_length);
    isds_log(ILF_HTTP, ILL_DEBUG, "%.*s\n", request_length, request);
    isds_log(ILF_HTTP, ILL_DEBUG, _("End of POST body\n"));
    if ((log_facilities & ILF_HTTP) && (log_level >= ILL_DEBUG) ) {
        curl_easy_setopt(context->curl, CURLOPT_VERBOSE, 1);
    }
    

    /*  Do the request */
    curl_err = curl_easy_perform(context->curl);

    /* Wipe credentials out of the handler */
    if (context->username) {
        curl_easy_setopt(context->curl, CURLOPT_USERNAME, NULL);
    }
    if (context->password) {
        curl_easy_setopt(context->curl, CURLOPT_PASSWORD, NULL);
    }

    if (!curl_err)
        curl_err = curl_easy_getinfo(context->curl, CURLINFO_CONTENT_TYPE,
            &content_type);

    if (curl_err) {
        /* TODO: CURL is not internationalized yet. Collect CURL messages for
         * I18N. */
        isds_printf_message(context,
                _("%s: %s"), url, _(curl_easy_strerror(curl_err)));
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

        close_connection(context);
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
 * @raw_response is automatically allocated bitstream with response body. Use
 * NULL if you don't care
 * @raw_response_length is size of @raw_response in bytes
 * In case of error the response will be deallocated automatically.
 * Side effect: message buffer */
_hidden isds_error soap(struct isds_ctx *context, const char *file,
        const xmlNodePtr request, xmlNodePtr *response,
        void **raw_response, size_t *raw_response_length) {

    isds_error err = IE_SUCCESS;
    char *url = NULL;
    char *mime_type = NULL;
    long http_code = 0;
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

    url = astrcat(context->url, file);
    if (!url) return IE_NOMEM;

    /* Build SOAP request envelope */
    request_soap_doc = xmlNewDoc(BAD_CAST "1.0");
    if (!request_soap_doc) {
        isds_log_message(context, _("Could not build SOAP request document"));
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
     * digital signatures probably because ISDS abandoned XMLDSig */
    save_ctx = xmlSaveToBuffer(http_request, "UTF-8", 1);
    if (!save_ctx) {
        isds_log_message(context,
                _("Could not create XML serializer"));
        err = IE_ERROR;
        goto leave;
    }
    /* XXX: According LibXML documentation, this function does not return
     * meaningfull value yet */
    xmlSaveDoc(save_ctx, request_soap_doc);
    if (-1 == xmlSaveFlush(save_ctx)) {
        isds_log_message(context,
                _("Could not serialize SOAP request to HTTP request bddy"));
        err = IE_ERROR;
        goto leave;
    }
    
    isds_log(ILF_SOAP, ILL_DEBUG,
            _("SOAP request to sent to %s:\n%.*s\nEnd of SOAP request\n"),
            url, http_request->use, http_request->content);

    err = http(context, url, http_request->content, http_request->use,
            &http_response, &response_length,
            &mime_type, NULL, &http_code);

    /* TODO: HTTP binding for SOAP prescribes non-200 HTTP return codes
     * to be processes too. */

    if (err) {
        goto leave;
    }

    /* Check for HTTP return code */
    isds_log(ILF_SOAP, ILL_DEBUG, _("Server returned %ld HTTP code\n"),
            http_code);
    switch (http_code) {
        /* XXX: We must see which code is used for not permitted ISDS
         * operation like downloading message without proper user
         * permissions. In that cat we should keep connection opened. */
        case 401:
            err = IE_NOT_LOGGED_IN;
            isds_log_message(context, _("Authentication failed"));
            goto leave;
            break;
        case 404:
            err = IE_HTTP;
            isds_log_message(context,
                    _("Code 404: Document not found on server"));
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
        char *mime_type_locale = utf82locale(mime_type);
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

    if (register_namespaces(xpath_ctx, MESSAGE_NS_UNSIGNED)) {
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
        /* TODO: log the faultcode and faultstring */ 
        isds_log_message(context, _("SOAP response signals Fault"));
        err = IE_SOAP;
        goto leave;
    }


    /* Extract XML Tree with ISDS response from SOAP envelope and return it.
     * XXX: response_soap_body is Body, we need children which may not exist
     * (i.e. empty Body). */
    /* TODO: Destroy SOAP response but Body childern. This is more memory
     * friendly than copying (potentialy) fat body */
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
    free(mime_type);
    free(http_response);
    xmlSaveClose(save_ctx);
    xmlBufferFree(http_request);
    xmlFreeDoc(request_soap_doc); /* recursive, frees request_body, soap_ns*/
    free(url);

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
