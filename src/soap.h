#ifndef __ISDS_SOAP_H__
#define __ISDS_SOAP_H__

#include "libdatovka/isds.h"

struct comm_req; /* Forward declaration. */
struct dbuf; /* Forward declaration. */
struct multipart_parts; /* Forward declaration. */

/* Flags to be used when working with SOAP. */
enum soap_communication_flags {
	SCF_BASIC = 0x00, /* Send plain XML, receive plain XML. */
	SCF_SND_XOP = 0x01, /* Send MTOM/XOP data. */
	SCF_RCV_XOP = 0x02 /* Receive XTOM/XOP data. */
};

/* Close connection to server and destroy CURL handle associated
 * with @context */
isds_error _isds_close_connection(struct isds_ctx *context);

/* Do SOAP request.
 * @context holds the base URL,
 * @file is a (CGI) file of SOAP URL,
 * @request is XML node set with SOAP request body.
 * @file must be NULL, @request should be NULL rather than empty, if they should
 * not be signalled in the SOAP request.
 * @response_document is an automatically allocated XML document whose subtree
 * identified by @response_node_list holds the SOAP response body content. You
 * must xmlFreeDoc() it. If you don't care pass NULL and also
 * NULL @response_node_list.
 * @response_node_list is a pointer to node set with SOAP response body
 * content. The returned pointer points into @response_document to the first
 * child of SOAP Body element. Pass NULL and NULL @response_document, if you
 * don't care.
 * @raw_response is automatically allocated bit stream with response body. Use
 * NULL if you don't care
 * @raw_response_length is size of @raw_response in bytes
 * In case of error the response will be deallocated automatically.
 * Side effect: message buffer */
isds_error _isds_soap(struct isds_ctx *context, const char *file,
        const xmlNodePtr request,
        xmlDocPtr *response_document, xmlNodePtr *response_node_list,
        void **raw_response, size_t *raw_response_length);

/*
 * Do SOAP request via the high-volume data message interface.
 * @context holds the base URL,
 * @file is a (CGI) file of SOAP URL,
 * @request is XML node set with SOAP request body.
 * @file must be NULL, @request should be NULL rather than empty, if they should
 * not be signalled in the SOAP request.
 * @s_flags are flags indcating how to communicate with the ISDS server,
 * use bit-masked values of enum soap_communication_flags.
 * @req contains XML request and optionally additional binary data to be added
 * @response_document is an automatically allocated XML document whose subtree
 * identified by @response_node_list holds the SOAP response body content. You
 * must xmlFreeDoc() it. If you don't care pass NULL and also
 * NULL @response_node_list.
 * @response_node_list is a pointer to node set with SOAP response body
 * content. The returned pointer points into @response_document to the first
 * child of SOAP Body element. Pass NULL and NULL @response_document, if you
 * don't care.
 * @raw_response is automatically allocated bit stream with response body. Use
 * NULL if you don't care. Raw response is received when SCF_RCV_XOP isn't
 * being used in @s_flags or when server decides not to use MTOM/XOP in response.
 * @parts is automatically reallocated container containing parts of
 * a multipart response. Multipart response is expected when using SCF_RCV_XOP
 * in @s_flags.
 * In case of error the response will be deallocated automatically.
 * Side effect: message buffer
 */
enum isds_error _isds_soap_vodz(struct isds_ctx *context, const char *file,
    int s_flags, const struct comm_req *req,
    xmlDoc **response_document, xmlNode **response_node_list,
    struct dbuf *raw_response, struct multipart_parts **parts);

/* Build new URL from current @context and template.
 * @context is context carrying an URL
 * @template is printf(3) format string. First argument is length of the base
 * URL found in @context, second argument is the base URL, third argument is
 * again the base URL.
 * XXX: We cannot use "$" formatting character because it's not in the ISO C99.
 * @new_url is newly allocated URL built from @template. Caller must free it.
 * Return IE_SUCCESS, or corresponding error code and @new_url will not be
 * allocated.
 * */
isds_error _isds_build_url_from_context(struct isds_ctx *context,
        const char *template, char **new_url);

/* Invalidate session cookie for OTP authenticated @context */
isds_error _isds_invalidate_otp_cookie(struct isds_ctx *context);

#endif
