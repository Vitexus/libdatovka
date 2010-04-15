#ifndef __ISDS_TEST_H
#define __ISDS_TEST_H

#define _XOPEN_SOURCE 500
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <stdarg.h>


/* Print formated string into automtically reallocated @uffer.
 * @buffer automatically reallocated buffer. Must be &NULL or preallocated
 * memory.
 * @format format string as for printf(3)
 * @ap list of variadic arguments, after call will be in udefined state
 * @Returns number of bytes printed. In case of errror, -1 and NULL @buffer*/
int test_vasprintf(char **buffer, const char *format, va_list ap) {
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


/* Print formated string into automtically reallocated @uffer.
 * @buffer automatically reallocated buffer. Must be &NULL or preallocated
 * memory.
 * @format format string as for printf(3)
 * @... variadic arguments
 * @Returns number of bytes printed. In case of errror, -1 and NULL @buffer*/
int test_asprintf(char **buffer, const char *format, ...) {
    int ret;
    va_list ap;
    va_start(ap, format);
    ret = test_vasprintf(buffer, format, ap);
    va_end(ap);
    return ret;
}


#endif
