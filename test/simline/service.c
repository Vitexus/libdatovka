#include "http.h"
#include <string.h>
#include <libxml/parser.h>
#include <libxml/xpath.h>
#include <libxml/xpathInternals.h>

static const char *soap_mime_type = "text/xml"; /* SOAP/1.1 requires text/xml */
/* DummyOperation response */
static const char *pong = "<?xml version='1.0' encoding='utf-8'?><SOAP-ENV:Envelope xmlns:SOAP-ENV=\"http://schemas.xmlsoap.org/soap/envelope/\" xmlns:xsd=\"http://www.w3.org/2001/XMLSchema\"><SOAP-ENV:Body><q:DummyOperationResponse xmlns:q=\"http://isds.czechpoint.cz/v20\" xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\"><q:dmStatus><q:dmStatusCode>0000</q:dmStatusCode><q:dmStatusMessage>Provedeno úspěšně.</q:dmStatusMessage></q:dmStatus></q:DummyOperationResponse></SOAP-ENV:Body></SOAP-ENV:Envelope>";

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
    const char *end_point;
    const xmlChar *name;
    void (*function) (int, xmlDocPtr, xmlXPathContextPtr, xmlNodePtr);
};

/* Parse and respond to DummyOperation */
static void service_DummyOperation(int socket, const xmlDocPtr soap_request,
        xmlXPathContextPtr xpath_ctx, xmlNodePtr isds_request) {
    http_send_response_200(socket, pong, strlen(pong), soap_mime_type);
}


/* List of implemented services */
static struct service services[] = {
    { "DS/dz", BAD_CAST "DummyOperation", service_DummyOperation },
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
void soap(int socket, const void *request, size_t request_length,
        const char *end_point) {
    xmlDocPtr request_doc = NULL;
    xmlXPathContextPtr xpath_ctx = NULL;
    xmlXPathObjectPtr request_soap_body = NULL;
    xmlNodePtr isds_request = NULL; /* pointer only */
    _Bool service_handled = 0;

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


    /* Dispatch request to service */
    for (int i = 0; i < sizeof(services)/sizeof(services[0]); i++) {
        if (!strcmp(services[i].end_point, end_point) &&
                !xmlStrcmp(services[i].name, isds_request->name)) {
            services[i].function(socket, request_doc, xpath_ctx, isds_request);
            service_handled = 1;
            break;
        }
    }

    xmlXPathFreeObject(request_soap_body);
    xmlXPathFreeContext(xpath_ctx);
    xmlFreeDoc(request_doc);

    if (service_handled) {
        http_send_response_500(socket,
                "Requested ISDS service not implemented");
    }

}


