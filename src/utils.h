#ifndef __ISDS_UTILS_H__
#define __ISDS_UTILS_H__

#include <stdlib.h>
#include <stdarg.h>
#include <stdio.h>
#include <time.h>

/* _hidden macro marks library private symbols. GCC can exclude them from global
 * symbols table */
#if defined(__GNUC__) && (__GNUC__ >= 4)
#define _hidden __attribute__((visibility("hidden")))
#else
#define _hidden
#endif

/* PANIC macro aborts current process without any clean up.
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
char *_isds_astrcat(const char *first, const char *second);

/* Concatenate three strings into newly allocated buffer.
 * You must free() them, when you don't need it anymore.
 * Any of the arguments can be NULL meaning empty string.
 * In case of error returns NULL.
 * Empty string is always returned as allocated empty string. */
char *_isds_astrcat3(const char *first, const char *second,
        const char *third);

/* Print formatted string into automatically reallocated @buffer.
 * @buffer automatically reallocated buffer. Must be &NULL or preallocated
 * memory.
 * @format format string as for printf(3)
 * @ap list of variadic arguments, after call will be in undefined state
 * @Returns number of bytes printed. In case of error, -1 and NULL @buffer*/
int isds_vasprintf(char **buffer, const char *format, va_list ap);

/* Print formatted string into automatically reallocated @buffer.
 * @buffer automatically reallocated buffer. Must be &NULL or preallocated
 * memory.
 * @format format string as for printf(3)
 * @... variadic arguments
 * @Returns number of bytes printed. In case of error, -1 and NULL @buffer*/
int isds_asprintf(char **buffer, const char *format, ...);

/* Converts UTF8 string into locale encoded string.
 * @utf string int UTF-8 terminated by zero byte
 * @return allocated string encoded in locale specific encoding. You must free
 * it. In case of error or NULL @utf returns NULL. */
char *_isds_utf82locale(const char *utf);

/* Encode given data into MIME Base64 encoded zero terminated string.
 * @plain are input data (binary stream)
 * @length is length of @plain data in bytes
 * @return allocated string of base64 encoded plain data or NULL in case of
 * error. You must free it. */
char *_isds_b64encode(const void *plain, const size_t length);

/* Decode given data from MIME Base64 encoded zero terminated string to binary
 * stream. Invalid Base64 symbols are skipped.
 * @encoded are input data (Base64 zero terminated string)
 * @plain are automatically reallocated output data (binary stream). You must
 * free it. Will be freed in case of error.
 * @return length of @plain data in bytes or (size_t) -1 in case of memory
 * allocation failure. */
size_t _isds_b64decode(const char *encoded, void **plain);

/* Convert UTC broken time to time_t.
 * @broken_utc it time in UTC in broken format. Despite its content is not
 * touched, it'sw not-const because underlying POSIX function has non-const
 * signature.
 * @return (time_t) -1 in case of error */
time_t _isds_timegm(struct tm *broken_utc);

/* Free() and set to NULL pointed memory */
#define zfree(memory) { \
    free(memory); \
    (memory) = NULL; \
}

#endif
