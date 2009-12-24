include ../src/Makefile.inc
CFLAGS += -I../src
OBJS := $(wildcard ../src/*.o)
SRCS := $(wildcard *.c)
TESTS := $(SRCS:%.c=%.test)

%.o: %.c test.h
	$(CC) $(CFLAGS) $< -c -o $@
%.test: %.o
	$(CC) $(LDFLAGS) $< $(OBJS) -o $@

dbtype.test: dbtype.o
	$(CC) $(LDFLAGS) $< $(filter-out ../src/isds.o,$(OBJS)) -o $@

all: $(TESTS)

test: $(TESTS)
	@FAILED=0; \
	for T in $^; do "./$$T" || FAILED=$$((FAILED+1)); done; \
	if [ "$$FAILED" -gt "0" ]; then \
		UNIT="unit tests"; \
		test "$$FAILED" == "1" && UNIT="unit test"; \
		echo "*** $$FAILED $$UNIT failed. Please tell developers about it."; \
		echo "Your report can make this world better!"; \
		exit 1; \
	else \
		echo "*** All tests passed. Today's your lucky day :)"; \
		exit 0; \
	fi

clean:
	-rm *.test *.o
