
#include <string.h>
#include <time.h>

#include "time_conversion.h"
#include "utils.h" /* _hidden */

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
