#ifndef __ISDS_VALIDATOR_H__
#define __ISDS_VALIDATOR_H__

#include "isds.h"

typedef enum {
    SERVICE_DM_OPERATIONS,
    SERVICE_DM_INFO,
    SERVICE_DB_SEARCH,
    SERVICE_DB_SUPPLEMENTARY
} isds_service;

/* Get ISDS status info from ISDS @response XML document.
 * Be ware that different request families return differently encoded status
 * (e.g. dmStatus, dbStatus)
 * @context is ISDS context
 * @service is ISDS web service identifier
 * @response is ISDS response document
 * @code is automatically allocated status code of the response
 * @message is automatically allocated status message. Returned NULL means no
 * message was delivered by server. Use NULL if you don't care.
 * @refnumber is automatically allocated status reference number. Returned
 * NULL means no referce was delivered by server. Use NULL if you don't care.
 * */
isds_error isds_response_status(struct isds_ctx *context,
        const isds_service service, xmlDocPtr response,
        xmlChar **code, xmlChar **message, xmlChar **refnumber);

/* Send @request to ISDS and return ISDS @response as XML document.
 * The returned @response is guaranted to be valid ISDS message as defined in
 * ISDS XML Schemata.
 * @context is ISDS session context,
 * @service identifies ISDS web service
 * @request is tree with ISDS message, can be NULL
 * @response is automatically allocated response from server as XML Document
 * In case of error, @response will be dealocated.
 * */
isds_error isds(struct isds_ctx *context, const isds_service service,
        const xmlNodePtr request, xmlDocPtr *response);

/* Walk through list of isds_documents and check for their types and
 * references.
 * @context is session context
 * @documents is list of isds_document to check
 * @returns IE_SUCCESS if structure is valid, otherwise context' message will
 * be filled with explanation of found problem. */
isds_error check_documents_hierarchy(struct isds_ctx *context,
        const struct isds_list *documents);

#endif
