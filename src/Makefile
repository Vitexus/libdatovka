CFLAGS=-std=c99 -Wall -Werror -lcurl
SONAME=isds

all: lib$(SONAME).so

lib$(SONAME).so: isds.c isds.h
	$(CC) $(CFLAGS) -fPIC -DPIC -shared -Wl,-soname=$(SO_NAME) -o $@ $<

clean:
	-rm -f lib$(SONAME).so
