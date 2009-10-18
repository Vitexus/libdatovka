CFLAGS = -std=c99 -Wall -Werror -fPIC -DPIC -g
SONAME = libisds.so.0
SOURCES = isds.c utils.c soap.c

LIBXML_CFLAGS := $(shell pkg-config --cflags libxml-2.0)
LIBCURL_CFLAGS := $(shell pkg-config --cflags libcurl)
CFLAGS += $(LIBCURL_CFLAGS) $(LIBXML_CFLAGS)

LIBXML_LDFLAGS := $(shell pkg-config --libs libxml-2.0)
LIBCURL_LDFLAGS := $(shell pkg-config --libs libcurl)
LDFLAGS += $(LIBCURL_LDFLAGS) $(LIBXML_LDFLAGS)


all: $(SONAME) libisds.so

libisds.so: $(SONAME)
	ln -s $(SONAME) $@

$(SONAME): $(SOURCES:%.c=%.o)
	$(CC) $(CFLAGS) $(LDFLAGS) -shared -Wl,-soname=$(SONAME) -o $@ $^

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
	-rm -f libisds.so $(SONAME) *.o

depend:
	#makedepend -- $(CFLAGS) -- $(SOURCES)
	for F in $(SOURCES); do cpp $(CFLAGS) -MM "$$F"; done > Makefile.deps

include Makefile.deps
