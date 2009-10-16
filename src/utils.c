#include <string.h>
#include <stdlib.h>
#include "utils.h"

/* _hidden macro marks library private symbols. GCC can exclude them from global
 * symbols table */
#if defined(__GNUC__) && (__GNUC__ >= 4)
# define _hidden __attribute__((visibility("hidden")))
#else
#define _hidden
#endif

/* Concatenate two strings into newly allocated buffer.
 * You must free() them, when you don't need it anymore. */
_hidden char *astrcat(const char *first, const char *second) {
    size_t first_len;
    char *buf;
    
    first_len = strlen(first);
    buf = malloc(1 + first_len + strlen(second));
    if (buf) {
        strcpy(buf, first);
        strcpy(buf + first_len, second);
    }
    return buf;
}
