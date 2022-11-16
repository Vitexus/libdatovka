#include "isds_priv.h"
#include "utils.h"
#include "validator.h"
#if HAVE_LIBCURL
    #include "soap.h"
#endif


#if HAVE_LIBCURL
_hidden enum isds_status_type _isds_service_to_status_type(
    enum isds_service service)
{
	enum isds_status_type type = STAT_DB;

	switch (service) {
	case SERVICE_DM_OPERATIONS:
	case SERVICE_DM_INFO:
	case SERVICE_VODZ_DM_OPERATIONS:
		type = STAT_DM;
		break;
	case SERVICE_DB_SEARCH:
	case SERVICE_DB_ACCESS:
	case SERVICE_DB_MANIPULATION:
		type = STAT_DB;
		break;
	case SERVICE_ASWS:
		type = STAT_DB;
		break;
	}
	return type;
}

_hidden enum isds_error isds_response_status(struct isds_ctx *context,
    const enum isds_service service, xmlDocPtr response,
    xmlChar **code, xmlChar **message, xmlChar **refnumber)
{
	enum isds_error err = IE_SUCCESS;
	xmlChar *status_code_expr = NULL;
	xmlChar *status_message_expr = NULL;
	xmlXPathContextPtr xpath_ctx = NULL;
	xmlXPathObjectPtr result = NULL;

	if ((NULL == response) || (NULL == code)) {
		err = IE_INVAL;
		goto leave;
	}

	switch (service) {
	case SERVICE_DM_OPERATIONS:
	case SERVICE_DM_INFO:
	case SERVICE_VODZ_DM_OPERATIONS:
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
	case SERVICE_ASWS:
		status_code_expr = BAD_CAST
		    "/*/oisds:dbStatus/oisds:dbStatusCode/text()";
		status_message_expr = BAD_CAST
		    "/*/oisds:dbStatus/oisds:dbStatusMessage/text()";
		break;
	default:
		err = IE_NOTSUP;
		goto leave;
	}

	xpath_ctx = xmlXPathNewContext(response);
	if (NULL == xpath_ctx) {
		err = IE_ERROR;
		goto leave;
	}
	if (IE_SUCCESS != _isds_register_namespaces(xpath_ctx,
	        (context->type == CTX_TYPE_TESTING_REQUEST_COLLECTOR) ?
	            MESSAGE_NS_1 : MESSAGE_NS_UNSIGNED,
	        SOAP_1_1)) {
		err = IE_ERROR;
		goto leave;
	}

	/* Get status code. */
	result = xmlXPathEvalExpression(status_code_expr, xpath_ctx);
	if (NULL == result) {
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
	if (NULL == *code) {
		err = IE_ERROR;
		goto leave;
	}

	if (NULL != message) {
		/* Get status message. */
		xmlXPathFreeObject(result);
		result = xmlXPathEvalExpression(status_message_expr, xpath_ctx);
		if (NULL == result) {
			err = IE_ERROR;
			goto leave;
		}
		if (xmlXPathNodeSetIsEmpty(result->nodesetval)) {
			/*
			 * E.g. CreateMessageResponse with dmStatusCode 9005
			 * has empty message.
			 */
			*message = NULL;
		} else {
			*message = xmlXPathCastNodeSetToString(result->nodesetval);
			if (NULL == *message) {
				err = IE_ERROR;
				goto leave;
			}
		}
	}

	if (NULL != refnumber) {
		/* Get reference number of client request. */
		zfree(*refnumber);
		xmlXPathFreeObject(result);
		result = xmlXPathEvalExpression(
		    (SERVICE_ASWS == service) ?
		        BAD_CAST "/*/oisds:dbStatus/oisds:dbStatusRefNumber/text()":
		        BAD_CAST "/*/isds:dbStatus/isds:dbStatusRefNumber/text()",
		    xpath_ctx);
		if (NULL == result) {
			err = IE_ERROR;
			goto leave;
		}
		if (xmlXPathNodeSetIsEmpty(result->nodesetval)) {
			*refnumber = NULL;
		} else {
			*refnumber = xmlXPathCastNodeSetToString(result->nodesetval);
			if (NULL == *refnumber) {
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

_hidden enum isds_error _isds(struct isds_ctx *context,
    const enum isds_service service, const xmlNodePtr request,
    xmlDocPtr *response, void **raw_response, size_t *raw_response_length)
{
	enum isds_error err = IE_SUCCESS;
	xmlDoc *response_document = NULL;
	xmlNode *response_body;
	xmlNode *isds_node;
	char *file = NULL;
	const char *name_space = ISDS_NS;

	if (NULL == context) {
		return IE_INVALID_CONTEXT;
	}
	if (NULL == response) {
		return IE_INVAL;
	}
	if ((NULL == raw_response_length) && (NULL != raw_response)) {
		return IE_INVAL;
	}

	/*
	 * Effective ISDS URL is build from base URL and suffix.
	 * Other connection types has specific stable URL.
	 */
	if (context->type == CTX_TYPE_ISDS) {
		switch (service) {
		case SERVICE_DM_OPERATIONS:      file = "DS/dz"; break;
		case SERVICE_DM_INFO:            file = "DS/dx"; break;
		case SERVICE_DB_SEARCH:          file = "DS/df"; break;
		case SERVICE_DB_ACCESS:          file = "DS/DsManage"; break;
		case SERVICE_DB_MANIPULATION:    file = "DS/DsManage"; break;
		case SERVICE_ASWS:               file = ""; break;
		case SERVICE_VODZ_DM_OPERATIONS: /* VODZ not supported here. */
		default: return (IE_INVAL); break;
		}
	}

	/* Also name space differs in some cases. */
	if (CTX_TYPE_TESTING_REQUEST_COLLECTOR == context->type) {
		name_space = ISDS1_NS;
	} else if (SERVICE_ASWS == service) {
		name_space = OISDS_NS;
	}

	err = _isds_soap(context, file, request, &response_document,
	    &response_body, raw_response, raw_response_length);

	if (IE_SUCCESS != err) {
		goto leave;
	}

	if (NULL == response_body) {
		isds_log_message(context, _("SOAP returned empty body"));
		err = IE_ISDS;
	}

	/* Find ISDS element. */
	for (isds_node = response_body; isds_node; isds_node = isds_node->next) {
		if ((isds_node->type == XML_ELEMENT_NODE) &&
		    (NULL != isds_node->ns) &&
		    (0 == xmlStrcmp(isds_node->ns->href, BAD_CAST name_space))) {
			break;
		}
	}
	if (NULL == isds_node) {
		char *name_space_local = _isds_utf82locale(name_space);
		isds_printf_message(context,
		    _("SOAP response does not contain element from name space %s"),
		    name_space_local);
		free(name_space_local);
		err = IE_ISDS;
		goto leave;
	}

	/* TODO: validate the response */

	/* Build XML document */
	*response = xmlNewDoc(BAD_CAST "1.0");
	if (NULL == *response) {
		isds_log_message(context,
		    _("Could not build ISDS response document"));
		err = IE_ERROR;
		goto leave;
	}
	xmlDocSetRootElement(*response, isds_node);

leave:
	if (IE_SUCCESS != err) {
		xmlFreeDoc(*response);
		if (NULL != raw_response) {
			zfree(*raw_response);
		}
	}
	xmlFreeDoc(response_document);

	return err;
}

_hidden enum isds_error _isds_vodz(struct isds_ctx *context,
    const enum isds_service service, const xmlNodePtr request,
    xmlDoc **response, void **raw_response, size_t *raw_response_length)
{
	enum isds_error err = IE_SUCCESS;
	xmlDoc *response_document = NULL;
	xmlNode *response_body;
	xmlNode *isds_node;
	char *file = NULL;
	const char *name_space = ISDS_NS;

	if (NULL == context) {
		return IE_INVALID_CONTEXT;
	}
	if (NULL == response) {
		return IE_INVAL;
	}
	if ((NULL == raw_response_length) && (NULL != raw_response)) {
		return IE_INVAL;
	}

	/*
	 * Effective ISDS URL is build from base URL and suffix.
	 * Other connection types has specific stable URL.
	 */
	if (context->type == CTX_TYPE_ISDS) {
		switch (service) {
		case SERVICE_VODZ_DM_OPERATIONS: file = "DS/vodz"; break;
		/* Only VODZ supported. */
		default: return (IE_INVAL); break;
		}
	}

	/* Also name space differs in some cases. */
	if (CTX_TYPE_TESTING_REQUEST_COLLECTOR == context->type) {
		name_space = ISDS1_NS;
	} else if (SERVICE_ASWS == service) {
		name_space = OISDS_NS;
	}

	err = _isds_soap_vodz(context, file, request, NULL, NULL, &response_document,
	    &response_body, raw_response, raw_response_length);

	if (IE_SUCCESS != err) {
		goto leave;
	}

	if (NULL == response_body) {
		isds_log_message(context, _("SOAP returned empty body"));
		err = IE_ISDS;
	}

	/* Find ISDS element. */
	for (isds_node = response_body; isds_node; isds_node = isds_node->next) {
		if ((isds_node->type == XML_ELEMENT_NODE) &&
		    (NULL != isds_node->ns) &&
		    (0 == xmlStrcmp(isds_node->ns->href, BAD_CAST name_space))) {
			break;
		}
	}
	if (NULL == isds_node) {
		char *name_space_local = _isds_utf82locale(name_space);
		isds_printf_message(context,
		    _("SOAP response does not contain element from name space %s"),
		    name_space_local);
		free(name_space_local);
		err = IE_ISDS;
		goto leave;
	}

	/* TODO: validate the response */

	/* Build XML document */
	*response = xmlNewDoc(BAD_CAST "1.0");
	if (NULL == *response) {
		isds_log_message(context,
		    _("Could not build ISDS response document"));
		err = IE_ERROR;
		goto leave;
	}
	xmlDocSetRootElement(*response, isds_node);

leave:
	if (IE_SUCCESS != err) {
		xmlFreeDoc(*response);
		if (NULL != raw_response) {
			zfree(*raw_response);
		}
	}
	xmlFreeDoc(response_document);

	return err;
}

enum isds_error _isds_vodz_mtomxop(struct isds_ctx *context,
    const enum isds_service service, const xmlNodePtr request,
    const char *content_id, const struct isds_dmFile *dm_file,
    xmlDoc **response, void **raw_response, size_t *raw_response_length)
{
	enum isds_error err = IE_SUCCESS;
	xmlDoc *response_document = NULL;
	xmlNode *response_body;
	xmlNode *isds_node;
	char *file = NULL;
	const char *name_space = ISDS_NS;

	if (NULL == context) {
		return IE_INVALID_CONTEXT;
	}
	if (NULL == response) {
		return IE_INVAL;
	}
	if ((NULL == raw_response_length) && (NULL != raw_response)) {
		return IE_INVAL;
	}

	/*
	 * Effective ISDS URL is build from base URL and suffix.
	 * Other connection types has specific stable URL.
	 */
	if (context->type == CTX_TYPE_ISDS) {
		switch (service) {
		case SERVICE_VODZ_DM_OPERATIONS: file = "DS/vodz"; break;
		/* Only VODZ supported. */
		default: return (IE_INVAL); break;
		}
	}

	/* Also name space differs in some cases. */
	if (CTX_TYPE_TESTING_REQUEST_COLLECTOR == context->type) {
		name_space = ISDS1_NS;
	} else if (SERVICE_ASWS == service) {
		name_space = OISDS_NS;
	}

	err = _isds_soap_vodz(context, file, request, content_id, dm_file,
	    &response_document,
	    &response_body, raw_response, raw_response_length);

	if (IE_SUCCESS != err) {
		goto leave;
	}

	if (NULL == response_body) {
		isds_log_message(context, _("SOAP returned empty body"));
		err = IE_ISDS;
	}

	/* Find ISDS element. */
	for (isds_node = response_body; isds_node; isds_node = isds_node->next) {
		if ((isds_node->type == XML_ELEMENT_NODE) &&
		    (NULL != isds_node->ns) &&
		    (0 == xmlStrcmp(isds_node->ns->href, BAD_CAST name_space))) {
			break;
		}
	}
	if (NULL == isds_node) {
		char *name_space_local = _isds_utf82locale(name_space);
		isds_printf_message(context,
		    _("SOAP response does not contain element from name space %s"),
		    name_space_local);
		free(name_space_local);
		err = IE_ISDS;
		goto leave;
	}

	/* TODO: validate the response */

	/* Build XML document */
	*response = xmlNewDoc(BAD_CAST "1.0");
	if (NULL == *response) {
		isds_log_message(context,
		    _("Could not build ISDS response document"));
		err = IE_ERROR;
		goto leave;
	}
	xmlDocSetRootElement(*response, isds_node);

leave:
	if (IE_SUCCESS != err) {
		xmlFreeDoc(*response);
		if (NULL != raw_response) {
			zfree(*raw_response);
		}
	}
	xmlFreeDoc(response_document);

	return err;
}
#endif /* HAVE_LIBCURL */


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
_hidden isds_error validate_message_id_length(struct isds_ctx *context,
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


#if HAVE_LIBCURL
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
    xmlDocPtr response_document = NULL;
    xmlNodePtr response_body = NULL, deposit_node;

    if (!context) return IE_INVALID_CONTEXT;
    if (!response) return IE_INVAL;

    err = _isds_soap(context, NULL, request,
            &response_document, &response_body, NULL, NULL);

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
    xmlFreeDoc(response_document);

    return err;
}
#endif /* HAVE_LIBCURL */
