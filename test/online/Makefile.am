AUTOMAKE_OPTIONS = color-tests parallel-tests
export VERBOSE=1

libdirpath = src
INCLUDEDIR = $(top_srcdir)/include
LIBDIR = $(top_srcdir)/$(libdirpath)
MULTIPART_PARSER_ROOT = $(LIBDIR)/3rdparty/multipart-parser-c/multipart-parser-c
localedir = $(datadir)/locale
AM_CPPFLAGS = -I$(INCLUDEDIR) -I$(LIBDIR) -I$(MULTIPART_PARSER_ROOT) \
	-I$(top_builddir)/$(libdirpath)

# Large files needed by GPGME
AM_CPPFLAGS += @XML_CPPFLAGS@ @LIBCURL_CPPFLAGS@ \
					  -DLOCALEDIR=\"$(localedir)\"
AM_CFLAGS = @LIBGCRYPT_CFLAGS@ @GPGME_CFLAGS@ @JSON_C_CFLAGS@
LDADD = @XML_LIBS@ @LIBCURL@ @LIBGCRYPT_LIBS@ @GPGME_LIBS@ @EXPAT_LIBS@ \
		@LTLIBINTL@ @JSON_C_LIBS@

TESTS = login login_mep login_mep2

noinst_PROGRAMS = $(TESTS)

common = ../test.c ../test.h ../test-tools.h common.c common.h \
	$(INCLUDEDIR)/libdatovka/isds.h \
	$(MULTIPART_PARSER_ROOT)/multipart_parser.c \
	$(MULTIPART_PARSER_ROOT)/multipart_parser.h \
	$(LIBDIR)/cdecode.c \
	$(LIBDIR)/cdecode.h \
	$(LIBDIR)/cencode.c \
	$(LIBDIR)/cencode.h \
	$(LIBDIR)/compiler.h \
	$(LIBDIR)/crypto.h \
	$(LIBDIR)/gettext.h \
	$(LIBDIR)/internal_types.h \
	$(LIBDIR)/isds.c \
	$(LIBDIR)/isds_priv.h \
	$(LIBDIR)/multipart_intermediate.c \
	$(LIBDIR)/multipart_intermediate.h \
	$(LIBDIR)/multipart_parts.c \
	$(LIBDIR)/multipart_parts.h \
	$(LIBDIR)/physxml.c \
	$(LIBDIR)/physxml.h \
	$(LIBDIR)/soap.c \
	$(LIBDIR)/soap.h \
	$(LIBDIR)/time_conversion.c \
	$(LIBDIR)/time_conversion.h \
	$(LIBDIR)/utils.c \
	$(LIBDIR)/utils.h \
	$(LIBDIR)/utils_memory.c \
	$(LIBDIR)/utils_memory.h \
	$(LIBDIR)/validator.c \
	$(LIBDIR)/validator.h
if USE_OPENSSL_BACKEND
common += \
	$(LIBDIR)/crypto_openssl.c
else
common += \
	$(LIBDIR)/crypto_gpg.c
endif
if WIN32
common += \
	$(LIBDIR)/time_conversion_win32.c \
	$(LIBDIR)/win32.c \
	$(LIBDIR)/win32.h
else
common += \
	$(LIBDIR)/time_conversion_unix.c \
	$(LIBDIR)/unix.c \
	$(LIBDIR)/unix.h
endif

# Omit isds.c
isds_common = ../test.c ../test.h ../test-tools.h \
	$(INCLUDEDIR)/libdatovka/isds.h \
	$(MULTIPART_PARSER_ROOT)/multipart_parser.c \
	$(MULTIPART_PARSER_ROOT)/multipart_parser.h \
	$(LIBDIR)/cdecode.c \
	$(LIBDIR)/cdecode.h \
	$(LIBDIR)/cencode.c \
	$(LIBDIR)/cencode.h \
	$(LIBDIR)/compiler.h \
	$(LIBDIR)/crypto.h \
	$(LIBDIR)/internal_types.h \
	$(LIBDIR)/isds_priv.h \
	$(LIBDIR)/multipart_intermediate.c \
	$(LIBDIR)/multipart_intermediate.h \
	$(LIBDIR)/multipart_parts.c \
	$(LIBDIR)/multipart_parts.h \
	$(LIBDIR)/physxml.c \
	$(LIBDIR)/physxml.h \
	$(LIBDIR)/soap.c \
	$(LIBDIR)/soap.h \
	$(LIBDIR)/time_conversion.c \
	$(LIBDIR)/time_conversion.h \
	$(LIBDIR)/utils.c \
	$(LIBDIR)/utils.h \
	$(LIBDIR)/utils_memory.c \
	$(LIBDIR)/utils_memory.h \
	$(LIBDIR)/validator.c \
	$(LIBDIR)/validator.h
if USE_OPENSSL_BACKEND
isds_common += \
	$(LIBDIR)/crypto_openssl.c
else
isds_common += \
	$(LIBDIR)/crypto_gpg.c
endif
if WIN32
isds_common += \
	$(LIBDIR)/time_conversion_win32.c \
	$(LIBDIR)/win32.c \
	$(LIBDIR)/win32.h
else
isds_common += \
	$(LIBDIR)/time_conversion_unix.c \
	$(LIBDIR)/unix.c \
	$(LIBDIR)/unix.h
endif

# Access _hidden symbols
#compute_hash_SOURCES 	= compute_hash.c $(common)
login_SOURCES 			= login.c $(common)
login_mep_SOURCES		= login_mep.c $(common)
login_mep2_SOURCES		= login_mep2.c $(common)

# Access static symbols from isds.c
#isds_dbtype_SOURCES 		= isds-dbtype.c $(isds_common)

