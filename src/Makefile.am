
localedir = $(datadir)/locale

AM_CPPFLAGS += -I$(top_srcdir)/include

lib_LTLIBRARIES = libdatovka.la
libdatovka_la_SOURCES = \
	cdecode.c \
	cdecode.h \
	cencode.c \
	cencode.h \
	crypto.h \
	gettext.h \
	isds.c \
	isds_priv.h \
	physxml.c \
	physxml.h \
	utils.c \
	utils.h \
	validator.c \
	validator.h \
	system.h \
	time_conversion.c \
	time_conversion.h

if USE_OPENSSL_BACKEND
libdatovka_la_SOURCES += \
	crypto_openssl.c
else
libdatovka_la_SOURCES += \
	crypto_gpg.c
endif

if BUILD_CURL
libdatovka_la_SOURCES += \
	soap.c \
	soap.h
endif

if WIN32
libdatovka_la_SOURCES += \
	time_conversion_win32.c \
	win32.c \
	win32.h
else
libdatovka_la_SOURCES += \
	time_conversion_unix.c \
	unix.c \
	unix.h
endif

# Read
# https://www.gnu.org/software/libtool/manual/html_node/Updating-version-info.html
# for a description how to bump the version info numbers.

libdatovka_la_CPPFLAGS = @XML_CPPFLAGS@ @LIBCURL_CPPFLAGS@ \
	-DLOCALEDIR=\"$(localedir)\" $(AM_CPPFLAGS)
libdatovka_la_CFLAGS = @LIBGCRYPT_CFLAGS@ @GPGME_CFLAGS@
libdatovka_la_LDFLAGS = -version-info 2:0:0 @XML_LIBS@ @LIBCURL@ @LIBGCRYPT_LIBS@ \
	@GPGME_LIBS@ @EXPAT_LIBS@ @LTLIBINTL@
