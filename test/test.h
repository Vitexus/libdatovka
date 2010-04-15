#ifndef __ISDS_TEST_H
#define __ISDS_TEST_H

#define _XOPEN_SOURCE 500
#include <stdlib.h>
#include <stdio.h>
#include <locale.h>
#include <stdint.h>
#include <stdarg.h>

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

#define PASS_TEST \
    return 0;

#define FAIL_TEST(...) { \
    if (!reason) \
        test_asprintf(&reason, __VA_ARGS__); \
    return 1; \
}

#define ABORT_UNIT(message) { \
    printf("Unit %s procedure aborted: %s\n", unit_name, (message)); \
    exit(EXIT_FAILURE); \
}


#define TEST(name, function, ...) { \
    const char *message; \
    free(reason); reason = NULL; \
    int status = (function)(__VA_ARGS__); \
    if (status) { \
        failed++; \
        message = "failed"; \
    } else { \
        passed++; \
        message = "passed"; \
    } \
    printf("\t%s: %s\n", (name), message); \
    if (status) printf("\t\treason: %s\n", reason); \
    free(reason); reason = NULL; \
}

#endif
