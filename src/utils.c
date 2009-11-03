#include <string.h>
#include <stdlib.h>
#include <stdio.h>
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

