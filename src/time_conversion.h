
#ifndef _ISDS_TIME_CONVERSION_H_
#define _ISDS_TIME_CONVERSION_H_

#include <stdint.h> /* int64_t */

/* Convert UTC broken time to int64_t representing seconds since the Epoch.
 * @broken_utc time in UTC in broken format. Its content is not touched,
 * it's non-const because underlying POSIX function has non-const signature.
 * @return -1 in case of error */
int64_t _isds_timegm(struct tm *broken_utc);

#endif /* _ISDS_TIME_CONVERSION_H_ */
