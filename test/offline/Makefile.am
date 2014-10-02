AUTOMAKE_OPTIONS = color-tests parallel-tests
export VERBOSE=1

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

# To fake home dir for gnupg2
export HOME=@top_builddir@

TESTS = prepare_environment \
		b64decode b64encode \
		compute_hash context guess_raw_type init_crypto isds_init load_raw \
		normalize_mime_type utf82locale \
		isds_list_free isds_pki_credentials_free isds_hash_free \
		isds_PersonName_free isds_BirthInfo_free isds_Address_free \
		isds_DbOwnerInfo_free isds_DbUserInfo_free isds_event_free \
		isds_envelope_free isds_document_free isds_message_free \
		isds_message_copy_free isds_message_status_change_free isds_approval_free \
		isds_commercial_permission_free isds_credentials_delivery_free \
		isds_credit_event_free \
		isds_PersonName_duplicate isds_Address_duplicate \
		isds_DbOwnerInfo_duplicate isds_DbUserInfo_duplicate \
		isds-filemetatype isds-hash_algorithm \
		isds-timestring2timeval \
		isds-uint2isds_message_status isds-eventstring2event
if BUILD_CURL
TESTS += isds-dbtype isds-sendertype isds-usertype isds-paymenttype\
		 isds-crediteventtype isds-datestring2tm isds-tm2datestring \
		 isds-timeval2timestring \
		 soap-decode_header_value soap-string2isds_otp_resolution
endif


noinst_PROGRAMS = $(TESTS)

common = ../test.c ../test.h ../test-tools.h \
	$(LIBDIR)/cdecode.c \
	$(LIBDIR)/cencode.c \
	$(LIBDIR)/isds.c \
	$(LIBDIR)/physxml.c \
	$(LIBDIR)/utils.c \
	$(LIBDIR)/validator.c \
	$(LIBDIR)/cdecode.h \
	$(LIBDIR)/cencode.h \
	$(LIBDIR)/crypto.h \
	$(LIBDIR)/gettext.h \
	$(LIBDIR)/isds.h \
	$(LIBDIR)/isds_priv.h \
	$(LIBDIR)/physxml.h \
	$(LIBDIR)/utils.h \
	$(LIBDIR)/validator.h
if USE_OPENSSL_BACKEND
common += \
	$(LIBDIR)/crypto_openssl.c
else
common += \
	$(LIBDIR)/crypto_gpg.c
endif
if BUILD_CURL
common += $(LIBDIR)/soap.c $(LIBDIR)/soap.h
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
	$(LIBDIR)/utils.c \
	$(LIBDIR)/validator.c \
	$(LIBDIR)/cdecode.h \
	$(LIBDIR)/cencode.h \
	$(LIBDIR)/crypto.h \
	$(LIBDIR)/isds.h \
	$(LIBDIR)/isds_priv.h \
	$(LIBDIR)/physxml.h \
	$(LIBDIR)/utils.h \
	$(LIBDIR)/validator.h
if USE_OPENSSL_BACKEND
isds_common += \
	$(LIBDIR)/crypto_openssl.c
else
isds_common += \
	$(LIBDIR)/crypto_gpg.c
endif
if BUILD_CURL
isds_common += $(LIBDIR)/soap.c $(LIBDIR)/soap.h
endif
if WIN32
isds_common += $(LIBDIR)/win32.c $(LIBDIR)/win32.h
else
isds_common += $(LIBDIR)/unix.c $(LIBDIR)/unix.h
endif

# Omit soap.c
soap_common = ../test.c ../test.h ../test-tools.h \
	$(LIBDIR)/cdecode.c \
	$(LIBDIR)/cencode.c \
	$(LIBDIR)/isds.c \
	$(LIBDIR)/physxml.c \
	$(LIBDIR)/utils.c \
	$(LIBDIR)/validator.c \
	$(LIBDIR)/cdecode.h \
	$(LIBDIR)/cencode.h \
	$(LIBDIR)/crypto.h \
	$(LIBDIR)/isds.h \
	$(LIBDIR)/isds_priv.h \
	$(LIBDIR)/physxml.h \
	$(LIBDIR)/utils.h \
	$(LIBDIR)/validator.h
if USE_OPENSSL_BACKEND
soap_common += \
	$(LIBDIR)/crypto_openssl.c
else
soap_common += \
	$(LIBDIR)/crypto_gpg.c
