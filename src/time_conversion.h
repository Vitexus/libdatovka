
#ifndef _ISDS_TIME_CONVERSION_H_
#define _ISDS_TIME_CONVERSION_H_

#include <stdint.h> /* int64_t */
#include <time.h>

/*
 * Convert UTC broken time to int64_t representing seconds since the Epoch.
 * @broken_utc time in UTC in broken format. Its content is not touched,
 * it's non-const because underlying POSIX function has non-const signature.
 * @return -1 in case of error
 */
int64_t _isds_timegm(struct tm *broken_utc);

/*
 * Implements gmtime_r functionality.
 * The time_t value is replaced with int64_t to always ensure 64-bit width
 * of the @timep value.
 */
struct tm *_isds_gmtime_r(const int64_t *timep, struct tm *result);

#endif /* _ISDS_TIME_CONVERSION_H_ */
