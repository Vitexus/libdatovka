#include "http.h"
#include "../test-tools.h"
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h> /* fprintf() */
#include <limits.h>
#include <strings.h> /* strcasecmp() */

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
        /* Check for EOL */
        for (p = *buffer; p < *buffer + *buffer_used; p++)
        {
            if (*p != '\r')
                continue;
            if (!(p + 1 < *buffer + *buffer_used && p[1] == '\n'))
                continue;

            /* EOL found */
            /* Crop by zero at EOL */
            *p = '\0';
            p += 2;
            /* Copy read ahead data to new buffer and point line to original
             * buffer. */
            tmp = malloc(BURST);
            if (tmp == NULL) return -1;
            memcpy(tmp, p, *buffer + *buffer_used - p);
            *line = *buffer;
            *buffer_size = BURST;
            *buffer_used = *buffer + *buffer_used - p;
            *buffer = tmp;
            /* And exit */
            return 0;
        }
        
        if (*buffer_size == *buffer_used) {
            /* Grow buffer */
            tmp = realloc(*buffer, *buffer_size + BURST);
            if (tmp == NULL) return -1;
            *buffer = tmp;
            *buffer_size += BURST;
        }

        /* Read data */
        got = read(socket, *buffer + *buffer_used, *buffer_size - *buffer_used);
        if (got == -1 && errno != EINTR) return 1;

        /* Check for EOF */
        if (got == 0) return 1;

        /* Move end of buffer */
        *buffer_used += got;
    }
#undef BURST

    return -1;
}


/* Write a bulk data into HTTP socket.
 * @socket is descriptor to write to.
 * @data are bitstream to send to client.
 * @length is size of @data in bytes.
 * @return 0 in success. */
static int http_write_bulk(int socket, const void *data, size_t length) {
    ssize_t written;
    const void *end;

    if (data == NULL && length > 0) return -1;

    for (end = data + length; data != end; data += written, length -= written) {
        written = write(socket, data, length);
        if (written == -1 && errno != EINTR) return 1;
    }

    return 0;
}


/* Write a line into HTTP socket.
 * @socket is descriptor to write to.
 * @line is NULL terminated string to send to client. HTTP EOL will be added.
 * @return 0 in success. */
static int http_write_line(int socket, const char *line) {
    if (line == NULL) return -1;

    fprintf(stderr, "Response: <%s>\n", line);

    /* Send the line */
    if (http_write_bulk(socket, line, strlen(line)))
        return 1;

    /* Send EOL */
    if (http_write_bulk(socket, "\r\n", 2))
        return 1;

    return 0;
}


/* Read data of given length from HTTP socket.
 * @socket is descriptor to read from.
 * @data is auto-allocated just read data bulk. Will be NULL if EOF has been
 * reached or error occured.
 * @data_length is size of bytes to read.
 * @buffer is automatically reallocated buffer for the socket. It can preserve
 * prematurately read socket data.
 * @buffer_size is allocated size of @buffer
 * @buffer_length is size of head of the buffer that holds read data.
 * @return 0 in success. */
