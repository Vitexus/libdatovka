AUTOMAKE_OPTIONS = color-tests parallel-tests
export VERBOSE=1

SUBDIRS = offline simline
if BUILD_ONLINE_TEST
SUBDIRS += online
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

# To fake home dir for gnupg2
export HOME=@top_builddir@

