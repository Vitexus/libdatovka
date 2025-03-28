#ifndef __ISDS_VALIDATOR_H__
#define __ISDS_VALIDATOR_H__

#include "libdatovka/isds.h"

struct comm_req; /* Forward declaration. */
struct dbuf_res; /* Forward declaration. */
struct multipart_parts; /* Forward declaration. */

#if HAVE_LIBCURL
typedef enum isds_service {
	SERVICE_DM_OPERATIONS,
	SERVICE_DM_INFO,
	SERVICE_VODZ_DM_OPERATIONS,
	SERVICE_DB_SEARCH,
	SERVICE_DB_ACCESS,
	SERVICE_DB_MANIPULATION,
	SERVICE_ASWS
} isds_service;

/* Flags to be used when communicating with VODZ services. */
enum vodz_flags {
	VODZ_BASIC = 0x00, /* Send plain XML, receive plain XML. */
	VODZ_SND_XOP = 0x01, /* Send MTOM/XOP data. */
	VODZ_RCV_XOP = 0x02 /* Receive XTOM/XOP data. */
};

/*
 * Convert enum isds_service to enum isds_status_type.
 * @service is ISDS web service identifier
 */
enum isds_status_type _isds_service_to_status_type(enum isds_service service);

/*
 * Get ISDS status info from ISDS @response XML document.
 * Be ware that different request families return differently encoded status
 * (e.g. dmStatus, dbStatus)
 * @context is ISDS context
 * @service is ISDS web service identifier
 * @response is ISDS response document
 * @code is automatically allocated status code of the response
 * @message is automatically allocated status message. Returned NULL means no
 * message was delivered by server. Use NULL if you don't care.
 * @refnumber is automatically reallocated request serial number assigned by
 * ISDS. Returned *NULL means no number was delivered by server.
 * Use NULL if you don't care.
 */
enum isds_error isds_response_status(struct isds_ctx *context,
    const enum isds_service service, xmlDocPtr response,
    xmlChar **code, xmlChar **message, xmlChar **refnumber);

/*
 * Send @request to ISDS and return ISDS @response as XML document.
 * Be ware the @response can be invalid (in sense of XML Schema).
 * (And it is because current ISDS server does not follow its own
 * specification. Please apology my government, its herd of incompetent
 * creatures.)
 * @context is ISDS session context,
 * @service identifies ISDS web service
 * @request is tree with ISDS message, can be NULL
 * @response is automatically allocated response from server as XML Document
 * @raw_response is automatically allocated bit stream with response body. Use
 * NULL if you don't care
 * @raw_response_length is size of @raw_response in bytes
 * In case of error, @response and @raw_response will be deallocated.
 */
enum isds_error _isds(struct isds_ctx *context, const enum isds_service service,
    const xmlNodePtr request, xmlDocPtr *response,
    void **raw_response, size_t *raw_response_length);

/*
 * Send @request to ISDS and return ISDS @response as MTOM/XOP document via
 * the high-volume data message interface.
 * Be ware the @response can be invalid (in sense of XML Schema).
 * (And it is because current ISDS server does not follow its own
 * specification. Please apology my government, its herd of incompetent
 * creatures.)
 * @context is ISDS session context,
 * @service identifies ISDS web service
 * @v_flags flags specifying type of communication, use enum vodz_flags
 * @req contains XML request and optionally additional binary data to be added
 * @response is automatically allocated response from server as XML Document
 * @raw_response is automatically allocated bit stream with response body. Use
 * NULL if you don't care
 * @parts is automatically allocated container for multipart data.
 * @raw_response_length is size of @raw_response in bytes
 * In case of error, @response and @raw_response will be deallocated.
 */
enum isds_error _isds_vodz(struct isds_ctx *context,
    const enum isds_service service, int v_flags, const struct comm_req *req,
    xmlDoc **response, struct dbuf_res *raw_response, struct multipart_parts **parts);
#endif /* HAVE_LIBCURL */

/* Walk through list of isds_documents and check for their types and
 * references.
 * @context is session context
 * @documents is list of isds_document to check
 * @ext_files is list of isds_dmExtFile to check
 * @returns IE_SUCCESS if structure is valid, otherwise context' message will
 * be filled with explanation of found problem. */
enum isds_error _isds_check_documents_hierarchy(struct isds_ctx *context,
    const struct isds_list *documents, const struct isds_list *ext_files);

/* Check for message ID length
 * @context is session context
 * @message_id checked message ID
 * @return IE_SUCCESS or appropriate error code and fill context' message */
isds_error validate_message_id_length(struct isds_ctx *context,
        const xmlChar *message_id);

#if HAVE_LIBCURL
/* Send @request to Czech POINT conversion deposit and return response
 * as XML document.
 * @context is Czech POINT session context,
 * @request is tree with deposit message, can be NULL
 * @response is automatically allocated response from server as XML Document
 * In case of error, @response will be deallocated.
 * */
isds_error _czp_czpdeposit(struct isds_ctx *context,
        const xmlNodePtr request, xmlDocPtr *response);
#endif /* HAVE_LIBCURL */

#endif
