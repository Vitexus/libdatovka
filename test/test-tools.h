#ifndef __ISDS_TEST_TOOLS_H
#define __ISDS_TEST_TOOLS_H

#include <stdarg.h> /* va_list */
#include <stddef.h> /* size_t, NULL */

/* _hidden macro marks library private symbols. GCC can exclude them from
 * global
 *  * symbols table */
#if defined(__GNUC__) && (__GNUC__ >= 4) && !defined(_WIN32)
#define _hidden __attribute__((visibility("hidden")))
#else
#define _hidden
#endif

/* Print formatted string into automatically reallocated @buffer.
 * @buffer automatically reallocated buffer. Must be &NULL or preallocated
 * memory.
 * @format format string as for printf(3)
 * @ap list of variadic arguments, after call will be in undefined state
 * @Returns number of bytes printed. In case of error, -1 and NULL @buffer*/
int test_vasprintf(char **buffer, const char *format, va_list ap);


/* Print formatted string into automatically reallocated @buffer.
 * @buffer automatically reallocated buffer. Must be &NULL or preallocated
 * memory.
 * @format format string as for printf(3)
 * @... variadic arguments
 * @Returns number of bytes printed. In case of error, -1 and NULL @buffer*/
int test_asprintf(char **buffer, const char *format, ...);


/* I/O. Return 0, in case of error -1. */
int test_mmap_file(const char *file, int *fd, void **buffer, size_t *length);
int test_munmap_file(int fd, void *buffer, size_t length);

#endif
