
localedir = $(datadir)/locale

MULTIPART_PARSER_ROOT = 3rdparty/multipart-parser-c/multipart-parser-c

AM_CPPFLAGS += -I$(top_srcdir)/include -I$(MULTIPART_PARSER_ROOT)

lib_LTLIBRARIES = libdatovka.la
libdatovka_la_SOURCES = \
	$(MULTIPART_PARSER_ROOT)/multipart_parser.c \
	$(MULTIPART_PARSER_ROOT)/multipart_parser.h \
	cdecode.c \
	cdecode.h \
	cencode.c \
	cencode.h \
	compiler.h \
	crypto.h \
	gettext.h \
	internal_types.h \
	isds.c \
	isds_priv.h \
	multipart_intermediate.c \
	multipart_intermediate.h \
	multipart_parts.c \
	multipart_parts.h \
	physxml.c \
	physxml.h \
	system.h \
	time_conversion.c \
	time_conversion.h \
	utils.c \
	utils.h \
	utils_memory.c \
	utils_memory.h \
	validator.c \
	validator.h

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
libdatovka_la_LDFLAGS = -version-info 4:0:0 @XML_LIBS@ @LIBCURL@ @LIBGCRYPT_LIBS@ \
	@GPGME_LIBS@ @EXPAT_LIBS@ @LTLIBINTL@
