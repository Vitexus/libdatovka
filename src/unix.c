#include "isds_priv.h"
#include <stdio.h>
#include <string.h>
#include <time.h>
#include "compiler.h" /* _hidden */
#include "libdatovka/isds.h"

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
