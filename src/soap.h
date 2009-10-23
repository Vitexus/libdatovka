#ifndef __ISDS_SOAP_H__
#define __ISDS_SOAP_H__

#include "isds.h"

/* Do SOAP request.
 * @context holds the base URL,
 * @file is a (CGI) file of SOAP URL,
 * @request is XML document with request. 
 * @request_length is lenght of @request in bytes
 * @file and @request must be NULL rather than empty strings, if the should
 * not be signaled in the SOAP request.
 * @reponse is automatically allocated() node set with SOAP response body.
 * You must xmlFreeNodeList() it. This is literal body, empty (NULL), one node
 * or more nodes can be returned.
 * In case of error the response will be deallocated automatically.
 * Side effect: message buffer */
isds_error soap(struct isds_ctx *context, const char *file,
        const void *request, const size_t request_length,
        xmlNodePtr *response);

#endif
