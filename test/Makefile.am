LIBDIR =../src
localedir = $(datadir)/locale
AM_CPPFLAGS =-I$(LIBDIR)

# Large files needed by GPGME
AM_CPPFLAGS += @XML_CPPFLAGS@ @LIBCURL_CPPFLAGS@ \
					  -DLOCALEDIR=\"$(localedir)\"
AM_CFLAGS = @LIBGCRYPT_CFLAGS@ @GPGME_CFLAGS@
LDADD = @XML_LIBS@ @LIBCURL@ @LIBGCRYPT_LIBS@ @GPGME_LIBS@ -lexpat @LTLIBINTL@

TESTS = compute_hash context init_gpgpme isds_init login \
		isds-dbtype isds-filemetatype isds-hash_algorithm


noinst_PROGRAMS = $(TESTS)

common = test.h \
	$(LIBDIR)/cdecode.c \
	$(LIBDIR)/cencode.c \
	$(LIBDIR)/crypto.c \
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

# Omit isds.c
isds_common = test.h \
	$(LIBDIR)/cdecode.c \
	$(LIBDIR)/cencode.c \
	$(LIBDIR)/crypto.c \
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

# Access _hidden symbols
compute_hash_SOURCES 	= compute_hash.c $(common)
context_SOURCES 		= context.c $(common)
init_gpgpme_SOURCES 	= init_gpgpme.c $(common)
isds_init_SOURCES 		= isds_init.c $(common)
login_SOURCES 			= login.c $(common)

# Access static symbols from isds.c
isds_dbtype_SOURCES 		= isds-dbtype.c $(isds_common)
isds_filemetatype_SOURCES 	= isds-filemetatype.c $(isds_common)
isds_hash_algorithm_SOURCES = isds-hash_algorithm.c $(isds_common)

## First specific tests
##isds-%.test: isds-%.o
##	$(CC) $(LDFLAGS) $< $(filter-out ../src/isds.o,$(OBJS)) -o $@

## Then generic rule. Order is important.
##%.test: %.o
##	$(CC) $(LDFLAGS) $< $(OBJS) -o $@


