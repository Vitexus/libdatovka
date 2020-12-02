#ifndef __ISDS_SYSTEM_H__
#define __ISDS_SYSTEM_H__

#include "libdatovka/isds.h"

#ifdef _WIN32
#include "win32.h"
#else
#include "unix.h"
#endif

/* Convert UTF-8 @string representation of ISO 8601 date to @time.
 * XXX: Not all ISO formats are supported */
isds_error _isds_datestring2tm(const xmlChar *string, struct tm *time);
#endif
