
#include "isds_priv.h" /* _XOPEN_SOURCE -- gmtime_r */
#include "compiler.h" /* _hidden */
#include "time_conversion.h"

#ifdef _WIN32
/* time_conversion_win32.c */
struct tm *gmtime_r(const time_t *timep, struct tm *result);
#endif /* _WIN32 */

_hidden struct tm *_isds_gmtime_r(const int64_t *timep, struct tm *result)
{
	/*
	 * Various systems use various time_t representation.
	 * FreeBSD 13.0 on i386 uses 32-bit time_t.
	 * Linux on x86 uses 32-bit or 64-bit time depending on the version of
	 * glibc and the kernel.
	 *
	 * MinGW32 GCC 4.8+ uses 64-bit time_t (when __MINGW_USE_VC2005_COMPAT
	 * is defined) but time->tv_sec is defined as 32-bit long in
	 * Microsoft API. Convert value to the type expected by gmtime_r().
	 */

	if (timep == NULL) {
		return NULL;
	}

	/* Use implicit conversion. */
	const time_t time = *timep;
	return gmtime_r(&time, result);
}
