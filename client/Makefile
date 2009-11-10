LIBDIR=../src
CFLAGS=-std=c99 -Wall -Werror -g
CFLAGS+=-I$(LIBDIR)
LDFLAGS=-L$(LIBDIR)

.PHONY: libisds

all: isdsclient

libisds:
	$(MAKE) -C $(LIBDIR) all

isdsclient.o: isdsclient.c
	$(CC) $(CFLAGS) -c -o $@ $<

#isdsclient: isdsclient.o libisds
isdsclient: isdsclient.o
	$(CC) $(LDFLAGS) -lisds -o $@ $<

clean:
	-rm -f isdsclient isdsclient.o

test: isdsclient
	LD_LIBRARY_PATH=$(LIBDIR) ./isdsclient