static int http_read_bulk(int socket, void **data, size_t data_length,
        char **buffer, size_t *buffer_size, size_t *buffer_used) {
    ssize_t got;
    char *tmp;

    if (data == NULL) return -1;
    *data = NULL;

    if (buffer == NULL || buffer_size == NULL || buffer_used == NULL)
        return -1;
    if (*buffer == NULL && *buffer_size > 0) return -1;
    if (*buffer_size < *buffer_used) return -1;

    if (data_length <= 0) return 0;

#define BURST 1024
    while (1) {
        /* Check whether enought data have been read */
        if (*buffer_used >= data_length) {
            /* Copy read ahead data to new buffer and point data to original
             * buffer. */
            tmp = malloc(BURST);
            if (tmp == NULL) return -1;
            memcpy(tmp, *buffer + data_length, *buffer_used - data_length);
            *data = *buffer;
            *buffer_size = BURST;
            *buffer_used = *buffer_used - data_length;
            *buffer = tmp;
            /* And exit */
            return 0;
        }
        
        if (*buffer_size == *buffer_used) {
            /* Grow buffer */
            tmp = realloc(*buffer, *buffer_size + BURST);
            if (tmp == NULL) return -1;
            *buffer = tmp;
            *buffer_size += BURST;
        }

        /* Read data */
        got = read(socket, *buffer + *buffer_used, *buffer_size - *buffer_used);
        if (got == -1 && errno != EINTR) return 1;

        /* Check for EOF */
        if (got == 0) return 1;

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


/* Send HTTP response status line to client.
 * @return 0 if success. */
static int http_write_response_status(int socket,
        const struct http_response *response) {
    char *buffer = NULL;
    int error;

    if (response == NULL) return -1;

    if (-1 == test_asprintf(&buffer, "HTTP/1.0 %u %s", response->status,
                (response->reason == NULL) ? "" : response->reason))
        return -1;
    error = http_write_line(socket, buffer);
    free(buffer);
    
    return error;
}

    
/* Parse generic HTTP header.
 * @request is pre-allocated HTTP request.
 * @return 0 if success, negative value if internal error, positive value if
 * header is not valid. */
static int http_parse_header(char *line, struct http_request *request) {
    struct http_header *header;

    if (line == NULL || request == NULL) return -1;

    fprintf(stderr, "Header: <%s>\n", line);

    /* Find last used header */
    for (header = request->headers; header != NULL && header->next != NULL;
            header = header->next); 

    if (*line == ' ' || *line == '\t') {
        /* Line is continuation of last header */
        if (header == NULL) return 1;   /* No previous header to continue */
        line++;
        size_t old_length = strlen(header->value);
        char *tmp = realloc(header->value,
                sizeof(header->value[0]) * (old_length + strlen(line) + 1));
        if (tmp == NULL) return -1;
        header->value = tmp;
        strcpy(&header->value[old_length], line);
    } else {
        /* New header */
        struct http_header *new_header = calloc(sizeof(*new_header), 1);
        if (new_header == NULL) return -1;

        char *p = strstr(line, ": ");
        if (p == NULL) return 1;

        size_t length = p - line;
        new_header->name = malloc(sizeof(line[0]) * (length + 1));
        if (new_header->name == NULL) {
            http_header_free(&new_header);
            return -1;
        }
        strncpy(new_header->name, line, length);
        new_header->name[length] = '\0';

        p += 2;
        length = strlen(p);
        new_header->value = malloc(sizeof(p[0]) * (length + 1));
        if (new_header->value == NULL) {
            http_header_free(&new_header);
            return -1;
        }
        strcpy(new_header->value, p);

        if (request->headers == NULL)
            request->headers = new_header;
        else
            header->next = new_header;
    }


    /* FIXME: Decode. After parsing all headers as we could decode begining
     * and then got encoded continuation. */
    return 0;
}


/* Send HTTP header to client.
 * @return 0 if success. */
static int http_write_header(int socket, const struct http_header *header) {
    char *buffer = NULL;
    int error;

    if (header == NULL) return -1;

    if (header->name == NULL) return -1;

    /* TODO: Quote, split long lines */
    if (-1 == test_asprintf(&buffer, "%s: %s", header->name,
                (header->value == NULL) ? "" : header->value))
        return -1;
    error = http_write_line(socket, buffer);
    free(buffer);
    
    return error;
}

    
/* Find Content-Length value in HTTP request headers and set it into @request.
 * @return 0 in success. */
static int find_content_length(struct http_request *request) {
    struct http_header *header;
    if (request == NULL) return -1;

    for (header = request->headers; header != NULL; header = header->next) {
        if (header->name == NULL) continue;
        if (!strcasecmp(header->name, "Content-Length")) break;
    }

    if (header != NULL || header->value == NULL) {
        char *p;
        long long int value = strtol(header->value, &p, 10);
        if (*p != '\0')
            return 1;
        if ((value == LLONG_MIN || value == LLONG_MAX) && errno == ERANGE)
            return -1;
        if (value < 0)
            return 1;
        /* FIXME:
        if (value > SIZE_T_MAX)
            return -1;*/
        request->body_length = value;
    } else {
        request->body_length = 0;
    }
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
            fprintf(stderr, "Error while reading HTTP request line\n");
            http_request_free(&request);
            goto leave;
        }

        /* Check for headers delimiter */
        if (line == NULL || *line == '\0') {
            break;
        }

        if (http_parse_header(line, request)) {
            fprintf(stderr, "Error while parsing HTTP request line: <%s>\n",
                    line);
            http_request_free(&request);
            goto leave;
        }
    };

    /* Get body */
    if (find_content_length(request)) {
        fprintf(stderr, "Could not determine length of body\n");
        http_request_free(&request);
        goto leave;
    }
    if (http_read_bulk(socket, &request->body, request->body_length,
                &buffer, &buffer_size, &buffer_used)) {
        fprintf(stderr, "Could not read request body\n");
        http_request_free(&request);
        goto leave;
    }
    fprintf(stderr, "Body of size %zu B has been received\n",
            request->body_length);
    /* TODO: Decode body*/

leave:
    free(line);
    free(buffer);
    return request;
}


/* Write a HTTP response to connected socket. Auto-add Content-Length header.
 * @return 0 in case of success. */
int http_write_response(int socket, const struct http_response *response) {
    char *buffer = NULL;
    int error = -1;

    if (response == NULL) return -1;

    /* Status line */
    error = http_write_response_status(socket, response);
    if (error) return error;
    
    /* Headers */
    for (struct http_header *header = response->headers; header != NULL;
            header = header->next) {
        error = http_write_header(socket, header);
        if (error) return error;
    }
    if (-1 == test_asprintf(&buffer, "Content-Length: %u",
                response->body_length))
        return -1;
    error = http_write_line(socket, buffer);
    if (error) return error;

    /* Headers trailer */
    error = http_write_line(socket, "");
    if (error) return error;

    /* Body */
    if (response->body_length > 0) {
        error = http_write_bulk(socket, response->body, response->body_length);
        if (error) return error;
    }

    free(buffer);
    return 0;
}


/* Free a HTTP header and set it to NULL */
void http_header_free(struct http_header **header) {
    if (header == NULL || *header == NULL) return;
    free((*header)->name);
    free((*header)->value);
    free(*header);
    *header = NULL;
}


/* Free linked list of HTTP headers and set it to NULL */
void http_headers_free(struct http_header **headers) {
    struct http_header *header, *next;

    if (headers == NULL || *headers == NULL) return;

    for (header = *headers; header != NULL;) {
        next = header->next;
        http_header_free(&header);
        header = next;
    }

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
