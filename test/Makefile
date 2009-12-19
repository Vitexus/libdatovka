include ../src/Makefile.inc
CFLAGS += -I../src
OBJS := $(wildcard ../src/*.o)
SRCS := $(wildcard *.c)
TESTS := $(SRCS:%.c=%.test)

%.o: %.c test.h
	$(CC) $(CFLAGS) $< -c -o $@
%.test: %.o
	$(CC) $(LDFLAGS) $< $(OBJS) -o $@

all: $(TESTS)

test: $(TESTS)
	for T in $^; do "./$$T" ; done

clean:
	-rm *.test *.o
