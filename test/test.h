#ifndef __ISDS_TEST_H
#define __ISDS_TEST_H

#define _XOPEN_SOURCE 500
#include <stdlib.h>
#include <stdio.h>
#include <locale.h>
#include <stdint.h>
#include <stdarg.h>
#include <string.h>

char *unit_name, *reason = NULL;
unsigned int passed, failed;

/* Print formated string into automtically reallocated @uffer.
 * @buffer automatically reallocated buffer. Must be &NULL or preallocated
 * memory.
 * @format format string as for printf(3)
 * @ap list of variadic arguments, after call will be in udefined state
 * @Returns number of bytes printed. In case of errror, -1 and NULL @buffer*/
int test_vasprintf(char **buffer, const char *format, va_list ap);


/* Print formated string into automtically reallocated @uffer.
 * @buffer automatically reallocated buffer. Must be &NULL or preallocated
 * memory.
 * @format format string as for printf(3)
 * @... variadic arguments
 * @Returns number of bytes printed. In case of errror, -1 and NULL @buffer*/
int test_asprintf(char **buffer, const char *format, ...);


/* I/O. Return 0, in case of error -1. */
int test_mmap_file(const char *file, int *fd, void **buffer, size_t *length);
int test_munmap_file(int fd, void *buffer, size_t length);

#define INIT_TEST(name) { \
    setlocale(LC_ALL, "C"); \
    unit_name = name; \
    passed = failed = 0; \
    printf("Testing unit: %s\n", unit_name); \
}

#define SUM_TEST() { \
    printf("Test results: unit = %s, passed = %u, failed = %u\n\n", \
            unit_name, passed, failed); \
    exit(failed ? EXIT_FAILURE : EXIT_SUCCESS ); \
}

#define PASS_TEST { \
    return 0; \
} \

#define FAILURE_REASON(...) { \
    if (!reason) \
        test_asprintf(&reason, __VA_ARGS__); \
}

#define FAIL_TEST(...) { \
    FAILURE_REASON(__VA_ARGS__); \
    return 1; \
}

#define ABORT_UNIT(message) { \
    printf("Unit %s procedure aborted: %s\n", unit_name, (message)); \
    exit(EXIT_FAILURE); \
}


#define TEST(name, function, ...) { \
    const char *test_message; \
    free(reason); reason = NULL; \
    int status = (function)(__VA_ARGS__); \
    if (status) { \
        failed++; \
        test_message = "failed"; \
    } else { \
        passed++; \
        test_message = "passed"; \
    } \
    printf("\t%s: %s\n", (name), test_message); \
    if (status) printf("\t\treason: %s\n", reason); \
    free(reason); reason = NULL; \
}

#define TEST_CALLOC(pointer) { \
    (pointer) = calloc(1, sizeof(*(pointer))); \
    if (!(pointer)) \
        ABORT_UNIT("No enough memory"); \
}

#define TEST_FILL_STRING(pointer) { \
    (pointer) = strdup("DATA"); \
    if (!(pointer)) \
        ABORT_UNIT("No enough memory"); \
}

#define TEST_FILL_INT(pointer) { \
    (pointer) = malloc(sizeof(*(pointer))); \
    if (!(pointer)) \
        ABORT_UNIT("No enough memory"); \
    *(pointer) = 42; \
}

#define STR(x) #x

#define TEST_STRING_DUPLICITY(x, y) { \
    if (x == NULL && y != NULL) \
        FAIL_TEST(STR(x) " is NULL, " STR(y) " is not NULL and should be"); \
    if (x != NULL && y == NULL) \
        FAIL_TEST(STR(x) " is not NULL, " STR(y) " is NULL and shouldn't be"); \
    if (x != NULL && y != NULL) { \
        if (x == y) \
            FAIL_TEST(STR(y) " is the same pointer as " STR(x)); \
        if (strcmp(x, y)) \
            FAIL_TEST(STR(y) " differs: expected=`%s' is=`%s'", x, y); \
    } \
}
    
#endif
