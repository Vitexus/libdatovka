#ifndef _XOPEN_SOURCE
#include "../../config.h"
#define _XOPEN_SOURCE XOPEN_SOURCE_LEVEL_FOR_STRDUP
#endif

#include "http.h"
#include "../test-tools.h"
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h> /* fprintf() */
#include <limits.h>
#include <string.h> /* strdup() */
#include <strings.h> /* strcasecmp() */
#include <stdint.h> /* int8_t */
#include <stddef.h> /* size_t, NULL */
#include <ctype.h> /* isprint() */
#include <sys/socket.h> /* MSG_NOSIGNAL for http_send_callback() */

/*
Base64 encoder is part of the libb64 project, and has been placed in the public domain.
For details, see http://sourceforge.net/projects/libb64
It's copy of ../../src/cencode.c due to symbol names.
*/


typedef enum {
    step_A, step_B, step_C
} base64_encodestep;

typedef struct {
    base64_encodestep step;
    int8_t result;
    int stepcount; /* number of encoded octet triplets on a line,
                      or -1 to for end-less line */
} base64_encodestate;

static const int CHARS_PER_LINE = 72;

/* Initialize Base64 coder.
 * @one_line is false for multi-line MIME encoding,
 * true for endless one-line format. */
static void base64_init_encodestate(base64_encodestate* state_in, _Bool one_line) {
    state_in->step = step_A;
    state_in->result = 0;
    state_in->stepcount = (one_line) ? -1 : 0;
}


