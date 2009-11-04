#ifndef __ISDS_UTILS_H__
#define __ISDS_UTILS_H__

#include <stdarg.h>

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

/* Print formated string into automtically reallocated @uffer.
 * @buffer automatically reallocated buffer. Must be &NULL or preallocated
 * memory.
 * @format format string as for printf(3)
 * @ap list of variadic arguments, after call will be in udefined state
 * @Returns number of bytes printed. In case of errror, -1 and NULL @buffer*/
int isds_vasprintf(char **buffer, const char *format, va_list ap);

/* Converts UTF8 string into locale encoded string.
 * @utf string int UTF-8 terminated by zero byte
 * @return allocated string encoded in locale specific encoding. You must free
 * it. In case of error or NULL @utf returns NULL. */
char *utf82locale(const char *utf);

#endif
