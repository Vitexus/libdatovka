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
 * @refnumber is automatically reallocated request serial number assigned by
 * ISDS. Returned *NULL means no number was delivered by server.
 * Use NULL if you don't care. */
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
        case SERVICE_DB_ACCESS:
        case SERVICE_DB_MANIPULATION:
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
    if (_isds_register_namespaces(xpath_ctx, 
                (context->type == CTX_TYPE_TESTING_REQUEST_COLLECTOR) ?
                    MESSAGE_NS_1 : MESSAGE_NS_UNSIGNED)) {
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
        isds_log_message(context,
                (context->type == CTX_TYPE_TESTING_REQUEST_COLLECTOR) ?
                    _("ISDS1 response is missing StatusCode element") :
                    _("ISDS response is missing StatusCode element"));
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
        /* Get reference number of client request */
        zfree(*refnumber);
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
 * */
_hidden isds_error isds(struct isds_ctx *context, const isds_service service,
        const xmlNodePtr request, xmlDocPtr *response,
        void **raw_response, size_t *raw_response_length) {
    isds_error err = IE_SUCCESS;
    xmlNodePtr response_body = NULL, isds_node;
    char *file = NULL;

    if (!context) return IE_INVALID_CONTEXT;
    if (!response) return IE_INVAL;
    if (!raw_response_length && raw_response) return IE_INVAL;

    /* Effective ISDS URL is build from base URL and suffix.
     * Other connection types has specific stable URL. */
    if (context->type == CTX_TYPE_ISDS) {
        switch (service) {
            case SERVICE_DM_OPERATIONS:     file = "DS/dz"; break;
            case SERVICE_DM_INFO:           file = "DS/dx"; break;
            case SERVICE_DB_SEARCH:         file = "DS/df"; break;
            case SERVICE_DB_ACCESS:         file = "DS/DsManage"; break;
            case SERVICE_DB_MANIPULATION:   file = "DS/DsManage"; break;
            default: return (IE_INVAL);
        }
    }

    err = _isds_soap(context, file, request, &response_body,
            raw_response, raw_response_length);

    if (err) goto leave;

    if (!response_body) {
        isds_log_message(context, _("SOAP returned empty body"));
        err = IE_ISDS;
    }

    /* Find ISDS element */
    for (isds_node = response_body; isds_node; isds_node = isds_node->next) {
        if (isds_node->type == XML_ELEMENT_NODE &&
                isds_node->ns &&
                !xmlStrcmp(isds_node->ns->href,
                    (context->type == CTX_TYPE_TESTING_REQUEST_COLLECTOR) ?
                        BAD_CAST ISDS1_NS : BAD_CAST ISDS_NS))
            break;
    }
    if (!isds_node) {
        isds_log_message(context,
                (context->type == CTX_TYPE_TESTING_REQUEST_COLLECTOR) ?
                    _("SOAP response does not contain ISDS1 element") :
                    _("SOAP response does not contain ISDS element"));
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
        isds_log_message(context,
                (context->type == CTX_TYPE_TESTING_REQUEST_COLLECTOR) ?
                    _("Could not build ISDS1 response document") :
                    _("Could not build ISDS response document"));
        err = IE_ERROR;
        goto leave;
    }
    xmlDocSetRootElement(*response, isds_node);

leave:
    if (err) {
        xmlFreeDoc(*response);
        if (raw_response) zfree(*raw_response);
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
_hidden isds_error _isds_check_documents_hierarchy(struct isds_ctx *context,
        const struct isds_list *documents) {

    const struct isds_list *item;
    const struct isds_document *document;
    _Bool main_exists = 0;

    if (!context) return IE_INVALID_CONTEXT;
    if (!documents) return IE_INVAL;

    for (item = documents; item; item = item->next) {
        document = (const struct isds_document *) item->data;
        if (!document) continue;

        /* Only one document can be main */
        if (document->dmFileMetaType == FILEMETATYPE_MAIN) {
            if (main_exists) {
                isds_log_message(context,
                        _("List contains more main documents"));
                return IE_ERROR;
            }
            main_exists = 1;
        }
        
        /* All document identifiers should be unique */
        if (document->dmFileGuid) {
            if (isds_find_document_by_id(documents, document->dmFileGuid) !=
                    document) {
                isds_printf_message(context, _("List contains more documents "
                            "with the same ID `%s'"), document->dmFileGuid);
                return IE_ERROR;
            }
        }

        /* All document references should point to existing document ID */
        /* ???: Should we forbid self-referencing? */
        if (document->dmUpFileGuid) {
            if (!isds_find_document_by_id(documents,
                        document->dmUpFileGuid)) {
                isds_printf_message(context, _("List contains documents "
                            "referencing to not existing document ID `%s'"),
                        document->dmUpFileGuid);
                return IE_ERROR;
            }
        }
    }
    
    if (!main_exists) {
        isds_log_message(context, _("List does not contain main document"));
        return IE_ERROR;
    }

    return IE_SUCCESS;
}


/* Check for message ID length
 * @context is session context
 * @message_id checked message ID
 * @return IE_SUCCESS or appropriate error code and fill context' message */
isds_error validate_message_id_length(struct isds_ctx *context,
        const xmlChar *message_id) {
    if (!context) return IE_INVALID_CONTEXT;
    if (!message_id) return IE_INVAL;

    const int length = xmlUTF8Strlen(message_id);

    if (length == -1) {
        char *message_id_locale = _isds_utf82locale((char*) message_id);
        isds_printf_message(context,
                _("Could not check message ID length: %s"),
                message_id_locale);
        free(message_id_locale);
        return IE_ERROR;
    }

    if (length >= 20) {
        char *message_id_locale = _isds_utf82locale((char*) message_id);
        isds_printf_message(context,
                _("Message ID must not be longer than 20 characters: %s"),
                message_id_locale);
        free(message_id_locale);
        return IE_INVAL;
    }

    return IE_SUCCESS;
}


/* Send @request to Czech POINT conversion deposit and return response
 * as XML document.
 * @context is Czech POINT session context,
 * @request is tree with deposit message, can be NULL
 * @response is automatically allocated response from server as XML Document
 * In case of error, @response will be deallocated.
 * */
_hidden isds_error _czp_czpdeposit(struct isds_ctx *context,
        const xmlNodePtr request, xmlDocPtr *response) {
    isds_error err = IE_SUCCESS;
    xmlNodePtr response_body = NULL, deposit_node;

    if (!context) return IE_INVALID_CONTEXT;
    if (!response) return IE_INVAL;

    err = _isds_soap(context, NULL, request, &response_body, NULL, NULL);

    if (err) goto leave;

    if (!response_body) {
        isds_log_message(context, _("SOAP returned empty body"));
        err = IE_ISDS;
    }

    /* Find deposit element */
    for (deposit_node = response_body; deposit_node;
            deposit_node = deposit_node->next) {
        if (deposit_node->type == XML_ELEMENT_NODE &&
                deposit_node->ns &&
                !xmlStrcmp(deposit_node->ns->href, BAD_CAST DEPOSIT_NS))
            break;
    }
    if (!deposit_node) {
        isds_log_message(context,
                _("SOAP response does not contain "
                    "Czech POINT deposit element"));
        err = IE_ISDS;
        goto leave;
    }

    /* Destroy other nodes */
    if (deposit_node == response_body)
        response_body = response_body->next;
    xmlUnlinkNode(deposit_node);
    xmlFreeNodeList(response_body);
    response_body = NULL;

    /* Build XML document */
    *response = xmlNewDoc(BAD_CAST "1.0");
    if (!*response) {
        isds_log_message(context,
                _("Could not build Czech POINT deposit response document"));
        err = IE_ERROR;
        goto leave;
    }
    xmlDocSetRootElement(*response, deposit_node);

leave:
    if (err) {
        xmlFreeDoc(*response);
    }
    xmlFreeNodeList(response_body);

    return err;
}
