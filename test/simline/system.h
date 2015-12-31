#ifndef __ISDS_SYSTEM_H__
#define __ISDS_SYSTEM_H__

#include "http.h"

#ifdef _WIN32
#include "win32.h"
#else
#include "unix.h"
#endif

#include <time.h>       /* for struvt tm */
#include <sys/time.h>   /* for time_t */

http_error _server_datestring2tm(const char *string, struct tm *time);

/* Convert UTC broken time to time_t.
 * @broken_utc it time in UTC in broken format. Despite its content is not
 * touched, it'sw not-const because underlying POSIX function has non-const
 * signature.
 * @return (time_t) -1 in case of error */
time_t _isds_timegm(struct tm *broken_utc);

#endif
