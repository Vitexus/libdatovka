#ifndef __ISDS_UTILS_H__
#define __ISDS_UTILS_H__

/* _hidden macro marks library private symbols. GCC can exclude them from global
 * symbols table */
#if defined(__GNUC__) && (__GNUC__ >= 4)
#define _hidden __attribute__((visibility("hidden")))
#else
#define _hidden
#endif

/* Concatenate two strings into newly allocated buffer.
 * You must free() them, when you don't need it anymore.
 * Any of the arguments can be NULL meaning empty string.
 * In case of error returns NULL.
 * Empty string is always returned as allocated empty string. */
char *astrcat(const char *first, const char *second);

/* Concatenate three strings into newly allocated buffer.
 * You must free() them, when you don't need it anymore.
 * Any of the arguments can be NULL meaning empty string.
 * In case of error returns NULL.
 * Empty string is always returned as allocated empty string. */
char *astrcat3(const char *first, const char *second,
        const char *third);
#endif
