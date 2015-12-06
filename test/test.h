#ifndef __ISDS_TEST_H
#define __ISDS_TEST_H

#ifndef _XOPEN_SOURCE
#define _XOPEN_SOURCE 700 /* strndup */
#endif
#include <stdlib.h>
#include <stdio.h>
#include <locale.h>
#include <stdint.h>
#include <inttypes.h>
#include <stdarg.h>
#include <string.h>

#include "test-tools.h"

char *unit_name, *reason = NULL;
unsigned int passed, failed;
void (*test_destructor_function)(void *) = NULL;
void *test_destructor_argument;

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

#define TEST_DESTRUCTOR(function, argument) { \
    test_destructor_function = function; \
    test_destructor_argument = argument; \
}

#define PASS_TEST { \
    if (NULL != test_destructor_function) \
        test_destructor_function(test_destructor_argument); \
    return 0; \
} \

#define FAILURE_REASON(...) { \
    if (!reason) \
        test_asprintf(&reason, __VA_ARGS__); \
}

#define FAIL_TEST(...) { \
    FAILURE_REASON(__VA_ARGS__); \
    if (NULL != test_destructor_function) \
        test_destructor_function(test_destructor_argument); \
    return 1; \
}

#define ABORT_UNIT(message) { \
    printf("Unit %s procedure aborted: %s\n", unit_name, (message)); \
    exit(EXIT_FAILURE); \
}


#define TEST(name, function, ...) { \
    const char *test_message; \
    free(reason); reason = NULL; \
    test_destructor_function = NULL; \
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

#define TEST_POINTER_IS_NULL(x) { \
    if (NULL != (x)) \
        FAIL_TEST(STR(x) " is not NULL and should be"); \
}

#define TEST_POINTER_DUPLICITY(x, y) { \
    if (x == NULL && y != NULL) \
        FAIL_TEST(STR(x) " is NULL, " STR(y) " is not NULL and should be"); \
    if (x != NULL && y == NULL) \
        FAIL_TEST(STR(x) " is not NULL, " STR(y) " is NULL and shouldn't be"); \
    if (x == y && x != NULL) \
        FAIL_TEST(STR(y) " is the same pointer as " STR(x)); \
}

#define TEST_STRING_DUPLICITY(x, y) { \
    TEST_POINTER_DUPLICITY(x, y); \
    if (x != NULL && y != NULL) { \
        if (strcmp(x, y)) \
            FAIL_TEST(STR(y) " differs: expected=`%s' is=`%s'", x, y); \
    } \
}
 
#define TEST_INT_DUPLICITY(x, y) { \
    if ((x) != (y)) \
        FAIL_TEST(STR(y) " differs: expected=%ld is=%ld", (x), (y)); \
}

#define TEST_INTPTR_DUPLICITY(x, y) { \
    TEST_POINTER_DUPLICITY(x, y); \
    if (x != NULL && y != NULL) { \
        if (*(x) != *(y)) \
            FAIL_TEST(STR(y) " differs: expected=%ld is=%ld", *(x), *(y)); \
    } \
}

#define TEST_BOOLEAN_DUPLICITY(x, y) { \
    if ((x) != (y)) \
        FAIL_TEST(STR(y) " differs: expected=%d is=%d", !!(x), !!(y)); \
}

#define TEST_BOOLEANPTR_DUPLICITY(x, y) { \
    TEST_POINTER_DUPLICITY(x, y); \
    if (x != NULL && y != NULL) { \
        if (*(x) != *(y)) \
            FAIL_TEST(STR(y) " differs: expected=%d is=%d", !!*(x), !!*(y)); \
    } \
}

#define TEST_TIMEVALPTR_DUPLICITY(x, y) { \
    TEST_POINTER_DUPLICITY(x, y); \
    if ((x) != NULL && (y) != NULL) { \
        if ((x)->tv_sec != (y)->tv_sec) \
            FAIL_TEST("struct timeval * differs in tv_sec: " \
                    "expected=%d, got=%d", (x)->tv_sec, (y)->tv_sec); \
        if ((x)->tv_usec != (y)->tv_usec) \
            FAIL_TEST("struct timeval * differs in tv_usec: " \
                    "expected=%" PRIdMAX ", got=%" PRIdMAX, (intmax_t)(x)->tv_usec, \
                    (intmax_t)(y)->tv_usec); \
    } \
}

#define TEST_TMPTR_DUPLICITY(x, y) { \
    TEST_POINTER_DUPLICITY(x, y); \
    if ((x) != NULL && (y) != NULL) { \
        if ((x)->tm_year != (y)->tm_year) \
            FAIL_TEST("struct tm * differs in tm_year: " \
                    "expected=%d, got=%d", (x)->tm_year, (y)->tm_year); \
        if ((x)->tm_mon != (y)->tm_mon) \
            FAIL_TEST("struct tm * differs in tm_mon: " \
                    "expected=%d, got=%d", (x)->tm_mon, (y)->tm_mon); \
        if ((x)->tm_mday != (y)->tm_mday) \
            FAIL_TEST("struct tm * differs in tm_mday: " \
                    "expected=%d, got=%d", (x)->tm_mday, (y)->tm_mday); \
    } \
}
#endif
