CFLAGS=-std=c99 -Wall -Werror -fPIC -DPIC
SONAME=libisds.so.0
SOURCES=isds.c utils.c soap.c

#include Makefile.deps

all: $(SONAME) libisds.so

libisds.so: $(SONAME)
	ln -s $(SONAME) $@

$(SONAME): $(SOURCES:%.c=%.o)
	$(CC) $(CFLAGS) -lcurl -shared -Wl,-soname=$(SONAME) -o $@ $^

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

depend:
	makedepend -- $(CFLAGS) -- $(SOURCES)

clean:
	-rm -f libisds.so $(SONAME) *.o
