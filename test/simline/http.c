#include "http.h"
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h> /* fprintf() */

/* Read a line from HTTP socket.
 * @socket is descriptor to read from.
 * @line is auto-allocated just read line. Will be NULL if EOF has been
 * reached or error occured.
 * @buffer is automatically reallocated buffer for the socket. It can preserve
 * prematurately read socket data.
 * @buffer_size is allocated size of @buffer
 * @buffer_length is size of head of the buffer that holds read data.
 * @return 0 in success. */
static int http_read_line(int socket, char **line,
        char **buffer, size_t *buffer_size, size_t *buffer_used) {
    ssize_t got;
    char *p, *tmp;

    if (line == NULL) return -1;
    *line = NULL;

    if (buffer == NULL || buffer_size == NULL || buffer_used == NULL)
        return -1;
    if (*buffer == NULL && *buffer_size > 0) return -1;
    if (*buffer_size < *buffer_used) return -1;

#define BURST 1024
    while (1) {
        if (*buffer_size == *buffer_used) {
            /* Grow buffer */
            tmp = realloc(*buffer, *buffer_size + BURST);
            if (tmp == NULL) return -1;
            *buffer = tmp;
            *buffer_size += BURST;
        }

        /* Read data */
        got = read(socket, *buffer + *buffer_used, *buffer_size - *buffer_used);
        if (got == -1 && errno != EINTR) return -1;

        /* Check for EOL */
        for (p = *buffer + *buffer_used; p < *buffer + *buffer_used + got; p++)
        {
            if (*p != '\r')
                continue;
            if (!(p + 1 < *buffer + *buffer_used + got && p[1] == '\n'))
                continue;

            /* EOL found */
            /* Crop by zero at EOL */
            *p = '\0';
            p += 2;
            /* Copy read ahead data to new buffer and point line to original
             * buffer. */
            tmp = malloc(BURST);
            if (tmp == NULL) return -1;
            memcpy(tmp, p, *buffer + *buffer_used + got - p);
            *line = *buffer;
            *buffer_size = BURST;
            *buffer_used = *buffer + *buffer_used + got - p;
            *buffer = tmp;
            /* And exit */
            return 0;
        }
        
        /* Chek for EOF */
        if (got == 0) return -1;

        /* Move end of buffer */
        *buffer_used += got;
    }
#undef BURST

    return -1;
}


/* Parse HTTP request header.
 * @request is pre-allocated HTTP request.
 * @return 0 if success. */
static int http_parse_request_header(char *line,
        struct http_request *request) {
    char *p;
    size_t length;

    fprintf(stderr, "Request: <%s>\n", line);

    /* Get method */
    p = strchr(line, ' ');
    if (p == NULL) return -1;
    *p = '\0';
    if (strcmp(line, "GET"))
        request->method = HTTP_METHOD_GET;
    else if (strcmp(line, "POST"))
        request->method = HTTP_METHOD_POST;
    else
        request->method = HTTP_METHOD_UNKNOWN;
    line = p + 1;

    /* Get URI */
    /* TODO: URI-decode */
    p = strchr(line, ' ');
    if (p == NULL) *p = '\0';
    length = strlen(line);
    request->uri = malloc(length + 1);
    if (request->uri == NULL) return -1;
    strcpy(request->uri, line);

    /* Do not care about HTTP version */

    return 0;
}


/* Parse generic HTTP header.
 * @request is pre-allocated HTTP request.
 * @return 0 if success. */
static int http_parse_header(char *line, struct http_request *request) {
    /* FIXME: Parse, multi-line, decode */
    fprintf(stderr, "Header: <%s>\n", line);
    return 0;
}


/* Read a HTTP request from connected socket.
 * @return is heap-allocated received HTTP request, or NULL in case of error. */
struct http_request *http_read_request(int socket) {
    struct http_request *request = NULL;
    char *line = NULL;
    char *buffer = NULL;
    size_t buffer_size = 0, buffer_used = 0;

    request = malloc(sizeof(*request));
    if (request == NULL) return NULL;
    memset(request, 0, sizeof(*request));

    /* Get request header */
    if (-1 == http_read_line(socket, &line, &buffer, &buffer_size,
                &buffer_used)) {
        http_request_free(&request);
        goto leave;
    }
    if (http_parse_request_header(line, request)) {
        http_request_free(&request);
        goto leave;
    }

    /* Get other headers */
    while (1) {
        if (-1 == http_read_line(socket, &line, &buffer, &buffer_size,
                    &buffer_used)) {
            http_request_free(&request);
            goto leave;
        }

        /* Check for headers delimiter */
        if (line == NULL || *line == '\0')
            break;

        if (http_parse_header(line, request)) {
            http_request_free(&request);
            goto leave;
        }
    };


leave:
    free(line);
    free(buffer);
    return request;;
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