static int8_t base64_encode_value(int8_t value_in) {
    /* XXX: CHAR_BIT == 8 because of <stdint.h> */
    static const int8_t encoding[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    if (value_in > 63) return '=';
    return encoding[value_in];
}


static size_t base64_encode_block(const int8_t* plaintext_in,
        size_t length_in, int8_t *code_out, base64_encodestate* state_in) {
    const int8_t *plainchar = plaintext_in;
    const int8_t* const plaintextend = plaintext_in + length_in;
    int8_t *codechar = code_out;
    int8_t result;
    int8_t fragment;

    result = state_in->result;

    switch (state_in->step) {
        while (1) {
    case step_A:
            if (plainchar == plaintextend) {
                state_in->result = result;
                state_in->step = step_A;
                return codechar - code_out;
            }
            fragment = *plainchar++;
            result = (fragment & 0x0fc) >> 2;
            *codechar++ = base64_encode_value(result);
            result = (fragment & 0x003) << 4;
    case step_B:
            if (plainchar == plaintextend) {
                state_in->result = result;
                state_in->step = step_B;
                return codechar - code_out;
            }
            fragment = *plainchar++;
            result |= (fragment & 0x0f0) >> 4;
            *codechar++ = base64_encode_value(result);
            result = (fragment & 0x00f) << 2;
    case step_C:
            if (plainchar == plaintextend) {
                state_in->result = result;
                state_in->step = step_C;
                return codechar - code_out;
            }
            fragment = *plainchar++;
            result |= (fragment & 0x0c0) >> 6;
            *codechar++ = base64_encode_value(result);
            result  = (fragment & 0x03f) >> 0;
            *codechar++ = base64_encode_value(result);

            if (state_in->stepcount >= 0) {
                ++(state_in->stepcount);
                if (state_in->stepcount == CHARS_PER_LINE/4) {
                    *codechar++ = '\n';
                    state_in->stepcount = 0;
                }
            }
        } /* while */
    } /* switch */

    /* control should not reach here */
    return codechar - code_out;
}


static size_t base64_encode_blockend(int8_t *code_out,
        base64_encodestate* state_in) {
    int8_t *codechar = code_out;

    switch (state_in->step) {
    case step_B:
        *codechar++ = base64_encode_value(state_in->result);
        *codechar++ = '=';
        *codechar++ = '=';
        break;
    case step_C:
        *codechar++ = base64_encode_value(state_in->result);
        *codechar++ = '=';
        break;
    case step_A:
        break;
    }
    if (state_in->stepcount >= 0)
        *codechar++ = '\n';

    return codechar - code_out;
}


/* Encode given data into MIME Base64 encoded zero terminated string.
 * @plain are input data (binary stream)
 * @length is length of @plain data in bytes
 * @one_line is false for multi-line MIME encoding,
 * true for endless one-line format.
 * @return allocated string of Base64 encoded plain data or NULL in case of
 * error. You must free it. */
/* TODO: Allow one-line format */
static char *base64encode(const void *plain, const size_t length,
        _Bool one_line) {

    base64_encodestate state;
    size_t code_length;
    char *buffer, *new_buffer;

    if (!plain) {
        if (length) return NULL;
        /* Empty input is valid input */
        plain = "";
    }

    base64_init_encodestate(&state, one_line);

    /* Allocate buffer
     * (4 is padding, 1 is final new line, and 1 is string terminator) */
    buffer = malloc(length * 2 + 4 + 1 + 1);
    if (!buffer) return NULL;

    /* Encode plain data */
    code_length = base64_encode_block(plain, length, (int8_t *)buffer,
            &state);
    code_length += base64_encode_blockend(((int8_t*)buffer) + code_length,
            &state);

    /* Terminate string */
    buffer[code_length++] = '\0';

    /* Shrink the buffer */
    new_buffer = realloc(buffer, code_length);
    if (new_buffer) buffer = new_buffer;

    return buffer;
}


/* Convert hexadecimal digit to integer. Return negative value if charcter is
 * not valid hexadecimal digit. */
static int hex2i(char digit) {
    if (digit >= '0' && digit <= '9')
        return digit - '0';
    if (digit >= 'a' && digit <= 'f')
        return digit - 'a' + 10;
    if (digit >= 'A' && digit <= 'F')
        return digit - 'A' + 10;
    return -1;
}


/* Decode URI-coded string.
 * @return allocated decoded string or NULL in case of error. */
static char *uri_decode(const char *coded) {
    char *plain, *p;
    int digit1, digit2;

    if (coded == NULL) return NULL;
    plain = malloc(strlen(coded) + 1);
    if (plain == NULL) return NULL;

    for (p = plain; *coded != '\0'; p++, coded++) {
        if (*coded == '%') {
            digit1 = hex2i(coded[1]);
            if (digit1 < 0) {
                free(plain);
                return NULL;
            }
            digit2 = hex2i(coded[2]);
            if (digit2< 0) {
                free(plain);
                return NULL;
            }
            *plain = (digit1 << 4) + digit2;
            coded += 2;
        } else {
            *p = *coded;
        }
    }
    *p = '\0';

    return plain;
}


/* Read a line from HTTP socket.
 * @connection is HTTP connection to read from.
 * @line is auto-reallocated just read line. Will be NULL if EOF has been
 * reached or error occurred.
 * @buffer is automatically reallocated buffer for the socket. It can preserve
 * prematurely read socket data.
 * @buffer_size is allocated size of @buffer
 * @buffer_length is size of head of the buffer that holds read data.
 * @return 0 in success. */
static int http_read_line(const struct http_connection *connection,
        char **line, char **buffer, size_t *buffer_size,
        size_t *buffer_used) {
    ssize_t got;
    char *p, *tmp;

    if (line == NULL) return HTTP_ERROR_SERVER;
    free(*line);
    *line = NULL;

    if (connection == NULL || connection->recv_callback == NULL)
        return HTTP_ERROR_SERVER;
    if (buffer == NULL || buffer_size == NULL || buffer_used == NULL)
        return HTTP_ERROR_SERVER;
    if (*buffer == NULL && *buffer_size > 0) return HTTP_ERROR_SERVER;
    if (*buffer_size < *buffer_used) return HTTP_ERROR_SERVER;

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
            if (tmp == NULL) return HTTP_ERROR_SERVER;
            memcpy(tmp, p, *buffer + *buffer_used - p);
            *line = *buffer;
            *buffer_size = BURST;
            *buffer_used = *buffer + *buffer_used - p;
            *buffer = tmp;
            /* And exit */
            return HTTP_ERROR_SUCCESS;
        }

        if (*buffer_size == *buffer_used) {
            /* Grow buffer */
            tmp = realloc(*buffer, *buffer_size + BURST);
            if (tmp == NULL) return HTTP_ERROR_SERVER;
            *buffer = tmp;
            *buffer_size += BURST;
        }

        /* Read data */
        got = connection->recv_callback(connection, *buffer + *buffer_used,
                *buffer_size - *buffer_used);
        if (got == -1) return HTTP_ERROR_CLIENT;

        /* Check for EOF */
        if (got == 0) return HTTP_ERROR_CLIENT;

        /* Move end of buffer */
        *buffer_used += got;
    }
