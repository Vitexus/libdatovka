#ifndef __ISDS_SYSTEM_H__
#define __ISDS_SYSTEM_H__

#include "isds.h"

#ifdef _WIN32
#include "win32.h"
#else
#include "unix.h"
#endif

isds_error _isds_datestring2tm(const xmlChar *string, struct tm *time);
#endif
