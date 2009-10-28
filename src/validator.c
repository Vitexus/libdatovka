#include "isds_priv.h"
#include "utils.h"
#include "validator.h"
#include "soap.h"


/* Get ISDS dmstatus info from ISDS @response xml node set.
 * Be ware that different request familes return differently encoded status
 * (e.g. dmStatus, dbStatus)
 * @response is SOAP body response 
 * @code is status code of the response
 * @message is automatically allocated status message*/
_hidden isds_error isds_response_dmstatus(xmlNodePtr response,
        unsigned int *code, xmlChar **message) {
    isds_error err = IE_SUCCESS;

    if (!response) {
        err = IE_INVAL;
        goto leave;
    }


leave:

    /*return err;*/
    return IE_NOTSUP;
}


/* Send @request to ISDS and return ISDS @response as XML document.
 * The returned @response is guaranted to be valid ISDS message as defined in
 * ISDS XML Schemata.
 * @context is ISDS session context,
 * @service identifies ISDS web service
 * @request is tree with ISDS message, can be NULL
 * @response is automatically allocated response from server as XML Document
 * In case of error, @response will be dealocated.
 * */
_hidden isds_error isds(struct isds_ctx *context, const isds_service service,
        const xmlNodePtr request, xmlDocPtr *response) {
    isds_error err = IE_SUCCESS;
    xmlNodePtr response_body = NULL, isds_node;
    char *file = NULL;

    if (!context) return IE_INVALID_CONTEXT;
    if (!response) return IE_INVAL;

    switch (service) {
        case SERVICE_DM_OPERATIONS: file = "dz"; break;
        case SERVICE_DM_INFO:       file = "dx"; break;
        case SERVICE_DB_SEARCH:     file = "df"; break;
        default: return (IE_INVAL);
    }

    err = soap(context, file, request, &response_body);

    if (err) goto leave;

    if (!response_body) {
        isds_log_message(context, _("SOAP returned empty body"));
        err = IE_ISDS;
    }

    /* Find ISDS element */
    for (isds_node = response_body; isds_node; isds_node = isds_node->next) {
        if (isds_node->type == XML_ELEMENT_NODE &&
                isds_node->ns &&
                !xmlStrcmp(isds_node->ns->href, BAD_CAST ISDS_NS))
            break;
    }
    if (!isds_node) {
        isds_log_message(context,
                _("SOAP response does not contain ISDS elemement"));
        err = IE_ISDS;
        goto leave;
    }

    /* Destroy other nodes */
    if (isds_node == response_body)
        response_body = response_body->next;
    xmlUnlinkNode(isds_node);
    xmlFreeNodeList(response_body);
    response_body = NULL;

    /* TODO: validate the response */

    /* Build XML document */
    *response = xmlNewDoc(BAD_CAST "1.0");
    if (!*response) {
        isds_log_message(context, _("Could not build ISDS response document"));
        err = IE_ERROR;
        goto leave;
    }
    xmlDocSetRootElement(*response, isds_node);

leave:
    if (err) {
        xmlFreeDoc(*response);
    }
    xmlFreeNodeList(response_body);

    return err;
}

