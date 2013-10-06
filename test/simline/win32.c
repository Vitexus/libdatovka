#include "../test-tools.h"
#include <stdio.h>
#include <string.h>
#include "http.h"
#include "win32.h"

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
http_error _server_datestring2tm(const char *string, struct tm *time) {
    char *ptr;
    int len, tmp;
    if (!string || !time) return HTTP_ERROR_SERVER;

    memset(time, 0, sizeof(*time));

    if (sscanf(string, "%d-%d-%d%n",
                &time->tm_year, &time->tm_mon, &time->tm_mday, &tmp) >= 3
            && tmp == strlen(string)) {
        time->tm_mon--;
        time->tm_year -= 1900;
            return HTTP_ERROR_SUCCESS;
    }

    memset(time, 0, sizeof(*time));

    if (sscanf(string, "%d-%d%n",
                &time->tm_year, &time->tm_yday, &tmp) >= 2
            && tmp == strlen(string)) {
        time->tm_yday--;
        time->tm_year -= 1900;
        yday2mday(time);
            return HTTP_ERROR_SUCCESS;
    }

    memset(time, 0, sizeof(*time));
    len = strlen(string);

    if (len < 4) {
        return HTTP_ERROR_SERVER;
    }

    ptr = strdup(string);

    if (sscanf(ptr + len - 2, "%d%n", &time->tm_mday, &tmp) < 1 || tmp < 2) {
        free(ptr);
        return HTTP_ERROR_SERVER;
    }

    ptr[len - 2] = '\0';

    if (sscanf(ptr + len - 4, "%d%n", &time->tm_mon, &tmp) < 1 || tmp < 2) {
        free(ptr);
        return HTTP_ERROR_SERVER;
    }

    ptr[len - 4] = '\0';

    if (sscanf(ptr, "%d%n", &time->tm_year, &tmp) < 1 || tmp < len - 4) {
        free(ptr);
        return HTTP_ERROR_SERVER;
    }

    free(ptr);
    time->tm_mon--;
    time->tm_year -= 1900;
    return HTTP_ERROR_SUCCESS;
}
