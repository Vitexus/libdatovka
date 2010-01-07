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
	install -d $(DESTDIR)$(LIBDIR) $(INCLUDEDIR)
	install -t $(DESTDIR)$(LIBDIR) $(SONAME)
	ln -s $(SONAME) $(DESTDIR)$(LIBDIR)/libisds.so
	install -m 644 -t $(DESTDIR)$(INCLUDEDIR) isds.h

uninstall:
	-rm $(DESTDIR)$(LIBDIR)/$(SONAME) $(DESTDIR)$(LIBDIR)/libisds.so
	-rm $(DESTDIR)$(INCLUDEDIR)/isds.h

depend:
	#makedepend -- $(CFLAGS) -- $(SOURCES)
	for F in $(SOURCES); do cpp $(CFLAGS) -MM "$$F"; done > Makefile.deps

include Makefile.deps
