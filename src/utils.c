#include <string.h>
#include <stdlib.h>
#include "utils.h"

/* Concatenate two strings into newly allocated buffer.
 * You must free() them, when you don't need it anymore. */
char *astrcat(const char *first, const char *second) {
    size_t first_len = strlen(first);
    char *buf = malloc(1 + first_len + strlen(second));
    if (buf) {
        strcpy(buf, first);
        strcpy(buf + first_len, second);
    }
    return buf;
}