#undef BURST

    return HTTP_ERROR_SERVER;
}


/* Write a bulk data into HTTP socket.
 * @connection is HTTP connection to write to.
 * @data are bitstream to send to client.
 * @length is size of @data in bytes.
 * @return 0 in success. */
static int http_write_bulk(const struct http_connection *connection,
        const void *data, size_t length) {
    ssize_t written;
    const void *end;

    if (connection == NULL || connection->send_callback == NULL)
        return HTTP_ERROR_SERVER;
    if (data == NULL && length > 0) return HTTP_ERROR_SERVER;

    for (end = data + length; data != end; data += written, length -= written) {
        written = connection->send_callback(connection, data, length);
        if (written == -1) return HTTP_ERROR_CLIENT;
    }

    return HTTP_ERROR_SUCCESS;
}


/* Write a line into HTTP socket.
 * @connection is HTTP connection to write to.
 * @line is NULL terminated string to send to client. HTTP EOL will be added.
 * @return 0 in success. */
static int http_write_line(const struct http_connection *connection,
        const char *line) {
    int error;
    if (line == NULL) return HTTP_ERROR_SERVER;

    fprintf(stderr, "Response: <%s>\n", line);

    /* Send the line */
    if ((error = http_write_bulk(connection, line, strlen(line))))
        return error;

    /* Send EOL */
    if ((error = http_write_bulk(connection, "\r\n", 2)))
        return error;

    return HTTP_ERROR_SUCCESS;
}


/* Read data of given length from HTTP socket.
 * @connection is HTTP connection to read from.
 * @data is auto-allocated just read data bulk. Will be NULL if EOF has been
 * reached or error occurred.
 * @data_length is size of bytes to read.
 * @buffer is automatically reallocated buffer for the socket. It can preserve
 * prematurely read socket data.
 * @buffer_size is allocated size of @buffer
 * @buffer_length is size of head of the buffer that holds read data.
 * @return 0 in success. */
static int http_read_bulk(const struct http_connection *connection,
        void **data, size_t data_length,
        char **buffer, size_t *buffer_size, size_t *buffer_used) {
    ssize_t got;
    char *tmp;

    if (data == NULL) return HTTP_ERROR_SERVER;
    *data = NULL;

    if (connection == NULL || connection->recv_callback == NULL)
        return HTTP_ERROR_SERVER;
    if (buffer == NULL || buffer_size == NULL || buffer_used == NULL)
        return HTTP_ERROR_SERVER;
    if (*buffer == NULL && *buffer_size > 0) return HTTP_ERROR_SERVER;
    if (*buffer_size < *buffer_used) return HTTP_ERROR_SERVER;

    if (data_length <= 0) return HTTP_ERROR_SUCCESS;

#define BURST 1024
    while (1) {
        /* Check whether enough data have been read */
        if (*buffer_used >= data_length) {
            /* Copy read ahead data to new buffer and point data to original
             * buffer. */
            tmp = malloc(BURST);
            if (tmp == NULL) return HTTP_ERROR_SERVER;
            memcpy(tmp, *buffer + data_length, *buffer_used - data_length);
            *data = *buffer;
            *buffer_size = BURST;
            *buffer_used = *buffer_used - data_length;
            *buffer = tmp;
            /* And exit */
            return HTTP_ERROR_SUCCESS;
        }

        if (*buffer_size == *buffer_used) {
            /* Grow buffer */
            tmp = realloc(*buffer, *buffer_size + BURST);
            if (tmp == NULL) return HTTP_ERROR_SERVER;
            *buffer = tmp;
            *buffer_size += BURST;
        }

        /* Read data */
        got = connection->recv_callback(connection, *buffer + *buffer_used,
                *buffer_size - *buffer_used);
        if (got == -1) return HTTP_ERROR_CLIENT;

        /* Check for EOF */
        if (got == 0) return HTTP_ERROR_CLIENT;

        /* Move end of buffer */
        *buffer_used += got;
    }
#undef BURST

    return HTTP_ERROR_SERVER;
}


/* Parse HTTP request header.
 * @request is pre-allocated HTTP request.
 * @return 0 if success. */
