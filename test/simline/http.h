#ifndef __ISDS_HTTP_H
#define __ISDS_HTTP_H

#include <sys/types.h>

/* Forward declarations */
struct http_connection;

/* Call-back type for non-interrupting receiving from socket. See recv(2).
 * @buffer is data to send by the call-back
 * @length is number of bytes to send
 * @connection carries socket and callback_data. */
typedef ssize_t (*http_recv_callback_t) (
        const struct http_connection *connection, void *buffer, size_t length);

/* Call-back type for non-interrupting sending to socket. See send(2).
 * @buffer is memory to store received data by the call-back
 * @length is size of the @buffer in bytes
 * @connection carries socket and callback_data. */
typedef ssize_t (*http_send_callback_t) (
        const struct http_connection *connection, const void *buffer,
        size_t length);

struct http_connection {
    int socket;     /* Accepted TCP client socket */
    http_recv_callback_t recv_callback; /* Non-interrupting reading
                                           from socket */
    http_send_callback_t send_callback; /* Non-interrupting writing
                                           to socket */
    void *callback_data;    /* Pointer to pass to callbacks */
};

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

/* Read a HTTP request from connection.
 * @http_request is heap-allocated received HTTP request,
 * or NULL in case of error.
 * @return http_error code. */
http_error http_read_request(const struct http_connection *connection,
        struct http_request **request);

/* Write a HTTP response to connection. Auto-add Content-Length header.
 * @return 0 in case of success. */
int http_write_response(const struct http_connection *connection,
        const struct http_response *response);

/* Send a 200 Ok response with a cookie */
int http_send_response_200_cookie(const struct http_connection *connection,
        const char *cokie_name, const char *cookie_value,
        const char *cookie_domain, const char *cookie_path,
        const void *body, size_t body_length, const char *type);

/* Send a 200 Ok response */
int http_send_response_200(const struct http_connection *connection,
        const void *body, size_t body_length, const char *type);

/* Send a 302 Found response setting a cookie */
int http_send_response_302_cookie(const struct http_connection *connection,
        const char *cokie_name, const char *cookie_value,
        const char *cookie_domain, const char *cookie_path,
        const char *location);

/* Send a 302 Found response with totp authentication scheme header */
int http_send_response_302_totp(const struct http_connection *connection,
        const char *code, const char *text, const char *location);

/* Send a 400 Bad Request response.
 * Use non-NULL @reason to override status message. */
int http_send_response_400(const struct http_connection *connection,
        const char *reason);

/* Send a 401 Unauthorized response with Basic authentication scheme header */
int http_send_response_401_basic(const struct http_connection *connection);

/* Send a 401 Unauthorized response with OTP authentication scheme header for
 * given @method. */
int http_send_response_401_otp(const struct http_connection *connection,
        const char *method, const char *code, const char *text);

/* Send a 403 Forbidden response */
int http_send_response_403(const struct http_connection *connection);

/* Send a 500 Internal Server Error response.
 * Use non-NULL @reason to override status message. */
int http_send_response_500(const struct http_connection *connection,
        const char *reason);

/* Send a 503 Service Temporarily Unavailable response */
int http_send_response_503(const struct http_connection *connection,
        const void *body, size_t body_length, const char *type);

/* Returns true if request carries WWW-Authenticate header */
int http_client_authenticates(const struct http_request *request);

/* Return HTTP_ERROR_SUCCESS if request carries valid Basic credentials.
 * NULL @username or @password equal to empty string. */
http_error http_authenticate_basic(const struct http_request *request,
        const char *username, const char *password);

/* Return HTTP_ERROR_SUCCESS if request carries valid OTP credentials.
 * NULL @username or @password or @otp equal to empty string. */
http_error http_authenticate_otp(const struct http_request *request,
        const char *username, const char *password, const char *otp);

/* Return cookie value by name or NULL if does not present. */
const char *http_find_cookie(const struct http_request *request,
        const char *name);

/* Return Host header value or NULL if does not present. Returned string is
 * statically allocated. */
const char *http_find_host(const struct http_request *request);

#endif
