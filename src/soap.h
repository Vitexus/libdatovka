#ifndef __ISDS_SOAP_H__
#define __ISDS_SOAP_H__

#include "isds.h"

/* Do SOAP request.
 * @file is a (CGI) file of SOAP URL,
 * @context holds the base URL,
 * @request is XML document with request (NULL terminated). 
 * @file and @request must be NULL rather than empty strings, if the should
 * not be signaled in the SOAP request.
 * @reponse is automatically reallocated() buffer to fit SOAP response with
 * @length (does not need to match allocates memory exactly. You must free() the
 * @response.
 * Side effect: message buffer */
isds_error soap(struct isds_ctx *context, const char *file,
        const char *request, void **response, size_t *length);

#endif