static int http_parse_request_header(char *line,
        struct http_request *request) {
    char *p;

    fprintf(stderr, "Request: <%s>\n", line);

    /* Get method */
    p = strchr(line, ' ');
    if (p == NULL) return HTTP_ERROR_SERVER;
    *p = '\0';
    if (!strcmp(line, "GET"))
        request->method = HTTP_METHOD_GET;
    else if (!strcmp(line, "POST"))
        request->method = HTTP_METHOD_POST;
    else
        request->method = HTTP_METHOD_UNKNOWN;
    line = p + 1;

    /* Get URI */
    p = strchr(line, ' ');
    if (p != NULL) *p = '\0';
    request->uri = uri_decode(line);
    if (request->uri == NULL) return HTTP_ERROR_SERVER;

    /* Do not care about HTTP version */

    fprintf(stderr, "Request-URI: <%s>\n", request->uri);
    return HTTP_ERROR_SUCCESS;
}


/* Send HTTP response status line to client.
 * @return 0 if success. */
static int http_write_response_status(const struct http_connection *connection,
        const struct http_response *response) {
    char *buffer = NULL;
    int error;

    if (response == NULL) return HTTP_ERROR_SERVER;

    if (-1 == test_asprintf(&buffer, "HTTP/1.0 %u %s", response->status,
                (response->reason == NULL) ? "" : response->reason))
        return HTTP_ERROR_SERVER;
    error = http_write_line(connection, buffer);
    free(buffer);

    return error;
}


/* Parse generic HTTP header.
 * @request is pre-allocated HTTP request.
 * @return 0 if success, negative value if internal error, positive value if
 * header is not valid. */
static int http_parse_header(char *line, struct http_request *request) {
    struct http_header *header;

    if (line == NULL || request == NULL) return HTTP_ERROR_SERVER;

    fprintf(stderr, "Header: <%s>\n", line);

    /* Find last used header */
    for (header = request->headers; header != NULL && header->next != NULL;
            header = header->next);

    if (*line == ' ' || *line == '\t') {
        /* Line is continuation of last header */
        if (header == NULL)
            return HTTP_ERROR_CLIENT;   /* No previous header to continue */
        line++;
        size_t old_length = strlen(header->value);
        char *tmp = realloc(header->value,
                sizeof(header->value[0]) * (old_length + strlen(line) + 1));
        if (tmp == NULL) return HTTP_ERROR_SERVER;
        header->value = tmp;
        strcpy(&header->value[old_length], line);
    } else {
        /* New header */
        struct http_header *new_header = calloc(sizeof(*new_header), 1);
        if (new_header == NULL) return HTTP_ERROR_SERVER;

        char *p = strstr(line, ": ");
        if (p == NULL) {
            http_header_free(&new_header);
            return HTTP_ERROR_CLIENT;
        }

        size_t length = p - line;
        new_header->name = malloc(sizeof(line[0]) * (length + 1));
        if (new_header->name == NULL) {
            http_header_free(&new_header);
            return HTTP_ERROR_SERVER;
        }
        strncpy(new_header->name, line, length);
        new_header->name[length] = '\0';

        p += 2;
        length = strlen(p);
        new_header->value = malloc(sizeof(p[0]) * (length + 1));
        if (new_header->value == NULL) {
            http_header_free(&new_header);
            return HTTP_ERROR_SERVER;
        }
        strcpy(new_header->value, p);

        if (request->headers == NULL)
            request->headers = new_header;
        else
            header->next = new_header;
    }


    /* FIXME: Decode. After parsing all headers as we could decode beginning
     * and then got encoded continuation. */
    return HTTP_ERROR_SUCCESS;
}


/* Send HTTP header to client.
 * @return 0 if success. */
static int http_write_header(const struct http_connection *connection,
        const struct http_header *header) {
    char *buffer = NULL;
    int error;

    if (header == NULL) return HTTP_ERROR_SERVER;

    if (header->name == NULL) return HTTP_ERROR_SERVER;

    /* TODO: Quote, split long lines */
    if (-1 == test_asprintf(&buffer, "%s: %s", header->name,
                (header->value == NULL) ? "" : header->value))
        return HTTP_ERROR_SERVER;
    error = http_write_line(connection, buffer);
    free(buffer);

    return error;
}


