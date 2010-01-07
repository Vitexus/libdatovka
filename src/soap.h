#ifndef __ISDS_SOAP_H__
#define __ISDS_SOAP_H__

#include "isds.h"

/* Close connection to server and destroy CURL handle associated
 * with @context */
isds_error close_connection(struct isds_ctx *context);

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
isds_error soap(struct isds_ctx *context, const char *file,
        const xmlNodePtr request, xmlNodePtr *response,
        void **raw_response, size_t *raw_response_length);

#endif
