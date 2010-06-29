ACLOCAL_AMFLAGS = -I m4
AUTOMAKE_OPTIONS = check-news dist-bzip2
SUBDIRS = po src

if BUILD_EXAMPLE
SUBDIRS += client
endif

if BUILD_TEST
SUBDIRS += test
endif

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = $(PACKAGE).pc

EXTRA_DIST = config.rpath doc server

dist-hook:
	git log --decorate=full > $(distdir)/ChangeLog || echo "Could not dump git log"
	git tag "v$(VERSION)" -s -m "Version $(VERSION) released on $$(date -Imin)" || true
