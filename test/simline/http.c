#include "http.h"
#include <stdlib.h>

/* Read a HTTP request from connected socket.
 * @request is automatically allocated received HTTP request
 * @return 0 in case of success */
int http_read_request(int socket, struct http_request *request) {
    return -1;
}

/* Free HTTP header and set it to NULL */
void http_header_free(struct http_header **header) {
    if (header == NULL || *header == NULL) return;
    free((*header)->name);
    free((*header)->value);
    free(*header);
    *header = NULL;
}

/* Free HTTP headers and set it to NULL */
void http_headers_free(struct http_header **headers) {
    struct http_header *header, *next;

    if (headers == NULL || *headers == NULL) return;

    for (header = *headers; header != NULL;) {
        next = header;
        http_header_free(&header);
        next++;
    }

    free(*headers);
    *headers = NULL;
}

/* Free HTTP request and set it to NULL */
void http_request_free(struct http_request **request) {
    if (request == NULL || *request == NULL) return;
    free((*request)->uri);
    http_headers_free(&((*request)->headers));
    free((*request)->body);
    free(*request);
    *request = NULL;
}
