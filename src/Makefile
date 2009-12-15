CFLAGS = -std=c99 -Wall -Werror -fPIC -DPIC -g
SONAME = libisds.so.0
SOURCES = isds.c utils.c soap.c validator.c cencode.c cdecode.c crypto.c
PREFIX=/usr/local
LIBDIR=$(PREFIX)/lib
INCLUDEDIR=$(PREFIX)/include

LIBXML_CFLAGS := $(shell pkg-config --cflags libxml-2.0)
LIBCURL_CFLAGS := $(shell pkg-config --cflags libcurl)
LIBGCRYPT_CFLAGS := $(shell libgcrypt-config --cflags)
CFLAGS += $(LIBCURL_CFLAGS) $(LIBXML_CFLAGS) $(LIBGCRYPT_CFLAGS)

LIBXML_LDFLAGS := $(shell pkg-config --libs libxml-2.0)
LIBCURL_LDFLAGS := $(shell pkg-config --libs libcurl)
LIBGCRYPT_LDFLAGS := $(shell libgcrypt-config --libs)
LDFLAGS += $(LIBCURL_LDFLAGS) $(LIBXML_LDFLAGS) $(LIBGCRYPT_LDFLAGS)


all: $(SONAME) libisds.so

libisds.so: $(SONAME)
	ln -s $(SONAME) $@

$(SONAME): $(SOURCES:%.c=%.o)
	$(CC) $(CFLAGS) $(LDFLAGS) -shared -Wl,-soname=$(SONAME) -o $@ $^

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
	-rm -f libisds.so $(SONAME) *.o

install: all
	install -d $(LIBDIR) $(INCLUDEDIR)
	install -t $(LIBDIR) $(SONAME)
	ln -s $(SONAME) $(LIBDIR)/libisds.so
	install -m 644 -t $(INCLUDEDIR) isds.h

uninstall:
	-rm $(LIBDIR)/$(SONAME) $(LIBDIR)/libisds.so
	-rm $(INCLUDEDIR)/isds.h

depend:
	#makedepend -- $(CFLAGS) -- $(SOURCES)
	for F in $(SOURCES); do cpp $(CFLAGS) -MM "$$F"; done > Makefile.deps

include Makefile.deps
