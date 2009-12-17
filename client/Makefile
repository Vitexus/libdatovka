LIBDIR=../src
CFLAGS=-std=c99 -Wall -Werror -g
# Large files needed by GPGME
CFLAGS += -D_FILE_OFFSET_BITS=64 -DLARGEFILE_SOURCE=1
CFLAGS+=-I$(LIBDIR)
LDFLAGS=-L$(LIBDIR) -lisds
BINS=isdsclient getsent getreceived getsignedreceived verifyreceivedhash

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

getsignedreceived: getsignedreceived.o common.o
	$(CC) $(LDFLAGS) -o $@ $^

verifyreceivedhash: verifyreceivedhash.o common.o
	$(CC) $(LDFLAGS) -o $@ $^

clean:
	-rm -f $(BINS) *.o

test-%: %
	LD_LIBRARY_PATH=$(LIBDIR) ./$<


depend:
	for F in *.c; do cpp $(CFLAGS) -MM "$$F"; done > Makefile.deps

include Makefile.deps
