#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.63])
AC_INIT([libisds], [0.1], [petr.pisar@atlas.cz])
AM_INIT_AUTOMAKE(libisds, 0.1)
AC_CONFIG_SRCDIR([src/isds.c])
AC_CONFIG_MACRO_DIR([m4])
AC_CONFIG_HEADERS([config.h])

# Checks for programs.
AC_PROG_CC
AC_PROG_CPP
#AC_PROG_MAKE_SET
LT_INIT


# Enable example clients
AC_ARG_ENABLE(example, [AS_HELP_STRING([--enable-example],
                              [Build library usage examples])])
AM_CONDITIONAL([BUILD_EXAMPLE], [test "$enable_example" = "yes"])

# Enable tests
AC_ARG_ENABLE(test, [AS_HELP_STRING([--enable-test],
                              [Enable internal library tests])])
AM_CONDITIONAL([BUILD_TEST], [test "$enable_test" = "yes"])

# Enable on-line test
AC_ARG_ENABLE(online-test, [AS_HELP_STRING([--enable-online-test],
                              [Enable on-line internal library tests])])
AM_CONDITIONAL([BUILD_ONLINE_TEST],
               [test "$enable_test" = "yes" -a "$enable_online_test" = "yes"])

# Enable gettext
AM_GNU_GETTEXT_VERSION([0.17])
AM_GNU_GETTEXT([external])

# Enable debug
AC_ARG_ENABLE(debug, [AS_HELP_STRING([--enable-debug],
                        [Compile in debugging mode])])
AS_IF([test "$enable_debug" = "yes"],
      CFLAGS="${CFLAGS} -g"
      dnl AC_SUBST(DEBUG_CFLAGS)
      AC_DEFINE([ENABLE_DEBUG], [1],
                [Define if you want include debugging code])
      )


# Checks for libraries.
#TODO: Verify minimal version. libxml2-2.6.26 is just a guess.
AM_PATH_XML2([2.6.26])

# Checks for CURL 
LIBCURL_CHECK_CONFIG()
AC_CHECK_DECLS([CURLOPT_TIMEOUT_MS], [],
    AC_MSG_WARN([Your libcurl does not support subsecond timeout resolution.
                 Consider upgrade to 7.16.2 version.]),
    [[#include <curl/curl.h>]])
AC_CHECK_DECLS([CURLOPT_USERNAME], [],
    AC_MSG_WARN([Your libcurl does not support separate username and password.
                 Your ISDS username could not contain colon.
                 Consider upgrade to 7.19.1 version.]),
    [[#include <curl/curl.h>]])

#TODO: Determine minimal version
AM_PATH_LIBGCRYPT([1:1.4.0])

# GPGME is compiled with large file support by default. We need to follow not
# to break ABI.
AC_SYS_LARGEFILE
#TODO: Determine minimal version
AM_PATH_GPGME()

# Check for expat
AC_CHECK_LIB([expat], [XML_ExpatVersionInfo],
             EXPAT_LIBS=-lexpat
             AC_SUBST(EXPAT_LIBS))
# Verify minimal version. See src/phys.c: init_expat()
expat_min_major="1"
expat_min_minor="95"
expat_min_micro="8"
expat_min_version="${expat_min_major}.${expat_min_minor}.${expat_min_micro}"
AC_MSG_CHECKING([for expat library version >= ${expat_min_version}])
AC_COMPILE_IFELSE(
        AC_LANG_PROGRAM(
            [#define MIN_MAJOR ${expat_min_major}
            #define MIN_MINOR ${expat_min_minor}
            #define MIN_MICRO ${expat_min_micro}
            #include <expat.h>],
            [[#if (XML_MAJOR_VERSION < MIN_MAJOR) || \
    (XML_MAJOR_VERSION == MIN_MAJOR && XML_MINOR_VERSION < MIN_MAJOR) || \
    (XML_MAJOR_VERSION == MIN_MAJOR && XML_MINOR_VERSION == MIN_MAJOR && \
        XML_MICRO_VERSION < MIN_MICRO)
#error "Expat library is too old"
#endif]]),
        AC_MSG_RESULT([yes]),
        AC_MSG_FAILURE([Upgrade your expat to ${expat_min_version}])
)


# Do not consider compiler warnings as errors
# This harms libxml confgure test
AC_ARG_ENABLE(fatalwarnings, [AS_HELP_STRING([--disable-fatalwarnings],
                        [Do not consider compiler warnings as errors])])
AS_IF([test "$enable_fatalwarnings" = "no"], , CFLAGS="${CFLAGS} -Werror")


# Checks for header files.
AC_CHECK_HEADERS([fcntl.h locale.h stdint.h stdlib.h string.h unistd.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_TYPE_SIZE_T
#AC_TYPE_SSIZE_T
AC_TYPE_UINT8_T

# Checks for library functions.
AC_FUNC_MALLOC
AC_FUNC_MKTIME
AC_FUNC_MMAP
AC_CHECK_FUNCS([localtime_r memset munmap setlocale strdup strerror])

# Default CFLAGS
CFLAGS="${CFLAGS} -std=c99 -Wall"

AC_CONFIG_FILES([Makefile src/Makefile po/Makefile.in client/Makefile
                 test/Makefile test/online/Makefile libisds.pc])

#AC_CONFIG_SUBDIRS([src])
AC_OUTPUT
