#ifndef __ISDS_VALIDATOR_H__
#define __ISDS_VALIDATOR_H__

#include "isds.h"

typedef enum {
    SERVICE_DM_OPERATIONS,
    SERVICE_DM_INFO,
    SERVICE_DB_SEARCH
} isds_service;

/* Get ISDS dmstatus info from ISDS @response xml node set.
 * Be ware that different request familes return differently encoded status
 * (e.g. dmStatus, dbStatus)
 * @response is SOAP body response 
 * @code is status code of the response
 * @message is automatically allocated status message*/
isds_error isds_response_dmstatus(xmlNodePtr response,
        unsigned int *code, xmlChar **message);

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

#endif
