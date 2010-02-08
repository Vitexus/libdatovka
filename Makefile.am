ACLOCAL_AMFLAGS = -I m4
AUTOMAKE_OPTIONS = check-news dist-bzip2
SUBDIRS = po src

if BUILD_EXAMPLE
SUBDIRS += client
endif

if BUILD_TEST
SUBDIRS += test
endif

EXTRA_DIST = config.rpath doc

dist-hook:
	git log --decorate=full > $(distdir)/ChangeLog
	git tag "v$(VERSION)" -s -m "Version $(VERSION) released on $(date -Imin)"
