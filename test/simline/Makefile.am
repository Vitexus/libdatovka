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
TESTS += basic_authentication hotp_authentication totp_authentication \
		 isds_change_password hotp_isds_change_password \
		 totp_isds_change_password \
		 isds_delete_message_from_storage
endif


noinst_PROGRAMS = server_cli $(TESTS)

server_implementation = ../test.c ../test-tools.h http.c http.h \
						server.c server.h service.c service.h \
						services.h

# Standalone server
server_cli_SOURCES = server_cli.c $(server_implementation)

# Access public API only
common = ../test.h $(server_implementation)
basic_authentication_SOURCES = basic_authentication.c $(common)
hotp_authentication_SOURCES = hotp_authentication.c $(common)
totp_authentication_SOURCES = totp_authentication.c $(common)
isds_change_password_SOURCES = isds_change_password.c $(common)
hotp_isds_change_password_SOURCES = hotp_isds_change_password.c $(common)
totp_isds_change_password_SOURCES = totp_isds_change_password.c $(common)
isds_delete_message_from_storage_SOURCES = isds_delete_message_from_storage.c $(common)

# Tests that need .gnupg
#guess_raw_type.log load_raw.log: prepare_environment.log
#prepare_environment_SOURCES = prepare_environment.c $(common)

