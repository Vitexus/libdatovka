#define _XOPEN_SOURCE   /* for strptime */
#include "../test-tools.h"
#include <time.h>
#include "http.h"

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
