#ifndef __ISDS_UTILS_H__
#define __ISDS_UTILS_H__

#include <stdlib.h>
#include <stdarg.h>
#include <stdio.h>
#include <time.h>

/* _hidden macro marks library private symbols. GCC can exclude them from global
 * symbols table */
#if defined(__GNUC__) && (__GNUC__ >= 4) && !defined(_WIN32)
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
 * @Returns number of bytes printed. In case of error, -1 and NULL @buffer */
int isds_asprintf(char **buffer, const char *format, ...);

/* Converts a block from charset to charset.
 * @from is input charset of @input block as known to iconv
 * @to is output charset @input will be converted to @output 
 * @input is block in @from charset/encoding of length @input_length
 * @input_length is size of @input block in bytes
 * @output is automatically allocated block of data converted from @input. No
 * NUL is apended. Can be NULL, if resulting size is 0. You must free it.
 * @return size of @output in bytes. In case of error returns (size_t) -1 and
 * deallocates @output if this function allocated it in this call. */
_hidden size_t _isds_any2any(const char *from, const char *to,
        const void *input, size_t input_length, void **output);

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

/* Convert hexadecimal digit to integer. Return negative value if character is
 * not valid hexadecimal digit. */
int _isds_hex2i(char digit);

/* Convert UTC broken time to time_t.
 * @broken_utc it time in UTC in broken format. Despite its content is not
 * touched, it'sw not-const because underlying POSIX function has non-const
 * signature.
 * @return (time_t) -1 in case of error */
time_t _isds_timegm(struct tm *broken_utc);

/* Convert size_t to int.
 * @val Value to be converted to int.
 * @return value converted to int or -1 when the supplied value is too large
 * to fit into integer. */
int _isds_sizet2int(size_t val);

/* Free() and set to NULL pointed memory */
#define zfree(memory) { \
    free(memory); \
    (memory) = NULL; \
}

#endif
