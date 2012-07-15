#include "http.h"
#include <string.h>

static const char *soap_mime_type = "text/xml"; /* SOAP/1.1 requires text/xml */
/* DummyOperation response */
static const char *pong = "<?xml version='1.0' encoding='utf-8'?><SOAP-ENV:Envelope xmlns:SOAP-ENV=\"http://schemas.xmlsoap.org/soap/envelope/\" xmlns:xsd=\"http://www.w3.org/2001/XMLSchema\"><SOAP-ENV:Body><q:DummyOperationResponse xmlns:q=\"http://isds.czechpoint.cz/v20\" xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\"><q:dmStatus><q:dmStatusCode>0000</q:dmStatusCode><q:dmStatusMessage>Provedeno úspěšně.</q:dmStatusMessage></q:dmStatus></q:DummyOperationResponse></SOAP-ENV:Body></SOAP-ENV:Envelope>";

/* Parse and respond to DummyOperation */
void service_DummyOperation(int socket, const void *request, size_t request_length) {
    http_send_response_200(socket, pong, strlen(pong), soap_mime_type);
}
