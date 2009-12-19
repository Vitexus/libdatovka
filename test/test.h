#ifndef __ISDS_TEST_H
#define __ISDS_TEST_H

#define _XOPEN_SOURCE 500
#include <stdlib.h>
#include <stdio.h>
#include <locale.h>
#include <stdint.h>

char *unit_name, *reason;
unsigned int passed, failed;

#define INIT_TEST(name) { \
    setlocale(LC_ALL, "C"); \
    unit_name = name; \
    passed = failed = 0; \
    printf("Testing unit: %s\n", unit_name); \
}

#define SUM_TEST() { \
    printf("Test results: unit = %s, passed = %u, failed = %u\n", \
            unit_name, passed, failed); \
    exit(failed ? EXIT_FAILURE : EXIT_SUCCESS ); \
}

#define PASS_TEST \
    return 0;

#define FAIL_TEST(message) { \
    reason = (message); \
    return 1; \
}

#define TEST(name, function, ...) { \
    const char *message; \
    reason = NULL; \
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
}

#endif
