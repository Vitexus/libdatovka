AUTOMAKE_OPTIONS = color-tests parallel-tests
export VERBOSE=1

libdirpath = src
LIBDIR = $(top_srcdir)/$(libdirpath)
localedir = $(datadir)/locale

AM_CPPFLAGS = @XML_CPPFLAGS@
AM_CFLAGS = -I$(srcdir)/$(LIBDIR) -DSRCDIR=\"$(top_srcdir)\"
LDADD = $(LIBDIR)/libisds.la -L$(LIBDIR) @XML_LIBS@

# To fake home dir for gnupg2
export HOME=@top_builddir@

#TESTS = prepare_environment
TESTS =

if BUILD_CURL
TESTS += basic_authentication
endif


noinst_PROGRAMS = $(TESTS)

common = ../test.c ../test.h ../test-tools.h http.c http.h server.c server.h

# Access public API only
basic_authentication_SOURCES = basic_authentication.c $(common)

# Tests that need .gnupg
#guess_raw_type.log load_raw.log: prepare_environment.log
#prepare_environment_SOURCES = prepare_environment.c $(common)

