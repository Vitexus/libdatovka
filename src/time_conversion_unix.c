
#include "isds_priv.h" /* _XOPEN_SOURCE -- strdup */
#include <stdlib.h> /* getenv */
#include <string.h>

#include "time_conversion.h"
#include "utils.h" /* PANIC */

static char *tz_orig = NULL; /* Copy of original TZ variable */

/*
 * Switches time zone to UTC.
 * XXX: This is not reentrant and not thread-safe.
 */
static void _isds_switch_tz_to_utc(void)
{
	char *tz;

	tz = getenv("TZ");
	if (tz != NULL) {
		tz_orig = strdup(tz);
		if (tz_orig == NULL) {
			PANIC("Cannot back original time zone up.");
		}
	} else {
		tz_orig = NULL;
	}

	if (setenv("TZ", "", 1)) {
		PANIC("Cannot change time zone to UTC temporarily.");
	}

	tzset();
}


/*
 * Switches time zone to original value.
 * XXX: This is not reentrant and not thread-safe.
 */
static void _isds_switch_tz_to_native(void)
{
	if (tz_orig != NULL) {
		if (setenv("TZ", tz_orig, 1)) {
			PANIC("Cannot restore time zone by setting TZ variable.");
		}
		free(tz_orig); tz_orig = NULL;
	} else {
		if(unsetenv("TZ")) {
			PANIC("Cannot restore time zone by unsetting TZ variable.");
		}
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
