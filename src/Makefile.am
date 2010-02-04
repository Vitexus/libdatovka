libisds_ladir = $(includedir)
localedir = $(datadir)/locale

lib_LTLIBRARIES = libisds.la
libisds_la_SOURCES = cdecode.c cencode.c crypto.c isds.c physxml.c soap.c \
					 utils.c validator.c \
					 cdecode.h cencode.h crypto.h gettext.h isds.h \
					 isds_priv.h physxml.h soap.h utils.h validator.h
libisds_la_HEADERS = isds.h
libisds_la_CPPFLAGS = -D_FILE_OFFSET_BITS=64 -DLARGEFILE_SOURCE=1 \
					  @XML_CPPFLAGS@ @LIBCURL_CPPFLAGS@ \
					  -DLOCALEDIR=\"$(localedir)\"
libisds_la_CFLAGS = @LIBGCRYPT_CFLAGS@ @GPGME_CFLAGS@
libisds_la_LDFLAGS = -version-info 1:0:0 @XML_LIBS@ @LIBCURL@ @LIBGCRYPT_LIBS@ \
	@GPGME_LIBS@ -lexpat @LTLIBINTL@
