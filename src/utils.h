#ifndef __ISDS_UTILS_H__
#define __ISDS_UTILS_H__

#include <stdlib.h>
#include <stdarg.h>

/* _hidden macro marks library private symbols. GCC can exclude them from global
 * symbols table */
#if defined(__GNUC__) && (__GNUC__ >= 4)
#define _hidden __attribute__((visibility("hidden")))
#else
#define _hidden
#endif

/* PANIC macro aborts current proces without any clean up.
 * Use it as last resort fatal error solution */
#define PANIC(message) { \
    if (stderr != NULL ) fprintf(stderr, \
            "LIBISDS PANIC (%s:%d): %s\n", __FILE__, __LINE__, (message)); \
    abort(); \
}

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

/* Print formated string into automtically reallocated @uffer.
 * @buffer automatically reallocated buffer. Must be &NULL or preallocated
 * memory.
 * @format format string as for printf(3)
 * @... variadic arguments
 * @Returns number of bytes printed. In case of errror, -1 and NULL @buffer*/
int isds_asprintf(char **buffer, const char *format, ...);

/* Converts UTF8 string into locale encoded string.
 * @utf string int UTF-8 terminated by zero byte
 * @return allocated string encoded in locale specific encoding. You must free
 * it. In case of error or NULL @utf returns NULL. */
char *utf82locale(const char *utf);

/* Encode given data into MIME Base64 encoded zero terminated string.
 * @plain are input data (binary stream)
 * @length is liength of @plain data in bytes
 * @return allocated string of base64 encoded plain data or NULL in case of
 * error. You must free it. */
char *b64encode(const void *plain, const size_t length);

/* Switches time zone to UTC.
 * XXX: This is not reentrant and not thread-safe */
_hidden void switch_tz_to_utc(void);

/* Switches time zone to original value.
 * XXX: This is not reentrant and not thread-safe */
_hidden void switch_tz_to_native(void);

#endif
