#ifndef __ISDS_SERVICE_H
#define __ISDS_SERVICE_H

#include "http.h"

/* Parse soap request, pass it to service endpoint and respond to it.
 * It sends final HTTP response. */
void soap(int socket, const struct service_configuration *configuration,
        const void *request, size_t request_length, const char *end_point);

#endif
