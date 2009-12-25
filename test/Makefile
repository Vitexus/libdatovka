include ../src/Makefile.inc
CFLAGS += -I../src
OBJS := $(wildcard ../src/*.o)
SRCS := $(wildcard *.c)
TESTS := $(SRCS:%.c=%.test)

%.o: %.c test.h
	$(CC) $(CFLAGS) $< -c -o $@

# First specific tests
isds-%.test: isds-%.o
	$(CC) $(LDFLAGS) $< $(filter-out ../src/isds.o,$(OBJS)) -o $@

# Then generic rule. Order is important.
%.test: %.o
	$(CC) $(LDFLAGS) $< $(OBJS) -o $@

all: $(TESTS)

test: $(TESTS)
	@FAILED=0; \
	for T in $^; do "./$$T" || FAILED=$$((FAILED+1)); done; \
	if [ "$$FAILED" -gt "0" ]; then \
		UNIT="unit tests"; \
		test "$$FAILED" == "1" && UNIT="unit test"; \
		echo "*** $$FAILED $$UNIT failed. Please tell developers about it."; \
		echo "Your report can make this world better!"; \
		echo "*** Environment info:"; \
		echo "Platform: $$(uname -msr)"; \
		echo "CFLAGS:   $(CFLAGS)"; \
		echo "LDFLAGS:  $(LDFLAGS)"; \
		echo "Compiler: $$($(CC) --version)"; \
		echo "libxml:   $$(pkg-config --modversion libxml-2.0)"; \
		echo "libcurl:  $$(pkg-config --modversion libcurl)"; \
		echo "libcrypt: $$(libgcrypt-config --version)"; \
		echo "ksba:     $$(ksba-config --version)"; \
		echo "gpgme:    $$(gpgme-config --version)"; \
		exit 1; \
	else \
		echo "*** All tests passed. Today's your lucky day :)"; \
		exit 0; \
	fi

clean:
	-rm *.test *.o
