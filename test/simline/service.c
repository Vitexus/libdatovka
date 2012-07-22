#define _XOPEN_SOURCE 500 /* For strdup(3) */
#include "../test-tools.h"
#include "http.h"
#include "services.h"
#include <string.h>
#include <libxml/parser.h>
#include <libxml/xpath.h>
#include <libxml/xpathInternals.h>
#include <libxml/xmlsave.h>

static const char *soap_mime_type = "text/xml"; /* SOAP/1.1 requires text/xml */

/* Used to choose proper name space for message elements.
 * See _isds_register_namespaces(). */
typedef enum {
    MESSAGE_NS_1,
    MESSAGE_NS_UNSIGNED,
    MESSAGE_NS_SIGNED_INCOMING,
    MESSAGE_NS_SIGNED_OUTGOING,
    MESSAGE_NS_SIGNED_DELIVERY
} message_ns_type;

#define SOAP_NS "http://schemas.xmlsoap.org/soap/envelope/"
#define SOAP2_NS "http://www.w3.org/2003/05/soap-envelope"
#define ISDS1_NS "http://isds.czechpoint.cz"
#define ISDS_NS "http://isds.czechpoint.cz/v20"
#define SISDS_INCOMING_NS "http://isds.czechpoint.cz/v20/message"
#define SISDS_OUTGOING_NS "http://isds.czechpoint.cz/v20/SentMessage"
#define SISDS_DELIVERY_NS "http://isds.czechpoint.cz/v20/delivery"
#define SCHEMA_NS "http://www.w3.org/2001/XMLSchema"
#define DEPOSIT_NS "urn:uschovnaWSDL"


struct service {
    service_id id;
    const char *end_point;
    const xmlChar *name;
    http_error (*function) (int, xmlDocPtr, xmlXPathContextPtr, xmlNodePtr,
            xmlDocPtr, xmlNodePtr, const void *arguments);
};

/* Following EXTRACT_* macros expect @xpath_ctx, @message, and leave label */
#define EXTRACT_STRING(element, string) { \
    xmlXPathObjectPtr result = NULL; \
    result = xmlXPathEvalExpression(BAD_CAST element "/text()", xpath_ctx); \
    if (NULL == result) { \
        error = HTTP_ERROR_SERVER; \
        goto leave; \
    } \
    if (!xmlXPathNodeSetIsEmpty(result->nodesetval)) { \
        if (result->nodesetval->nodeNr > 1) { \
            xmlXPathFreeObject(result); \
            test_asprintf(&message, "Multiple %s element", element); \
            error = HTTP_ERROR_CLIENT; \
            goto leave; \
        } \
        (string) = (char *) \
            xmlXPathCastNodeSetToString(result->nodesetval); \
        if (!(string)) { \
            xmlXPathFreeObject(result); \
            error = HTTP_ERROR_SERVER; \
            goto leave; \
        } \
    } \
    xmlXPathFreeObject(result); \
}

/* Following INSERT_* macros expect @error and leave label */
#define INSERT_STRING_WITH_NS(parent, ns, element, string) \
    { \
        xmlNodePtr node = xmlNewTextChild(parent, ns, BAD_CAST (element), \
                (xmlChar *) (string)); \
        if (NULL == node) { \
            error = HTTP_ERROR_SERVER; \
            goto leave; \
        } \
    }

#define INSERT_STRING(parent, element, string) \
    { INSERT_STRING_WITH_NS(parent, NULL, element, string) }

#define INSERT_ELEMENT(child, parent, element) \
    { \
        (child) = xmlNewChild((parent), NULL, BAD_CAST (element), NULL); \
        if (NULL == (child)) { \
            error = HTTP_ERROR_SERVER; \
            goto leave; \
        } \
    }

/* Insert dmStatus or similar subtree
 * @parent is element to insert to
 * @dm is true for dmStatus, otherwise dbStatus
 * @code is stautus code as string
 * @message is UTF-8 encoded message
 * @db_ref_number is optinal reference number propagated if not @dm
 * @return 0 on success, otherwise non-0. */
