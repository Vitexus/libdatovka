CFLAGS=-std=c99 -Wall -Werror -fPIC -DPIC
SONAME=libisds.so.0
SOURCES=isds.c utils.c soap.c


all: $(SONAME) libisds.so

libisds.so: $(SONAME)
	ln -s $(SONAME) $@

$(SONAME): $(SOURCES:%.c=%.o)
	$(CC) $(CFLAGS) -lcurl -shared -Wl,-soname=$(SONAME) -o $@ $^

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
	-rm -f libisds.so $(SONAME) *.o

depend:
	#makedepend -- $(CFLAGS) -- $(SOURCES)
	for F in $(SOURCES); do cpp $(CFLAGS) -MM "$$F"; done > Makefile.deps

include Makefile.deps
