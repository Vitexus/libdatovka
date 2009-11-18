#include "isds_priv.h"
#include "utils.h"
#include "validator.h"
#include "soap.h"


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
_hidden isds_error isds_response_status(struct isds_ctx *context,
        const isds_service service, xmlDocPtr response,
        xmlChar **code, xmlChar **message, xmlChar **refnumber) {
    isds_error err = IE_SUCCESS;
    xmlChar *status_code_expr = NULL, *status_message_expr = NULL;
    xmlXPathContextPtr xpath_ctx = NULL;
    xmlXPathObjectPtr result = NULL;

    if (!response || !code) {
        err = IE_INVAL;
        goto leave;
    }

    switch (service) {
        case SERVICE_DM_OPERATIONS:
        case SERVICE_DM_INFO:
            status_code_expr = BAD_CAST
                "/*/isds:dmStatus/isds:dmStatusCode/text()";
            status_message_expr = BAD_CAST
                "/*/isds:dmStatus/isds:dmStatusMessage/text()";
            break;
        case SERVICE_DB_SEARCH:
        case SERVICE_DB_SUPPLEMENTARY:
            status_code_expr = BAD_CAST
                "/*/isds:dbStatus/isds:dbStatusCode/text()";
            status_message_expr = BAD_CAST
                "/*/isds:dbStatus/isds:dbStatusMessage/text()";
            break;
        default:
            err = IE_NOTSUP;
            goto leave;
    }

    xpath_ctx = xmlXPathNewContext(response);
    if (!xpath_ctx) {
        err = IE_ERROR;
        goto leave;
    }
    if (register_namespaces(xpath_ctx)) {
        err = IE_ERROR;
        goto leave;
    }

    /* Get status code */
    result = xmlXPathEvalExpression(status_code_expr, xpath_ctx);
    if (!result) {
        err = IE_ERROR;
        goto leave;
    }
    if (xmlXPathNodeSetIsEmpty(result->nodesetval)) {
        isds_log_message(context, _("ISDS response is missing StatusCode"));
        err = IE_ISDS;
        goto leave;
    }
    *code = xmlXPathCastNodeSetToString(result->nodesetval);
    if (!code) {
        err = IE_ERROR;
        goto leave;
    }

    if (message) {
        /* Get status message */
        xmlXPathFreeObject(result);
        result = xmlXPathEvalExpression(status_message_expr, xpath_ctx);
        if (!result) {
            err = IE_ERROR;
            goto leave;
        }
        if (xmlXPathNodeSetIsEmpty(result->nodesetval)) {
            /* E.g. CreateMessageResponse with dmStatusCode 9005 has empty
             * message */
            *message = NULL;
        } else {
            *message = xmlXPathCastNodeSetToString(result->nodesetval);
            if (!message) {
                err = IE_ERROR;
                goto leave;
            }
        }
    }

    if (refnumber) {
        /* Get status reference number */
        xmlXPathFreeObject(result);
        result = xmlXPathEvalExpression(
                BAD_CAST "/*/isds:dbStatus/isds:dbStatusRefNumber/text()",
                xpath_ctx);
        if (!result) {
            err = IE_ERROR;
            goto leave;
        }
        if (xmlXPathNodeSetIsEmpty(result->nodesetval)) {
            *refnumber = NULL;
        } else {
            *refnumber = xmlXPathCastNodeSetToString(result->nodesetval);
            if (!message) {
                err = IE_ERROR;
                goto leave;
            }
        }
    }
leave:
    xmlXPathFreeObject(result);
    xmlXPathFreeContext(xpath_ctx);
    return err;
}


/* Send @request to ISDS and return ISDS @response as XML document.
 * Be ware the @response can be invalid (in sense of XML Schema).
 * (And it is because current ISDS server does not follow its own
 * specification. Please appology my government, its herd of imcompetent
 * creatures.)
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
        case SERVICE_DM_OPERATIONS:     file = "dz"; break;
        case SERVICE_DM_INFO:           file = "dx"; break;
        case SERVICE_DB_SEARCH:         file = "df"; break;
        case SERVICE_DB_SUPPLEMENTARY:  file = "DsManage"; break;
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

/* Walk through list of isds_documents and check for their types and
 * references.
 * @context is session context
 * @documents is list of isds_document to check
 * @returns IE_SUCCESS if structure is valid, otherwise context' message will
 * be filled with explanation of found problem. */
isds_error check_documents_hiararchy(struct isds_ctx *context,
        const struct isds_list *documents) {

    const struct isds_list *item;
    const struct isds_document *document;
    _Bool main_exists = 0;

    if (!context) return IE_INVALID_CONTEXT;
    if (!documents) return IE_INVAL;

    for (item = documents; item; item = item->next) {
        document = (const struct isds_document *) item->data;
        if (!document) continue;

        if (document->dmFileMetaType == FILEMETATYPE_MAIN) {
            if (main_exists) {
                isds_log_message(context,
                        _("List contains more main documents"));
                return IE_ERROR;
            }
            main_exists = 1;
        }
        
    }
    
    if (!main_exists) {
        isds_log_message(context, _("List does not contain main document"));
        return IE_ERROR;
    }

    return IE_SUCCESS;
}