endif
if BUILD_CURL
soap_common += $(LIBDIR)/soap.h
endif
if WIN32
soap_common += $(LIBDIR)/win32.c $(LIBDIR)/win32.h
else
soap_common += $(LIBDIR)/unix.c $(LIBDIR)/unix.h
endif

# Access _hidden symbols
b64decode_SOURCES 		= b64decode.c $(common)
b64encode_SOURCES 		= b64encode.c $(common)
compute_hash_SOURCES 	= compute_hash.c $(common)
context_SOURCES 		= context.c $(common)
guess_raw_type_SOURCES 	= guess_raw_type.c $(common)
init_crypto_SOURCES 	= init_crypto.c $(common)
isds_init_SOURCES 		= isds_init.c $(common)
load_raw_SOURCES 		= load_raw.c $(common)
normalize_mime_type_SOURCES 		= normalize_mime_type.c $(common)
utf82locale_SOURCES		= utf82locale.c $(common)
isds_list_free_SOURCES 	= isds_list_free.c $(common)
isds_pki_credentials_free_SOURCES 	= isds_pki_credentials_free.c $(common)
isds_hash_free_SOURCES 	= isds_hash_free.c $(common)
isds_PersonName_free_SOURCES 	= isds_PersonName_free.c $(common)
isds_BirthInfo_free_SOURCES 	= isds_BirthInfo_free.c $(common)
isds_Address_free_SOURCES 	= isds_Address_free.c $(common)
isds_DbOwnerInfo_free_SOURCES 	= isds_DbOwnerInfo_free.c $(common)
isds_DbUserInfo_free_SOURCES 	= isds_DbUserInfo_free.c $(common)
isds_event_free_SOURCES 	= isds_event_free.c $(common)
isds_envelope_free_SOURCES 	= isds_envelope_free.c $(common)
isds_document_free_SOURCES 	= isds_document_free.c $(common)
isds_message_free_SOURCES 	= isds_message_free.c $(common)
isds_message_copy_free_SOURCES 	= isds_message_copy_free.c $(common)
isds_message_status_change_free_SOURCES	= isds_message_status_change_free.c \
										  $(common)
isds_approval_free_SOURCES 	= isds_approval_free.c $(common)
isds_commercial_permission_free_SOURCES = isds_commercial_permission_free.c \
										  $(common)
isds_credentials_delivery_free_SOURCES 	= isds_credentials_delivery_free.c \
										  $(common)
isds_credit_event_free_SOURCES 	= isds_credit_event_free.c $(common)
isds_PersonName_duplicate_SOURCES	= isds_PersonName_duplicate.c $(common)
isds_Address_duplicate_SOURCES	= isds_Address_duplicate.c $(common)
isds_DbOwnerInfo_duplicate_SOURCES	= isds_DbOwnerInfo_duplicate.c $(common)
isds_DbUserInfo_duplicate_SOURCES	= isds_DbUserInfo_duplicate.c $(common)

# Access static symbols from isds.c
isds_dbtype_SOURCES 		= isds-dbtype.c $(isds_common)
isds_filemetatype_SOURCES 	= isds-filemetatype.c $(isds_common)
isds_hash_algorithm_SOURCES = isds-hash_algorithm.c $(isds_common)
isds_sendertype_SOURCES 	= isds-sendertype.c $(isds_common)
isds_usertype_SOURCES 		= isds-usertype.c $(isds_common)
isds_paymenttype_SOURCES 	= isds-paymenttype.c $(isds_common)
isds_crediteventtype_SOURCES 	= isds-crediteventtype.c $(isds_common)
isds_datestring2tm_SOURCES 	= isds-datestring2tm.c $(isds_common)
isds_tm2datestring_SOURCES 	= isds-tm2datestring.c $(isds_common)
isds_timestring2timeval_SOURCES = isds-timestring2timeval.c $(isds_common)
isds_timeval2timestring_SOURCES	= isds-timeval2timestring.c $(isds_common)
isds_uint2isds_message_status_SOURCES	= isds-uint2isds_message_status.c \
										  $(isds_common)
isds_eventstring2event_SOURCES	= isds-eventstring2event.c $(isds_common)

# Access static symbols from soap.c
soap_decode_header_value_SOURCES	= soap-decode_header_value.c $(soap_common)
soap_string2isds_otp_resolution_SOURCES	= soap-string2isds_otp_resolution.c \
										  $(soap_common)

# Tests that need .gnupg
guess_raw_type.log load_raw.log: prepare_environment.log

prepare_environment_SOURCES = prepare_environment.c \
							  ../test.c ../test.h ../test-tools.h

