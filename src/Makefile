CFLAGS=-std=c99 -Wall -Werror -fPIC -DPIC
SONAME=libisds.so.0

all: $(SONAME) libisds.so

libisds.so: $(SONAME)
	ln -s $(SONAME) $@

$(SONAME): isds.o utils.o soap.o
	$(CC) $(CFLAGS) -lcurl -shared -Wl,-soname=$(SONAME) -o $@ $^

%.o: %.c %.h
	$(CC) $(CFLAGS) -c -o $@ $<


clean:
	-rm -f libisds.so $(SONAME) *.o
