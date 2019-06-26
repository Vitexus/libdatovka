AUTOMAKE_OPTIONS = color-tests parallel-tests
export VERBOSE=1

libdirpath = src
LIBDIR = $(top_srcdir)/$(libdirpath)
localedir = $(datadir)/locale
AM_CPPFLAGS = -I$(LIBDIR) -I$(top_builddir)/$(libdirpath)

# Large files needed by GPGME
AM_CPPFLAGS += @XML_CPPFLAGS@ @LIBCURL_CPPFLAGS@ \
					  -DLOCALEDIR=\"$(localedir)\"
AM_CFLAGS = @LIBGCRYPT_CFLAGS@ @GPGME_CFLAGS@
LDADD = @XML_LIBS@ @LIBCURL@ @LIBGCRYPT_LIBS@ @GPGME_LIBS@ @EXPAT_LIBS@ \
		@LTLIBINTL@

TESTS = login login_mep

noinst_PROGRAMS = $(TESTS)

common = ../test.c ../test.h ../test-tools.h common.c common.h \
	$(LIBDIR)/cdecode.c \
	$(LIBDIR)/cencode.c \
	$(LIBDIR)/isds.c \
	$(LIBDIR)/physxml.c \
	$(LIBDIR)/soap.c \
	$(LIBDIR)/utils.c \
	$(LIBDIR)/validator.c \
	$(LIBDIR)/cdecode.h \
	$(LIBDIR)/cencode.h \
	$(LIBDIR)/crypto.h \
	$(LIBDIR)/gettext.h \
	$(LIBDIR)/isds.h \
	$(LIBDIR)/isds_priv.h \
	$(LIBDIR)/physxml.h \
	$(LIBDIR)/soap.h \
	$(LIBDIR)/utils.h \
	$(LIBDIR)/validator.h
if USE_OPENSSL_BACKEND
common += \
	$(LIBDIR)/crypto_openssl.c
else
common += \
	$(LIBDIR)/crypto_gpg.c
endif
if WIN32
common += $(LIBDIR)/win32.c $(LIBDIR)/win32.h
else
common += $(LIBDIR)/unix.c $(LIBDIR)/unix.h
endif

# Omit isds.c
isds_common = ../test.c ../test.h ../test-tools.h \
	$(LIBDIR)/cdecode.c \
	$(LIBDIR)/cencode.c \
	$(LIBDIR)/physxml.c \
	$(LIBDIR)/soap.c \
	$(LIBDIR)/utils.c \
	$(LIBDIR)/validator.c \
	$(LIBDIR)/cdecode.h \
	$(LIBDIR)/cencode.h \
	$(LIBDIR)/crypto.h \
	$(LIBDIR)/isds.h \
	$(LIBDIR)/isds_priv.h \
	$(LIBDIR)/physxml.h \
	$(LIBDIR)/soap.h \
	$(LIBDIR)/utils.h \
	$(LIBDIR)/validator.h
if USE_OPENSSL_BACKEND
isds_common += \
	$(LIBDIR)/crypto_openssl.c
else
isds_common += \
	$(LIBDIR)/crypto_gpg.c
endif
if WIN32
isds_common += $(LIBDIR)/win32.c $(LIBDIR)/win32.h
else
isds_common += $(LIBDIR)/unix.c $(LIBDIR)/unix.h
endif

# Access _hidden symbols
#compute_hash_SOURCES 	= compute_hash.c $(common)
login_SOURCES 			= login.c $(common)
login_mep_SOURCES		= login_mep.c $(common)

# Access static symbols from isds.c
#isds_dbtype_SOURCES 		= isds-dbtype.c $(isds_common)

