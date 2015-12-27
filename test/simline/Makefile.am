AUTOMAKE_OPTIONS = color-tests parallel-tests
export VERBOSE=1

libdirpath = src
INCLUDEDIR = $(top_srcdir)/$(libdirpath)
LIBDIR = $(top_builddir)/$(libdirpath)
localedir = $(datadir)/locale

AM_CPPFLAGS = @XML_CPPFLAGS@
AM_CFLAGS = -I$(INCLUDEDIR) -DSRCDIR=\"$(top_srcdir)\"
LDADD = -L$(LIBDIR) $(LIBDIR)/libisds.la -L. ./libserver.la

# To fake home dir for gnupg2
export HOME=@top_builddir@

TESTS =
if BUILD_CURL
TESTS += basic_authentication hotp_authentication totp_authentication \
		 certificate_user_password_authentication \
		 isds_change_password isds_get_commercial_credit \
		 isds_find_box_by_fulltext isds_FindDataBox \
		 hotp_isds_change_password totp_isds_change_password \
		 isds_delete_message_from_storage isds_ping isds_resign_message
endif


# Server library
noinst_LTLIBRARIES = libserver.la
libserver_la_SOURCES = ../test.c ../test-tools.h http.c http.h \
						server.c server.h service.c service.h \
						services.h server_types.h system.h
if WIN32
libserver_la_SOURCES += win32.c win32.h
else
libserver_la_SOURCES += unix.c unix.h
endif

libserver_la_CPPFLAGS = @XML_CPPFLAGS@ @LIBGNUTLS_CFLAGS@
libserver_la_LIBADD = @XML_LIBS@ @LIBGNUTLS_LIBS@

# Standalone server
noinst_PROGRAMS = server_cli $(TESTS)
server_cli_SOURCES = server_cli.c
server_cli_LDADD = ./libserver.la -L.

# Access public API only
common = ../test.h
basic_authentication_SOURCES = basic_authentication.c $(common)
hotp_authentication_SOURCES = hotp_authentication.c $(common)
totp_authentication_SOURCES = totp_authentication.c $(common)
certificate_user_password_authentication_SOURCES = \
	certificate_user_password_authentication.c $(common)
isds_change_password_SOURCES = isds_change_password.c $(common)
isds_get_commercial_credit_SOURCES = isds_get_commercial_credit.c $(common)
isds_find_box_by_fulltext_SOURCES = isds_find_box_by_fulltext.c $(common)
isds_FindDataBox_SOURCES = isds_FindDataBox.c $(common)
hotp_isds_change_password_SOURCES = hotp_isds_change_password.c $(common)
totp_isds_change_password_SOURCES = totp_isds_change_password.c $(common)
isds_delete_message_from_storage_SOURCES = isds_delete_message_from_storage.c $(common)
isds_resign_message_SOURCES = isds_resign_message.c $(common)
isds_ping_SOURCES = isds_ping.c $(common)

