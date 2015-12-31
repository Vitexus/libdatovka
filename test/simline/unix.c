#ifndef _XOPEN_SOURCE
/* >= 500: strdup(3) from string.h, strptime(3) from time.h */
/* >= 600: setenv(3) */
#define _XOPEN_SOURCE 600
#endif
#include "../test-tools.h"
#include <stdlib.h>     /* for getenv(), abort() */
#include <string.h>     /* for strdup() */
#include <stdio.h>      /* for stderr */
#include <time.h>
#include "http.h"

/* PANIC macro aborts current process without any clean up.
 * Use it as last resort fatal error solution */
#define PANIC(message) { \
    if (stderr != NULL ) fprintf(stderr, \
            "SERVER PANIC (%s:%d): %s\n", __FILE__, __LINE__, (message)); \
    abort(); \
}

static char *tz_orig; /* Copy of original TZ variable */

/* Convert UTF-8 @string representation of ISO 8601 date to @time.
 * XXX: Not all ISO formats are supported */
_hidden http_error _server_datestring2tm(const char *string, struct tm *time) {
    char *offset;
    if (!string || !time) return HTTP_ERROR_SERVER;

    /* xsd:date is ISO 8601 string, thus ASCII */
    offset = strptime(string, "%Y-%m-%d", time);
    if (offset && *offset == '\0')
        return HTTP_ERROR_SUCCESS;

    offset = strptime(string, "%Y%m%d", time);
    if (offset && *offset == '\0')
        return HTTP_ERROR_SUCCESS;

    offset = strptime(string, "%Y-%j", time);
    if (offset && *offset == '\0')
        return HTTP_ERROR_SUCCESS;

    return HTTP_ERROR_SERVER;
}

/* Switches time zone to UTC.
 * XXX: This is not reentrant and not thread-safe */
static void _isds_switch_tz_to_utc(void) {
    char *tz;

    tz = getenv("TZ");
    if (tz) {
        tz_orig = strdup(tz);
        if (!tz_orig)
            PANIC("Can not back original time zone up");
    } else {
        tz_orig = NULL;
    }

    if (setenv("TZ", "", 1))
            PANIC("Can not change time zone to UTC temporarily");

    tzset();
}


/* Switches time zone to original value.
 * XXX: This is not reentrant and not thread-safe */
static void _isds_switch_tz_to_native(void) {
    if (tz_orig) {
        if (setenv("TZ", tz_orig, 1))
            PANIC("Can not restore time zone by setting TZ variable");
        free(tz_orig);
        tz_orig = NULL;
    } else {
        if(unsetenv("TZ"))
            PANIC("Can not restore time zone by unsetting TZ variable");
    }
    tzset();
}

/* Convert UTC broken time to time_t.
 * @broken_utc it time in UTC in broken format. Despite its content is not
 * touched, it'sw not-const because underlying POSIX function has non-const
 * signature.
 * @return (time_t) -1 in case of error */
_hidden time_t _isds_timegm(struct tm *broken_utc) {
    time_t ret;

    _isds_switch_tz_to_utc();
    ret = mktime(broken_utc);
    _isds_switch_tz_to_native();

    return ret;
}
