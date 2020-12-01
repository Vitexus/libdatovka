PARAMS = --stringparam "man.justify" 1
PARAMS += --stringparam "man.hyphenate" 1
STYLE = @DOCBOOK_STYLE@/manpages/docbook.xsl
.PHONY: checkspell man-clean validate

man_SRC = libdatovka.xml
man_MANS = libdatovka.3 isds.h.libdatovka.3
EXTRA_DIST = $(man_SRC)

validate: $(man_SRC)
	xmllint --valid --noout $<

checkspell: $(man_SRC)
	aspell --lang=en --mode=sgml list < $< | sort -u > badlist

if BUILD_MAN
$(man_MANS): $(man_SRC)
	xsltproc $(PARAMS) $(STYLE) $<

man-clean:
	-rm -f $(man_MANS)
endif
