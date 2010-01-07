LIBDIR=../src
CFLAGS=-std=c99 -Wall -Werror -g
# Large files needed by GPGME
CFLAGS += -D_FILE_OFFSET_BITS=64 -DLARGEFILE_SOURCE=1
CFLAGS+=-I$(LIBDIR)
LDFLAGS=-L$(LIBDIR) -lisds
BINS=isdsclient getsent getsignedsent \
	 loadreceivedmessage getreceived getsignedreceived \
	 getreceivedenvelope markasread \
	 downloadmessagehash verifyreceivedhash computemessagehash \
	 loaddelivery getreceiveddelivery getreceivedsigneddelivery

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

getsignedsent: getsignedsent.o common.o
	$(CC) $(LDFLAGS) -o $@ $^

loadreceivedmessage: loadreceivedmessage.o common.o
	$(CC) $(LDFLAGS) -o $@ $^

getreceived: getreceived.o common.o
	$(CC) $(LDFLAGS) -o $@ $^

getsignedreceived: getsignedreceived.o common.o
	$(CC) $(LDFLAGS) -o $@ $^

getreceivedenvelope: getreceivedenvelope.o common.o
	$(CC) $(LDFLAGS) -o $@ $^

downloadmessagehash: downloadmessagehash.o common.o
	$(CC) $(LDFLAGS) -o $@ $^

verifyreceivedhash: verifyreceivedhash.o common.o
	$(CC) $(LDFLAGS) -o $@ $^

computemessagehash: computemessagehash.o common.o
	$(CC) $(LDFLAGS) -o $@ $^

markasread: markasread.o common.o
	$(CC) $(LDFLAGS) -o $@ $^

loaddelivery: loaddelivery.o common.o
	$(CC) $(LDFLAGS) -o $@ $^

getreceiveddelivery: getreceiveddelivery.o common.o
	$(CC) $(LDFLAGS) -o $@ $^

getreceivedsigneddelivery: getreceivedsigneddelivery.o common.o
	$(CC) $(LDFLAGS) -o $@ $^


clean:
	-rm -f $(BINS) *.o output

test-%: %
	LD_LIBRARY_PATH=$(LIBDIR) ./$<


depend:
	for F in *.c; do cpp $(CFLAGS) -MM "$$F"; done > Makefile.deps

include Makefile.deps
