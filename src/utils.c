#include <string.h>
#include <stdlib.h>
#include "utils.h"

/* Concatenate two strings into newly allocated buffer.
 * You must free() them, when you don't need it anymore.
 * Any of the arguments can be NULL meaning empty string.
 * In case of error returns NULL.
 * Empty string is always returned as alloceted empty string. */
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
