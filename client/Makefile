LIBDIR=../src
CFLAGS=-std=c99 -Wall -Werror -g
CFLAGS+=-I$(LIBDIR)
LDFLAGS=-L$(LIBDIR)

.PHONY: libisds depend

all: isdsclient

libisds:
	$(MAKE) -C $(LIBDIR) all

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

#isdsclient: isdsclient.o libisds
isdsclient: isdsclient.o common.o
	$(CC) $(LDFLAGS) -lisds -o $@ $^

clean:
	-rm -f isdsclient *.o

test: isdsclient
	LD_LIBRARY_PATH=$(LIBDIR) ./isdsclient

depend:
	for F in *.c; do cpp $(CFLAGS) -MM "$$F"; done > Makefile.deps

include Makefile.deps
