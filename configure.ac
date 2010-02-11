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
#TODO: Determine minimal version
LIBCURL_CHECK_CONFIG()
#TODO: Determine minimal version
AM_PATH_LIBGCRYPT([1:1.4.0])

# GPGME is compiled with large file support by default. We need to follow not
# to break ABI.
AC_SYS_LARGEFILE
#TODO: Determine minimal version
AM_PATH_GPGME()

# Check for expat
#TODO: Verify minimal version. See src/phys.c: init_expat()
AC_CHECK_LIB([expat], [XML_ExpatVersionInfo],
             EXPAT_LIBS=-lexpat
             AC_SUBST(EXPAT_LIBS))


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
                 test/Makefile])

#AC_CONFIG_SUBDIRS([src])
AC_OUTPUT