/* Find Content-Length value in HTTP request headers and set it into @request.
 * @return 0 in success. */
static int find_content_length(struct http_request *request) {
    struct http_header *header;
    if (request == NULL) return HTTP_ERROR_SERVER;

    for (header = request->headers; header != NULL; header = header->next) {
        if (header->name == NULL) continue;
        if (!strcasecmp(header->name, "Content-Length")) break;
    }

    if (header != NULL && header->value != NULL) {
        char *p;
        long long int value = strtoll(header->value, &p, 10);
        if (*p != '\0')
            return HTTP_ERROR_CLIENT;
        if ((value == LLONG_MIN || value == LLONG_MAX) && errno == ERANGE)
            return HTTP_ERROR_SERVER;
        if (value < 0)
            return HTTP_ERROR_CLIENT;
        if ((unsigned long long)value > SIZE_MAX)
            return HTTP_ERROR_SERVER;
        request->body_length = value;
    } else {
        request->body_length = 0;
    }
    return HTTP_ERROR_SUCCESS;
}


/* Print binary stream to STDERR with some escapes */
static void dump_body(const void *data, size_t length) {
    fprintf(stderr, "===BEGIN BODY===\n");
    if (length > 0 && NULL != data) {
        for (size_t i = 0; i < length; i++) {
            if (isprint(((const unsigned char *)data)[i]))
                fprintf(stderr, "%c", ((const unsigned char *)data)[i]);
            else
                fprintf(stderr, "\\x%02x", ((const unsigned char*)data)[i]);
        }
        fprintf(stderr, "\n");
    }
    fprintf(stderr, "===END BODY===\n");
}


/* Read a HTTP request from connection.
 * @http_request is heap-allocated received HTTP request,
 * or NULL in case of error.
 * @return http_error code. */
http_error http_read_request(const struct http_connection *connection,
        struct http_request **request) {
    char *line = NULL;
    char *buffer = NULL;
    size_t buffer_size = 0, buffer_used = 0;
    int error;

    if (request == NULL) return HTTP_ERROR_SERVER;

    *request = calloc(1, sizeof(**request));
    if (*request == NULL) return HTTP_ERROR_SERVER;

    /* Get request header */
    if ((error = http_read_line(connection, &line, &buffer, &buffer_size,
                &buffer_used)))
        goto leave;
    if ((error = http_parse_request_header(line, *request)))
        goto leave;

    /* Get other headers */
    while (1) {
        if ((error = http_read_line(connection, &line, &buffer, &buffer_size,
                    &buffer_used))) {
            fprintf(stderr, "Error while reading HTTP request line\n");
            goto leave;
        }

        /* Check for headers delimiter */
        if (line == NULL || *line == '\0') {
            break;
        }

        if ((error = http_parse_header(line, *request))) {
            fprintf(stderr, "Error while parsing HTTP request line: <%s>\n",
                    line);
            goto leave;
        }
    };

    /* Get body */
    if ((error = find_content_length(*request))) {
        fprintf(stderr, "Could not determine length of body\n");
        goto leave;
    }
    if ((error = http_read_bulk(connection,
                    &(*request)->body, (*request)->body_length,
                    &buffer, &buffer_size, &buffer_used))) {
        fprintf(stderr, "Could not read request body\n");
        goto leave;
    }
    fprintf(stderr, "Body of size %zu B has been received:\n",
            (*request)->body_length);
    dump_body((*request)->body, (*request)->body_length);

leave:
    free(line);
    free(buffer);
    if (error) http_request_free(request);
    return error;
}


/* Write a HTTP response to connection. Auto-add Content-Length header.
 * @return 0 in case of success. */
int http_write_response(const struct http_connection *connection,
        const struct http_response *response) {
    char *buffer = NULL;
    int error = -1;

    if (response == NULL) return HTTP_ERROR_SERVER;

    /* Status line */
    error = http_write_response_status(connection, response);
    if (error) return error;

    /* Headers */
    for (struct http_header *header = response->headers; header != NULL;
            header = header->next) {
        error = http_write_header(connection, header);
        if (error) return error;
    }
    if (-1 == test_asprintf(&buffer, "Content-Length: %u",
                response->body_length))
        return HTTP_ERROR_SERVER;
    error = http_write_line(connection, buffer);
    if (error) return error;

    /* Headers trailer */
    error = http_write_line(connection, "");
    if (error) return error;

    /* Body */
    if (response->body_length > 0) {
        error = http_write_bulk(connection, response->body,
                response->body_length);
        if (error) return error;
    }
    fprintf(stderr, "Body of size %zu B has been sent:\n",
            response->body_length);
    dump_body(response->body, response->body_length);

    free(buffer);
    return HTTP_ERROR_SUCCESS;
}


