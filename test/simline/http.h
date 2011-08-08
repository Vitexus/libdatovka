#ifndef __ISDS_HTTP_H
#define __ISDS_HTTP_H

#include <sys/types.h>

typedef enum {
    HTTP_ERROR_SERVER = -1,
    HTTP_ERROR_SUCCESS = 0,
    HTTP_ERROR_CLIENT = 1
} http_error;

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
    void *body;
    size_t body_length;
};

struct http_response {
    unsigned int status;
    char *reason;
    struct http_header *headers; /* NULL terminated linked list */
    void *body;
    size_t body_length;
};

/* Free HTTP header and set it to NULL */
void http_header_free(struct http_header **header);

/* Free HTTP headers and set it to NULL */
void http_headers_free(struct http_header **headers);

/* Free HTTP request and set it to NULL */
void http_request_free(struct http_request **request);

/* Free HTTP response and set it to NULL */
void http_response_free(struct http_response **response);

/* Read a HTTP request from connected socket.
 * @return is heap-allocated received HTTP request, or NULL in case of error. */
struct http_request *http_read_request(int socket);

/* Write a HTTP response to connected socket. Auto-add Content-Length header.
 * @return 0 in case of success. */
int http_write_response(int socket, const struct http_response *response);

/* Send a 401 Unauthorized response with Basic authentication scheme header */ 
int http_send_response_401_basic(int socket);

#endif
