#include <stdio.h>
#include <string.h>
#include "libdatovka/isds.h"
#include "isds_priv.h"
#include "utils.h"
#include "win32.h"

#if HAVE_LIBCURL
static void yday2mday(struct tm *time) {
    static int mtab[12] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
    int i, year = 1900 + time->tm_year;

    mtab[1] = (!(year % 4) && ((year % 100) || !(year %400))) ? 29 : 28;
    time->tm_mday = time->tm_yday + 1;

    for (i = 0; i < 12; i++) {
        if (time->tm_mday > mtab[i]) {
            time->tm_mday -= mtab[i];
        } else {
            break;
        }
    }

    time->tm_mon = i;
}

/* Convert UTF-8 @string representation of ISO 8601 date to @time.
 * XXX: Not all ISO formats are supported */
isds_error _isds_datestring2tm(const xmlChar *string, struct tm *time) {
    char *ptr;
    int len, tmp;
    if (!string || !time) return IE_INVAL;

    memset(time, 0, sizeof(*time));

    if (sscanf((const char*)string, "%d-%d-%d%n",
                &time->tm_year, &time->tm_mon, &time->tm_mday, &tmp) >= 3
            && tmp == strlen((const char*)string)) {
        time->tm_mon--;
        time->tm_year -= 1900;
            return IE_SUCCESS;
    }

    memset(time, 0, sizeof(*time));

    if (sscanf((const char*)string, "%d-%d%n",
                &time->tm_year, &time->tm_yday, &tmp) >= 2
            && tmp == strlen((const char*)string)) {
        time->tm_yday--;
        time->tm_year -= 1900;
        yday2mday(time);
            return IE_SUCCESS;
    }

    memset(time, 0, sizeof(*time));
    len = strlen((const char*)string);

    if (len < 4) {
        return IE_NOTSUP;
    }

    ptr = strdup((const char*)string);

    if (sscanf(ptr + len - 2, "%d%n", &time->tm_mday, &tmp) < 1 || tmp < 2) {
        free(ptr);
        return IE_NOTSUP;
    }

    ptr[len - 2] = '\0';

    if (sscanf(ptr + len - 4, "%d%n", &time->tm_mon, &tmp) < 1 || tmp < 2) {
        free(ptr);
        return IE_NOTSUP;
    }

    ptr[len - 4] = '\0';

    if (sscanf(ptr, "%d%n", &time->tm_year, &tmp) < 1 || tmp < len - 4) {
        free(ptr);
        return IE_NOTSUP;
    }

    free(ptr);
    time->tm_mon--;
    time->tm_year -= 1900;
    return IE_SUCCESS;
}
#endif

/* MSVCRT gmtime() uses thread-local buffer. This is reentrant. */
_hidden struct tm *gmtime_r(const time_t *timep, struct tm *result) {
    struct tm *buf;

    buf = gmtime(timep);

    if (!buf) {
        return NULL;
    }

    memcpy(result, buf, sizeof(struct tm));
    return result;
}

_hidden char *strndup(const char *s, size_t n) {
    char *ret;
    size_t len;

    len = strlen(s);
    len = len > n ? n : len;
    ret = malloc((len + 1) * sizeof(char));

    if (!ret) {
        return NULL;
    }

    strncpy(ret, s, len);
    ret[len] = '\0';
    return ret;
}

ssize_t getline(char **bufptr, size_t *length, FILE *fp) {
    int pos = 0;
    char *ret = NULL;

    if (!*bufptr || *length < 1) {
        free(*bufptr);
        *length = 256;
        *bufptr = malloc(*length * sizeof(char));
    }

    if (!*bufptr) {
        *length = 0;
        return -1;
    }

    do {
        if (ret) {
            *length *= 2;
            ret = realloc(*bufptr, *length * sizeof(char));

            if (!ret) {
                free(*bufptr);
                *bufptr = NULL;
                *length = 0;
                return -1;
            }

            *bufptr = ret;
        }

        ret = fgets(*bufptr + pos, *length, fp);

        if (ret) {
            pos = strlen(*bufptr);
        }
    } while (ret && (*bufptr)[pos - 1] != '\n');

    return pos || ret ? pos : -1;
}