/* Build Set-Cookie header. In case of error, return NULL. Caller must free
 * the header. */
static struct http_header *http_build_setcookie_header (
        const char *cokie_name, const char *cookie_value,
        const char *cookie_domain, const char *cookie_path) {
    struct http_header *header = NULL;
    char *domain_parameter = NULL;
    char *path_parameter = NULL;

    if (cokie_name == NULL) goto error;

    header = calloc(1, sizeof(*header));
    if (header == NULL) goto error;

    header->name = strdup("Set-Cookie");
    if (header->name == NULL) goto error;

    if (cookie_domain != NULL)
        if (-1 == test_asprintf(&domain_parameter, "; Domain=%s",
                    cookie_domain))
            goto error;

    if (cookie_path != NULL)
        if (-1 == test_asprintf(&path_parameter, "; Path=%s",
                    cookie_path))
            goto error;

    if (-1 == test_asprintf(&header->value, "%s=%s%s%s",
                cokie_name,
                (cookie_value == NULL) ? "" : cookie_value,
                (domain_parameter == NULL) ? "": domain_parameter,
                (path_parameter == NULL) ? "": path_parameter))
        goto error;
    goto ok;

error:
    http_header_free(&header);
ok:
    free(domain_parameter);
    free(path_parameter);
    return header;
}


/* Send a 200 Ok response with a cookie */
int http_send_response_200_cookie(const struct http_connection *connection,
        const char *cokie_name, const char *cookie_value,
        const char *cookie_domain, const char *cookie_path,
        const void *body, size_t body_length, const char *type) {
    int retval;
    struct http_header *header_cookie = NULL;
    struct http_header header_contenttype = {
        .name = "Content-Type",
        .value = (char *)type,
        .next = NULL
    };
    struct http_response response = {
        .status = 200,
        .reason = "OK",
        .headers = NULL,
        .body_length = body_length,
        .body = (void *)body
    };

    if (cokie_name != NULL) {
        if (NULL == (header_cookie = http_build_setcookie_header(
                        cokie_name, cookie_value, cookie_domain, cookie_path)))
            return http_send_response_500(connection,
                    "Could not build Set-Cookie header");
    }

    /* Link defined headers */
    if (type != NULL) {
        response.headers = &header_contenttype;
    }
    if (header_cookie != NULL) {
        header_cookie->next = response.headers;
        response.headers = header_cookie;
    }

    retval = http_write_response(connection, &response);
    http_header_free(&header_cookie);
    return retval;
}


/* Send a 200 Ok response */
int http_send_response_200(const struct http_connection *connection,
        const void *body, size_t body_length, const char *type) {
    return http_send_response_200_cookie(connection,
            NULL, NULL, NULL, NULL,
            body, body_length, type);
}


/* Send a 302 Found response setting a cookie */
int http_send_response_302_cookie(const struct http_connection *connection,
        const char *cokie_name, const char *cookie_value,
        const char *cookie_domain, const char *cookie_path,
        const char *location) {
    int retval;
    struct http_header *header_cookie = NULL;
    struct http_header header_location = {
        .name = "Location",
        .value = (char *) location,
        .next = NULL
    };
    struct http_response response = {
        .status = 302,
        .reason = "Found",
        .headers = NULL,
        .body_length = 0,
        .body = NULL
    };

    if (cokie_name != NULL) {
        if (NULL == (header_cookie = http_build_setcookie_header(
                        cokie_name, cookie_value, cookie_domain, cookie_path)))
            return http_send_response_500(connection,
                    "Could not build Set-Cookie header");
    }

    /* Link defined headers */
    if (location != NULL) {
        response.headers = &header_location;
    }
    if (header_cookie != NULL) {
        header_cookie->next = response.headers;
        response.headers = header_cookie;
    }

    retval = http_write_response(connection, &response);
    http_header_free(&header_cookie);
    return retval;
}


