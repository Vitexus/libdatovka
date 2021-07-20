#include "isds_priv.h"
#include <stdio.h>
#include <stdlib.h> /* for getenv */
#include <string.h>
#include <time.h>
#include "libdatovka/isds.h"
#include "utils.h"

static char *tz_orig; /* Copy of original TZ variable */

#if HAVE_LIBCURL
/* Convert UTF-8 @string representation of ISO 8601 date to @time.
 * XXX: Not all ISO formats are supported */
_hidden isds_error _isds_datestring2tm(const xmlChar *string, struct tm *time) {
    char *offset;
    if (!string || !time) return IE_INVAL;

    /* xsd:date is ISO 8601 string, thus ASCII */
    offset = strptime((char*)string, "%Y-%m-%d", time);
    if (offset && *offset == '\0')
        return IE_SUCCESS;

    offset = strptime((char*)string, "%Y%m%d", time);
    if (offset && *offset == '\0')
        return IE_SUCCESS;

    offset = strptime((char*)string, "%Y-%j", time);
    if (offset && *offset == '\0')
        return IE_SUCCESS;

    return IE_NOTSUP;
}
#endif

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

_hidden int64_t _isds_timegm(struct tm *broken_utc) {
    int64_t ret;

    _isds_switch_tz_to_utc();
    ret = mktime(broken_utc);
    _isds_switch_tz_to_native();

    return ret;
}
