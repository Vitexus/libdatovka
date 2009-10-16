CFLAGS=-std=c99 -Wall -Werror -lcurl
SONAME=libisds.so.0

all: $(SONAME) libisds.so

libisds.so: $(SONAME)
	ln -s $(SONAME) $@

$(SONAME): isds.o utils.o
	$(CC) $(CFLAGS) -fPIC -DPIC -shared -Wl,-soname=$(SONAME) -o $@ $^

%.o: %.c %.h
	$(CC) $(CFLAGS) -fPIC -DPIC -c -o $@ $<


clean:
	-rm -f libisds.so $(SONAME) *.o
