LIBDIR=../src
CFLAGS=-std=c99 -Wall -Werror -g
CFLAGS+=-I$(LIBDIR)
LDFLAGS=-L$(LIBDIR) -lisds
BINS=isdsclient getsent getreceived

.PHONY: libisds depend

all: $(BINS)

libisds:
	$(MAKE) -C $(LIBDIR) all

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

isdsclient: isdsclient.o common.o
	$(CC) $(LDFLAGS) -o $@ $^

getsent: getsent.o common.o
	$(CC) $(LDFLAGS) -o $@ $^

getreceived: getreceived.o common.o
	$(CC) $(LDFLAGS) -o $@ $^

clean:
	-rm -f $(BINS) *.o

test-%: %
	LD_LIBRARY_PATH=$(LIBDIR) ./$<


depend:
	for F in *.c; do cpp $(CFLAGS) -MM "$$F"; done > Makefile.deps

include Makefile.deps
