#ifndef __ISDS_SOAP_H__
#define __ISDS_SOAP_H__

#include "isds.h"

/* Do SOAP request.
 * @file is a (CGI) file of SOAP URL,
 * @context holds the base URL,
 * @request is XML document with request
 * @request_length is lenght of @request in bytes
 * @file and @request must be NULL rather than empty strings, if the should
 * not be signaled in the SOAP request.
 * @reponse is automatically reallocated() buffer to fit SOAP response with
 * @response_length (does not need to match allocatef memory exactly. You must
 * free() the @response.  Side effect: message buffer */
isds_error soap(struct isds_ctx *context, const char *file,
        const void *request, const size_t request_length,
        void **response, size_t *response_length);

#endif
