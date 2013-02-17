#ifndef __ISDS_WIN32_SYSTEM_H
#define __ISDS_WIN32_SYSTEM_H

#define mkdir(x,y) mkdir(x)
// FIXME: Get real implementation of nl_langinfo
#define nl_langinfo(x) "CP1250"

#ifdef __GNUC__
// MinGW provides neither strtok_r() nor strtok_s().
// For now, we can at least use strtok() because it's thread-safe in MSVCRT.
#define strtok_r(x,y,z) strtok(x,y)
#else
#define strtok_r strtok_s
#endif

struct tm *gmtime_r(const time_t *timep, struct tm *result);
char *strndup(const char *s, size_t n);

// disabled under MinGW by -std=c99
_CRTIMP char* __cdecl __MINGW_NOTHROW strdup (const char*) __MINGW_ATTRIB_MALLOC;

#endif
