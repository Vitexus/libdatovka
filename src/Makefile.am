libisds_ladir = $(includedir)
localedir = $(datadir)/locale

lib_LTLIBRARIES = libisds.la
libisds_la_SOURCES = cdecode.c cencode.c crypto.c isds.c physxml.c \
					 utils.c validator.c \
					 cdecode.h cencode.h crypto.h gettext.h isds.h \
					 isds_priv.h physxml.h utils.h validator.h
if BUILD_CURL
libisds_la_SOURCES += soap.c soap.h
endif

libisds_la_HEADERS = isds.h
libisds_la_CPPFLAGS = @XML_CPPFLAGS@ @LIBCURL_CPPFLAGS@ \
					  -DLOCALEDIR=\"$(localedir)\"
libisds_la_CFLAGS = @LIBGCRYPT_CFLAGS@ @GPGME_CFLAGS@
libisds_la_LDFLAGS = -version-info 4:0:1 @XML_LIBS@ @LIBCURL@ @LIBGCRYPT_LIBS@ \
	@GPGME_LIBS@ @EXPAT_LIBS@ @LTLIBINTL@
