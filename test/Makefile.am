AUTOMAKE_OPTIONS = color-tests parallel-tests
export VERBOSE=1

if BUILD_ONLINE_TEST
SUBDIRS = online
endif

libdirpath = src
LIBDIR = $(top_srcdir)/$(libdirpath)
localedir = $(datadir)/locale
AM_CPPFLAGS = -I$(LIBDIR) -I$(top_builddir)/$(libdirpath) \
			  -DSRCDIR=\"$(top_srcdir)\"

# Large files needed by GPGME
AM_CPPFLAGS += @XML_CPPFLAGS@ @LIBCURL_CPPFLAGS@ \
					  -DLOCALEDIR=\"$(localedir)\"
AM_CFLAGS = @LIBGCRYPT_CFLAGS@ @GPGME_CFLAGS@
LDADD = @XML_LIBS@ @LIBCURL@ @LIBGCRYPT_LIBS@ @GPGME_LIBS@ @EXPAT_LIBS@ \
		@LTLIBINTL@

TESTS = b64decode b64encode \
		compute_hash context guess_raw_type init_gpgpme isds_init load_raw \
		normalize_mime_type  \
		isds_list_free isds_pki_credentials_free isds_hash_free \
		isds_DbOwnerInfo_free isds_DbUserInfo_free isds_event_free \
		isds_envelope_free isds_document_free isds_message_free \
		isds_message_copy_free isds_approval_free \
		isds-dbtype isds-filemetatype isds-hash_algorithm isds-usertype \
		isds-datestring2tm isds-tm2datestring \
		isds-timestring2timeval isds-timeval2timestring \
		isds-uint2isds_message_status isds-eventstring2event


noinst_PROGRAMS = $(TESTS)

common = test.c test.h \
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
isds_common = test.c test.h \
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
b64decode_SOURCES 		= b64decode.c $(common)
b64encode_SOURCES 		= b64encode.c $(common)
compute_hash_SOURCES 	= compute_hash.c $(common)
context_SOURCES 		= context.c $(common)
guess_raw_type_SOURCES 	= guess_raw_type.c $(common)
init_gpgpme_SOURCES 	= init_gpgpme.c $(common)
isds_init_SOURCES 		= isds_init.c $(common)
load_raw_SOURCES 		= load_raw.c $(common)
normalize_mime_type_SOURCES 		= normalize_mime_type.c $(common)
isds_list_free_SOURCES 	= isds_list_free.c $(common)
isds_pki_credentials_free_SOURCES 	= isds_pki_credentials_free.c $(common)
isds_hash_free_SOURCES 	= isds_hash_free.c $(common)
isds_DbOwnerInfo_free_SOURCES 	= isds_DbOwnerInfo_free.c $(common)
isds_DbUserInfo_free_SOURCES 	= isds_DbUserInfo_free.c $(common)
isds_event_free_SOURCES 	= isds_event_free.c $(common)
isds_envelope_free_SOURCES 	= isds_envelope_free.c $(common)
isds_document_free_SOURCES 	= isds_document_free.c $(common)
isds_message_free_SOURCES 	= isds_message_free.c $(common)
isds_message_copy_free_SOURCES 	= isds_message_copy_free.c $(common)
isds_approval_free_SOURCES 	= isds_approval_free.c $(common)

# Access static symbols from isds.c
isds_dbtype_SOURCES 		= isds-dbtype.c $(isds_common)
isds_filemetatype_SOURCES 	= isds-filemetatype.c $(isds_common)
isds_hash_algorithm_SOURCES = isds-hash_algorithm.c $(isds_common)
isds_usertype_SOURCES 		= isds-usertype.c $(isds_common)
isds_datestring2tm_SOURCES 	= isds-datestring2tm.c $(isds_common)
isds_tm2datestring_SOURCES 	= isds-tm2datestring.c $(isds_common)
isds_timestring2timeval_SOURCES = isds-timestring2timeval.c $(isds_common)
isds_timeval2timestring_SOURCES	= isds-timeval2timestring.c $(isds_common)
isds_uint2isds_message_status_SOURCES	= isds-uint2isds_message_status.c \
										  $(isds_common)
isds_eventstring2event_SOURCES	= isds-eventstring2event.c $(isds_common)

## First specific tests
##isds-%.test: isds-%.o
##	$(CC) $(LDFLAGS) $< $(filter-out ../src/isds.o,$(OBJS)) -o $@

## Then generic rule. Order is important.
##%.test: %.o
##	$(CC) $(LDFLAGS) $< $(OBJS) -o $@


