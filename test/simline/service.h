#ifndef __ISDS_SERVICE_H
#define __ISDS_SERVICE_H

#include "http.h"

/* Parse and respond to DummyOperation */
void service_DummyOperation(int socket, const void *request, size_t request_length);

#endif