static http_error insert_isds_status(xmlNodePtr parent, _Bool dm,
        const xmlChar *code, const xmlChar *message,
        const xmlChar *db_ref_number) {
    http_error error = HTTP_ERROR_SUCCESS;
    xmlNodePtr status;
    
    if (NULL == code || NULL == message) {
        error = HTTP_ERROR_SERVER;
        goto leave;
    }

    INSERT_ELEMENT(status, parent, (dm) ? "dmStatus" : "dbStatus");
    INSERT_STRING(status, (dm) ? "dmStatusCode" : "dbStatusCode", code);
    INSERT_STRING(status, (dm) ? "dmStatusMessage" : "dbStatusMessage", message);
    if (!dm && NULL != db_ref_number) {
        INSERT_STRING(status, "dbStatusRefNumber", db_ref_number);
    }

leave:
    return error;
}


/* Implement DummyOperation */
static http_error service_DummyOperation(int socket, const xmlDocPtr soap_request,
        xmlXPathContextPtr xpath_ctx, xmlNodePtr isds_request,
        xmlDocPtr soap_response, xmlNodePtr isds_response,
        const void *arguments) {
    return insert_isds_status(isds_response, 1, BAD_CAST "0000",
            BAD_CAST "Success", NULL);
}


/* Implement ChangeISDSPassword.
 * @arguments is current password as const char * */
