#ifndef __ISDS_HTTP_H
#define __ISDS_HTTP_H

#include <sys/types.h>

typedef enum {
    HTTP_METHOD_UNKNOWN = 0,
    HTTP_METHOD_GET,
    HTTP_METHOD_POST
} http_method;

struct http_header {
    char *name;
    char *value;
    struct http_header *next;
};

struct http_request {
    http_method method;
    char *uri;
    struct http_header *headers; /* NULL terminated linked list */
    char *body;
    size_t body_length;
};

/* Free HTTP header and set it to NULL */
void http_header_free(struct http_header **header);

/* Free HTTP headers and set it to NULL */
void http_headers_free(struct http_header **headers);

/* Free HTTP request and set it to NULL */
void http_request_free(struct http_request **request);

/* Read a HTTP request from connected socket.
 * @return is heap-allocated received HTTP request, or NULL in case of error. */
struct http_request *http_read_request(int socket);

#endif
