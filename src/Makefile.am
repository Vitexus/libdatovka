libisds_ladir = $(includedir)
localedir = $(datadir)/locale

lib_LTLIBRARIES = libisds.la
libisds_la_SOURCES = \
	cdecode.c \
	cdecode.h \
	cencode.c \
	cencode.h \
	crypto.c \
	crypto.h \
	gettext.h \
	isds.c \
	isds.h \
	isds_priv.h \
	physxml.c \
	physxml.h \
	utils.c \
	utils.h \
	validator.c \
	validator.h \
	system.h

#if USE_OPENSSL
libisds_la_SOURCES += \
	crypto_openssl.c
#else
libisds_la_SOURCES += \
	crypto_gpg.c
#endif

if BUILD_CURL
libisds_la_SOURCES += \
	soap.c \
	soap.h
endif

if WIN32
libisds_la_SOURCES += \
	win32.c \
	win32.h
else
libisds_la_SOURCES += \
	unix.c \
	unix.h
endif

libisds_la_HEADERS = isds.h
libisds_la_CPPFLAGS = @XML_CPPFLAGS@ @LIBCURL_CPPFLAGS@ \
					  -DLOCALEDIR=\"$(localedir)\"
libisds_la_CFLAGS = @LIBGCRYPT_CFLAGS@ @GPGME_CFLAGS@
libisds_la_LDFLAGS = -version-info 8:0:3 @XML_LIBS@ @LIBCURL@ @LIBGCRYPT_LIBS@ \
	@GPGME_LIBS@ @EXPAT_LIBS@ @LTLIBINTL@