/* Send a 302 Found response with totp authentication scheme header */
int http_send_response_302_totp(const struct http_connection *connection,
        const char *code, const char *text, const char *location) {
    struct http_header header_code = {
        .name = "X-Response-message-code",
        .value = (char *) code,
        .next = NULL
    };
    struct http_header header_text = {
        .name = "X-Response-message-text",
        .value = (char *) text,
        .next = NULL
    };
    struct http_header header_location = {
        .name = "Location",
        .value = (char *) location,
        .next = NULL
    };
    struct http_response response = {
        .status = 302,
        .reason = "Found",
        .headers = NULL,
        .body_length = 0,
        .body = NULL
    };

    /* Link defined headers */
    if (location != NULL) {
        response.headers = &header_location;
    }
    if (text != NULL) {
        header_text.next = response.headers;
        response.headers = &header_text;
    }
    if (code != NULL) {
        header_code.next = response.headers;
        response.headers = &header_code;
    }

    return http_write_response(connection, &response);
}


/* Send a 400 Bad Request response.
 * Use non-NULL @reason to override status message. */
int http_send_response_400(const struct http_connection *connection,
        const char *reason) {
    struct http_response response = {
        .status = 400,
        .reason = (reason == NULL) ? "Bad Request" : (char *) reason,
        .headers = NULL,
        .body_length = 0,
        .body = NULL
    };

    return http_write_response(connection, &response);
}


/* Send a 401 Unauthorized response with Basic authentication scheme header */
int http_send_response_401_basic(const struct http_connection *connection) {
    struct http_header header = {
        .name = "WWW-Authenticate",
        .value = "Basic realm=\"SimulatedISDSServer\"",
        .next = NULL
    };
    struct http_response response = {
        .status = 401,
        .reason = "Unauthorized",
        .headers = &header,
        .body_length = 0,
        .body = NULL
    };

    return http_write_response(connection, &response);
}


/* Send a 401 Unauthorized response with OTP authentication scheme header for
 * given @method. */
int http_send_response_401_otp(const struct http_connection *connection,
        const char *method, const char *code, const char *text) {
    int retval;
    struct http_header header = {
        .name = "WWW-Authenticate",
        .value = NULL,
        .next = NULL
    };
    struct http_header header_code = {
        .name = "X-Response-message-code",
        .value = (char *) code,
        .next = NULL
    };
    struct http_header header_text = {
        .name = "X-Response-message-text",
        .value = (char *) text,
        .next = NULL
    };
    struct http_response response = {
        .status = 401,
        .reason = "Unauthorized",
        .headers = &header,
        .body_length = 0,
        .body = NULL
    };

    if (-1 == test_asprintf(&header.value, "%s realm=\"SimulatedISDSServer\"",
                method)) {
        return http_send_response_500(connection,
                "Could not build WWW-Authenticate header value");
    }

    /* Link defined headers */
    if (code != NULL) header.next = &header_code;
    if (text != NULL) {
        if (code != NULL)
            header_code.next = &header_text;
        else
            header.next = &header_text;
    }

    retval = http_write_response(connection, &response);
    free(header.value);
    return retval;
}


/* Send a 403 Forbidden response */
int http_send_response_403(const struct http_connection *connection) {
    struct http_response response = {
        .status = 403,
        .reason = "Forbidden",
        .headers = NULL,
        .body_length = 0,
        .body = NULL
    };

    return http_write_response(connection, &response);
}


/* Send a 500 Internal Server Error response.
 * Use non-NULL @reason to override status message. */
int http_send_response_500(const struct http_connection *connection,
        const char *reason) {
    struct http_response response = {
        .status = 500,
        .reason = (NULL == reason) ? "Internal Server Error" : (char *) reason,
        .headers = NULL,
        .body_length = 0,
        .body = NULL
    };

    return http_write_response(connection, &response);
}


/* Send a 503 Service Temporarily Unavailable response */
int http_send_response_503(const struct http_connection *connection,
        const void *body, size_t body_length, const char *type) {
    struct http_header header = {
        .name = "Content-Type",
        .value = (char *)type
    };
    struct http_response response = {
        .status = 503,
        .reason = "Service Temporarily Unavailable",
        .headers = (type == NULL) ? NULL : &header,
        .body_length = body_length,
        .body = (void *)body
    };

    return http_write_response(connection, &response);
}


