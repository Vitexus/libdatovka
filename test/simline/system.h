#ifndef __ISDS_SYSTEM_H__
#define __ISDS_SYSTEM_H__

#include "http.h"

#ifdef _WIN32
#include "win32.h"
#else
#include "unix.h"
#endif

http_error _server_datestring2tm(const char *string, struct tm *time);
#endif
