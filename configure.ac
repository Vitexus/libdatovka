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

# Checks for libraries.
#TODO: Verify minimal version. libxml2-2.6.26 is just a guess.
AM_PATH_XML2([2.6.26])
#TODO: Determine minimal version
LIBCURL_CHECK_CONFIG()
#TODO: Determine minimal version
AM_PATH_LIBGCRYPT([1:1.4.0])
#TODO: Determine minimal version
AM_PATH_GPGME()
#AC_CHECK_LIB([expat])

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

AC_CONFIG_FILES([Makefile src/Makefile client/Makefile test/Makefile])
#AC_CONFIG_SUBDIRS([src])
AC_OUTPUT