static http_error service_ChangeISDSPassword(int socket,
        const xmlDocPtr soap_request, xmlXPathContextPtr xpath_ctx,
        const xmlNodePtr isds_request,
        xmlDocPtr soap_response, xmlNodePtr isds_response,
        const void *arguments) {
    http_error error = HTTP_ERROR_SUCCESS;
    char *code = "9999", *message = NULL;
    const struct arguments_DS_DsManage_ChangeISDSPassword *configuration =
        (const struct arguments_DS_DsManage_ChangeISDSPassword *)arguments;
    char *old_password = NULL, *new_password = NULL;
    size_t length;

    if (NULL == configuration || NULL == configuration->username ||
            NULL == configuration->current_password) {
        error = HTTP_ERROR_SERVER;
        goto leave;
    }

    /* Parse request */
    EXTRACT_STRING("isds:dbOldPassword", old_password);
    if (NULL == old_password) {
        message = strdup("Empty isds:dbOldPassword");
        error = HTTP_ERROR_CLIENT;
        goto leave;
    }
    EXTRACT_STRING("isds:dbNewPassword", new_password);
    if (NULL == new_password) {
        message = strdup("Empty isds:dbOldPassword");
        error = HTTP_ERROR_CLIENT;
        goto leave;
    }

    /* Check defined cases */
    if (strcmp(configuration->current_password, old_password)) {
        code = "1090";
        message = strdup("Bad current password");
        error = HTTP_ERROR_CLIENT;
        goto leave;
    }

    length = strlen(new_password);

    if (length < 8 || length > 32) {
        code = "1066";
        message = strdup("Too short or too long");
        error = HTTP_ERROR_CLIENT;
        goto leave;
    }

    {
        const char lower[] = "abcdefghijklmnopqrstuvwxyz";
        const char upper[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ";
        const char digit[] = "0123456789";
        const char special[] = "!#$%&()*+,-.:=?@[]_{}|~";
        _Bool has_lower = 0, has_upper = 0, has_digit=0;

        for (int i = 0; i < length; i++) {
            if (NULL != strchr(lower, new_password[i]))
                has_lower = 1;
            else if (NULL != strchr(upper, new_password[i]))
                has_upper = 1;
            else if (NULL != strchr(digit, new_password[i]))
                has_digit = 1;
            else if (NULL == strchr(special, new_password[i])) {
                code = "1079";
                message = strdup("Password contains forbidden character");
                error = HTTP_ERROR_CLIENT;
                goto leave;
            }
        }

        if (!has_lower || !has_upper || !has_digit) {
            code = "1080";
            message = strdup("Password does not contain lower cased letter, "
                    "upper cased letter and a digit");
            error = HTTP_ERROR_CLIENT;
            goto leave;
        }
    }

    if (!strcmp(old_password, new_password)) {
        code = "1067";
        message = strdup("New password same as current one");
        error = HTTP_ERROR_CLIENT;
        goto leave;
    }

    if (NULL != strstr(new_password, configuration->username)) {
        code = "1082";
        message = strdup("New password contains user ID");
        error = HTTP_ERROR_CLIENT;
        goto leave;
    }

    for (int i = 0; i < length - 2; i++) {
        if (new_password[i] == new_password[i+1] &&
                new_password[i] == new_password[i+2]) {
            code = "1083";
            message = strdup("Password contains sequence "
                    "of three identical characters");
            error = HTTP_ERROR_CLIENT;
            goto leave;
        }
    }
    
    {
        const char *forbidden_prefix[] = { "qwert", "asdgf", "12345" };
        for (int i = 0; i < sizeof(forbidden_prefix)/sizeof(*forbidden_prefix);
                i++) {
            if (!strncmp(new_password, forbidden_prefix[i],
                        strlen(forbidden_prefix[i]))) {
                code = "1083";
                message = strdup("Password has forbidden prefix");
                error = HTTP_ERROR_CLIENT;
                goto leave;
            }
        }
    }

    code = "0000";
    message = strdup("Success");
leave:
    if (HTTP_ERROR_SERVER != error) {
        http_error next_error = insert_isds_status(isds_response, 0,
                BAD_CAST code, BAD_CAST message, NULL);
        if (HTTP_ERROR_SUCCESS != next_error) error = next_error;
    }
    free(old_password);
    free(new_password);
    free(message);
    return error;
}


/* List of implemented services */
static struct service services[] = {
    { SERVICE_DS_Dz_DummyOperation,
        "DS/dz", BAD_CAST "DummyOperation",
        service_DummyOperation },
    { SERVICE_DS_DsManage_ChangeISDSPassword,
        "DS/DsManage", BAD_CAST "ChangeISDSPassword",
        service_ChangeISDSPassword },
};


/* Makes known all relevant namespaces to given XPath context
 * @xpath_ctx is XPath context
 * @message_ns selects proper message name space. Unsigned and signed
 * messages and delivery info's differ in prefix and URI.
 * @return 0 in success, otherwise not 0. */
static int register_namespaces(xmlXPathContextPtr xpath_ctx,
        const message_ns_type message_ns) {
    const xmlChar *message_namespace = NULL;

    if (!xpath_ctx) return -1;

    switch(message_ns) {
        case MESSAGE_NS_1:
            message_namespace = BAD_CAST ISDS1_NS; break;
        case MESSAGE_NS_UNSIGNED:
            message_namespace = BAD_CAST ISDS_NS; break;
        case MESSAGE_NS_SIGNED_INCOMING:
            message_namespace = BAD_CAST SISDS_INCOMING_NS; break;
        case MESSAGE_NS_SIGNED_OUTGOING:
            message_namespace = BAD_CAST SISDS_OUTGOING_NS; break;
        case MESSAGE_NS_SIGNED_DELIVERY:
            message_namespace = BAD_CAST SISDS_DELIVERY_NS; break;
        default:
            return -1;
    }

    if (xmlXPathRegisterNs(xpath_ctx, BAD_CAST "soap", BAD_CAST SOAP_NS))
        return -1;
    if (xmlXPathRegisterNs(xpath_ctx, BAD_CAST "isds", BAD_CAST ISDS_NS))
        return -1;
    if (xmlXPathRegisterNs(xpath_ctx, BAD_CAST "sisds", message_namespace))
        return -1;
    if (xmlXPathRegisterNs(xpath_ctx, BAD_CAST "xs", BAD_CAST SCHEMA_NS))
        return -1;
    if (xmlXPathRegisterNs(xpath_ctx, BAD_CAST "deposit", BAD_CAST DEPOSIT_NS))
        return -1;
    return 0;
}


/* Parse soap request, pass it to service endpoint and respond to it.
 * It sends final HTTP response. */
void soap(int socket, const struct service_configuration *configuration,
        const void *request, size_t request_length, const char *end_point) {
    xmlDocPtr request_doc = NULL;
    xmlXPathContextPtr xpath_ctx = NULL;
    xmlXPathObjectPtr request_soap_body = NULL;
    xmlNodePtr isds_request = NULL; /* pointer only */
    _Bool service_handled = 0, service_passed = 0;
    xmlDocPtr response_doc = NULL;
    xmlNodePtr response_soap_envelope = NULL, response_soap_body = NULL,
               isds_response = NULL;
    xmlNsPtr soap_ns = NULL, isds_ns = NULL;
    char *response_name = NULL;
    xmlBufferPtr http_response_body = NULL;
    xmlSaveCtxtPtr save_ctx = NULL;


    if (NULL == configuration) {
        http_send_response_500(socket, "Second argument of soap() is NULL");
        return;
    }

    if (NULL == request || request_length == 0) {
        http_send_response_400(socket, "Client sent empty body");
        return;
    }

    request_doc = xmlParseMemory(request, request_length);
    if (NULL == request_doc) {
        http_send_response_400(socket, "Client sent invalid XML document");
        return;
    }
    
    xpath_ctx = xmlXPathNewContext(request_doc);
    if (NULL == xpath_ctx) {
        xmlFreeDoc(request_doc);
        http_send_response_500(socket, "Could not create XPath context");
        return;
    }

    if (register_namespaces(xpath_ctx, MESSAGE_NS_UNSIGNED)) {
        xmlXPathFreeContext(xpath_ctx);
        xmlFreeDoc(request_doc);
        http_send_response_500(socket,
                "Could not register name spaces to the XPath context");
        return;
    }

    /* Get SOAP Body */
    request_soap_body = xmlXPathEvalExpression(
            BAD_CAST "/soap:Envelope/soap:Body", xpath_ctx);
    if (NULL == request_soap_body) {
        xmlXPathFreeContext(xpath_ctx);
        xmlFreeDoc(request_doc);
        http_send_response_400(socket, "Client sent invalid SOAP request");
        return;
    }
    if (xmlXPathNodeSetIsEmpty(request_soap_body->nodesetval)) {
        xmlXPathFreeObject(request_soap_body);
        xmlXPathFreeContext(xpath_ctx);
        xmlFreeDoc(request_doc);
        http_send_response_400(socket,
                "SOAP request does not contain SOAP Body element");
        return;
    }
    if (request_soap_body->nodesetval->nodeNr > 1) {
        xmlXPathFreeObject(request_soap_body);
        xmlXPathFreeContext(xpath_ctx);
        xmlFreeDoc(request_doc);
        http_send_response_400(socket,
                "SOAP response has more than one Body element");
        return;
    }
    isds_request = request_soap_body->nodesetval->nodeTab[0]->children;
    if (isds_request->next != NULL) {
        xmlXPathFreeObject(request_soap_body);
        xmlXPathFreeContext(xpath_ctx);
        xmlFreeDoc(request_doc);
        http_send_response_400(socket, "SOAP body has more than one child");
        return;
    }
    if (isds_request->type != XML_ELEMENT_NODE || isds_request->ns == NULL ||
            xmlStrcmp(isds_request->ns->href, BAD_CAST ISDS_NS)) {
        xmlXPathFreeObject(request_soap_body);
        xmlXPathFreeContext(xpath_ctx);
        xmlFreeDoc(request_doc);
        http_send_response_400(socket,
                "SOAP body does not contain an ISDS elment");
        return;
    }

    /* Build SOAP response envelope */
    response_doc = xmlNewDoc(BAD_CAST "1.0");
    if (!response_doc) {
        http_send_response_500(socket, "Could not build SOAP response document");
        goto leave;
    }
    response_soap_envelope = xmlNewNode(NULL, BAD_CAST "Envelope");
    if (!response_soap_envelope) {
        http_send_response_500(socket, "Could not build SOAP response envelope");
        goto leave;
    }
    xmlDocSetRootElement(response_doc, response_soap_envelope);
    /* Only this way we get namespace definition as @xmlns:soap,
     * otherwise we get namespace prefix without definition */
    soap_ns = xmlNewNs(response_soap_envelope, BAD_CAST SOAP_NS, NULL);
    if(NULL == soap_ns) {
        http_send_response_500(socket, "Could not create SOAP name space");
        goto leave;
    }
    xmlSetNs(response_soap_envelope, soap_ns);
    response_soap_body = xmlNewChild(response_soap_envelope, NULL,
            BAD_CAST "Body", NULL);
    if (!response_soap_body) {
        http_send_response_500(socket,
                "Could not add Body to SOAP response envelope");
        goto leave;
    }
    /* Append ISDS response element */
    if (-1 == test_asprintf(&response_name, "%s%s", isds_request->name,
                "Response")) {
        http_send_response_500(socket,
                "Could not buld ISDS resposne element name");
        goto leave;
    }
    isds_response = xmlNewChild(response_soap_body, NULL,
            BAD_CAST response_name, NULL);
    free(response_name);
    if (NULL == isds_response) {
        http_send_response_500(socket,
                "Could not add ISDS response element to SOAP response body");
        goto leave;
    }
    isds_ns = xmlNewNs(isds_response, BAD_CAST ISDS_NS, NULL);
    if(NULL == isds_ns) {
        http_send_response_500(socket, "Could not create ISDS name space");
        goto leave;
    }
    xmlSetNs(isds_response, isds_ns);

    /* Dispatch request to service */
    for (int i = 0; i < sizeof(services)/sizeof(services[0]); i++) {
        if (!strcmp(services[i].end_point, end_point) &&
                !xmlStrcmp(services[i].name, isds_request->name)) {
            /* Check if the configuration is enabled and find configuration */
            for (const struct service_configuration *service = configuration;
                    service->name != SERVICE_END; service++) {
                if (service->name == services[i].id) {
                    service_handled = 1;
                    xpath_ctx->node = isds_request;
                    if (HTTP_ERROR_SERVER != services[i].function(socket,
                                request_doc, xpath_ctx, isds_request,
                                response_doc, isds_response,
                                service->arguments)) {
                        service_passed = 1;
                    } else {
                        http_send_response_500(socket,
                                "Internal server error while processing "
                                "ISDS request");
                    }
                }
            }
            break;
        }
    }
    
    /* Send response */
    if (service_passed) {
        /* Serialize the SOAP response */
        http_response_body = xmlBufferCreate();
        if (NULL == http_response_body) {
            http_send_response_500(socket,
                    "Could not create xmlBuffer for response serialization");
            goto leave;
        }
        /* Last argument 1 means format the XML tree. This is pretty but it breaks
         * XML document transport as it adds text nodes (indentiation) between
         * elements. */
        save_ctx = xmlSaveToBuffer(http_response_body, "UTF-8", 0);
        if (NULL == save_ctx) {
            http_send_response_500(socket, "Could not create XML serializer");
            goto leave;
        }
        /* XXX: According LibXML documentation, this function does not return
         * meaningful value yet */
        xmlSaveDoc(save_ctx, response_doc);
        if (-1 == xmlSaveFlush(save_ctx)) {
            http_send_response_500(socket,
                    "Could not serialize SOAP response");
            goto leave;
        }

        http_send_response_200(socket, http_response_body->content,
                http_response_body->use, soap_mime_type);
    }

leave:
    xmlSaveClose(save_ctx);
    xmlBufferFree(http_response_body);

    xmlFreeDoc(response_doc);

    xmlXPathFreeObject(request_soap_body);
    xmlXPathFreeContext(xpath_ctx);
    xmlFreeDoc(request_doc);

    if (!service_handled) {
        http_send_response_500(socket,
                "Requested ISDS service not implemented");
    }

}


