include Makefile.inc

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
