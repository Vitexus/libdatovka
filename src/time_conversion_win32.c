
#include <string.h>
#include <time.h>

#include "compiler.h" /* _hidden */
#include "time_conversion.h"

_hidden int64_t _isds_timegm(struct tm *broken_utc)
{
	time_t ret;
	time_t diff;
	struct tm broken, *tmp;

	ret = time(NULL);
	tmp = gmtime(&ret);

	if (tmp == NULL) {
		return (int64_t)-1;
	}

	tmp->tm_isdst = broken_utc->tm_isdst;
	diff = ret - mktime(tmp);
	memcpy(&broken, broken_utc, sizeof(struct tm));
	broken.tm_isdst = tmp->tm_isdst; /* handle broken_utc->tm_isdst < 0 */
	return mktime(&broken) + diff;
}

/* MSVCRT gmtime() uses thread-local buffer. This is reentrant. */
_hidden struct tm *gmtime_r(const time_t *timep, struct tm *result)
{
	struct tm *buf = gmtime(timep);

	if (buf == NULL) {
		return NULL;
	}

	memcpy(result, buf, sizeof(struct tm));
	return result;
}
