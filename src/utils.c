#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <iconv.h>
#include <langinfo.h>
#include "utils.h"

/* Concatenate two strings into newly allocated buffer.
 * You must free() them, when you don't need it anymore.
 * Any of the arguments can be NULL meaning empty string.
 * In case of error returns NULL.
 * Empty string is always returned as allocated empty string. */
_hidden char *astrcat(const char *first, const char *second) {
    size_t first_len, second_len;
    char *buf;
    
    first_len = (first) ? strlen(first) : 0;
    second_len = (second) ? strlen(second) : 0;
    buf = malloc(1 + first_len + second_len);
    if (buf) {
        buf[0] = '\0';
        if (first) strcpy(buf, first);
        if (second) strcpy(buf + first_len, second);
    }
    return buf;
}


/* Concatenate three strings into newly allocated buffer.
 * You must free() them, when you don't need it anymore.
 * Any of the arguments can be NULL meaning empty string.
 * In case of error returns NULL.
 * Empty string is always returned as allocated empty string. */
_hidden char *astrcat3(const char *first, const char *second,
        const char *third) {
    size_t first_len, second_len, third_len;
    char *buf, *next;
    
    first_len = (first) ? strlen(first) : 0;
    second_len = (second) ? strlen(second) : 0;
    third_len = (third) ? strlen(third) : 0;
    buf = malloc(1 + first_len + second_len + third_len);
    if (buf) {
        buf[0] = '\0';
        next = buf;
        if (first) {
            strcpy(next, first);
            next += first_len;
        }
        if (second) {
            strcpy(next, second);
            next += second_len;
        }
        if (third) {
            strcpy(next, third);
        }
    }
    return buf;
}


/* Print formated string into automtically reallocated @uffer.
 * @buffer automatically reallocated buffer. Must be &NULL or preallocated
 * memory.
 * @format format string as for printf(3)
 * @ap list of variadic arguments, after call will be in udefined state
 * @Returns number of bytes printed. In case of errror, -1 and NULL @buffer*/
_hidden int isds_vasprintf(char **buffer, const char *format, va_list ap) {
    va_list aq;
    int length, new_length;
    char *new_buffer;

    if (!buffer || !format) {
        if (buffer) {
            free(*buffer);
            *buffer = NULL;
        }
        return -1;
    }

    va_copy(aq, ap);
    length = vsnprintf(NULL, 0, format, aq) + 1;
    va_end(aq);
    if (length <= 0) {
        free(*buffer);
        *buffer = NULL;
        return -1;
    }

    new_buffer = realloc(*buffer, length);
    if (!new_buffer) {
        free(*buffer);
        *buffer = NULL;
        return -1;
    }
    *buffer = new_buffer;

    new_length = vsnprintf(*buffer, length, format, ap);
    if (new_length >= length) {
        free(*buffer);
        *buffer = NULL;
        return -1;
    }

    return new_length;
}


/* Converts UTF8 string into locale encoded string.
 * @utf string int UTF-8 terminated by zero byte
 * @return allocated string encoded in locale specific encoding. You must free
 * it. In case of error or NULL @utf returns NULL. */
char *utf82locale(const char *utf) {
    iconv_t state;
    size_t utf_length;
    char *buffer = NULL, *new_buffer;
    size_t buffer_length = 0, buffer_used = 0;
    char *inbuf, *outbuf;
    size_t inleft, outleft;

    if (!utf) return NULL;

    /* nl_langinfo() is not thread-safe */
    state = iconv_open(nl_langinfo(CODESET), "UTF-8");
    if (state == (iconv_t) -1) return NULL;

    /* Get the initial ouput buffer length */
    utf_length = strlen(utf);
    buffer_length = utf_length + 1;

    inbuf = (char *) utf;
    inleft = utf_length + 1;

    while (inleft > 0) {
        /* Extend buffer */
        new_buffer = realloc(buffer, buffer_length);
        if (!buffer) {
            free(buffer);
            buffer = NULL;
            goto leave;
        }
        buffer = new_buffer;

        /* FIXME */
        outbuf = buffer + buffer_used;
        outleft = buffer_length - buffer_used;
        
        /* Convert chunk of data */
        if ((size_t) -1 == iconv(state, &inbuf, &inleft, &outbuf, &outleft)) {
            free(buffer);
            buffer = NULL;
            goto leave;
        }

        /* Update positions */
        buffer_length += 1024;
        buffer_used = outbuf - buffer;
    }

leave:
    iconv_close(state);
    return buffer;
}