/* Returns Authorization header in given request */
static const struct http_header *find_authorization(
        const struct http_request *request) {
    const struct http_header *header;
    if (request == NULL) return NULL;
    for (header = request->headers; header != NULL; header = header->next) {
        if (header->name != NULL &&
                !strcasecmp(header->name, "Authorization"))
            break;
    }
    return header;
}


/* Return true if request carries Authorization header */
int http_client_authenticates(const struct http_request *request) {
    if (find_authorization(request))
        return 1;
    else
        return 0;
}


/* Return HTTP_ERROR_SUCCESS if request carries valid Basic credentials.
 * NULL @username or @password equals to empty string. */
http_error http_authenticate_basic(const struct http_request *request,
        const char *username, const char *password) {
    const struct http_header *header;
    const char *basic_cookie_client;
    char *basic_cookie_plain = NULL, *basic_cookie_encoded = NULL;
    _Bool is_valid;

    header = find_authorization(request);
    if (header == NULL) return HTTP_ERROR_CLIENT;

    if (strncmp(header->value, "Basic ", 6)) return HTTP_ERROR_CLIENT;
    basic_cookie_client = header->value + 6;

    if (-1 == test_asprintf(&basic_cookie_plain, "%s:%s",
                (username == NULL) ? "" : username,
                (password == NULL) ? "" : password)) {
        return HTTP_ERROR_SERVER;
    }

    basic_cookie_encoded = base64encode(basic_cookie_plain,
            strlen(basic_cookie_plain), 1);
    if (basic_cookie_encoded == NULL) {
        free(basic_cookie_plain);
        return HTTP_ERROR_SERVER;
    }

    fprintf(stderr, "Authenticating basic: got=<%s>, expected=<%s> (%s)\n",
            basic_cookie_client, basic_cookie_encoded, basic_cookie_plain);
    free(basic_cookie_plain);

    is_valid = !strcmp(basic_cookie_encoded, basic_cookie_client);
    free(basic_cookie_encoded);

    return (is_valid) ? HTTP_ERROR_SUCCESS : HTTP_ERROR_CLIENT;
}


/* Return HTTP_ERROR_SUCCESS if request carries valid OTP credentials.
 * NULL @username or @password or @otp equal to empty string. */
http_error http_authenticate_otp(const struct http_request *request,
        const char *username, const char *password, const char *otp) {
    char *basic_password = NULL;
    http_error retval;

    /* Concatenate password and OTP code */
    if (-1 == test_asprintf(&basic_password, "%s%s",
                (password == NULL) ? "": password,
                (otp == NULL) ? "" : otp)) {
        return HTTP_ERROR_SERVER;
    }

    /* Use Basic authentication */
    /* XXX: Specification does not define authorization method string */
    retval = http_authenticate_basic(request, username, basic_password);

    free(basic_password);
    return retval;
}


/* Return cookie value by name or NULL if does not present. */
const char *http_find_cookie(const struct http_request *request,
        const char *name) {
    const struct http_header *header;
    size_t length;
    const char *value = NULL;

    if (request == NULL || name == NULL) return NULL;
    length = strlen(name);

    for (header = request->headers; header != NULL; header = header->next) {
        if (header->name != NULL && !strcasecmp(header->name, "Cookie") &&
                header->value != NULL) {
            if (!strncmp(header->value, name, length) &&
                    header->value[length] == '=') {
                /* Return last cookie with the name */
                value = header->value + length + 1;
            }
        }
    }
    return value;
}


/* Return Host header value or NULL if does not present. Returned string is
 * statically allocated. */
const char *http_find_host(const struct http_request *request) {
    const struct http_header *header;
    const char *value = NULL;

    if (request == NULL) return NULL;

    for (header = request->headers; header != NULL; header = header->next) {
        if (header->name != NULL && !strcmp(header->name, "Host")) {
                value = header->value;
        }
    }
    return value;
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

/* Free HTTP response and set it to NULL */
void http_response_free(struct http_response **response) {
    if (response == NULL || *response == NULL) return;
    free((*response)->reason);
    http_headers_free(&((*response)->headers));
    free((*response)->body);
    free(*response);
    *response = NULL;
}
