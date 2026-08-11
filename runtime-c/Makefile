# miniBox runtime (C port) - phase 1 build.
#   make            build all objects
#   make lib        build libminiboxhost.so
#   make test       run the memory-block self-test
CC ?= gcc
CFLAGS ?= -O2 -g -Wall -Wextra -Iinclude -std=c11 -fPIC
LDFLAGS ?=

SRCS = src/sha256.c src/pal_linux.c src/memblock.c src/tripguard.c \
       src/interop_bin.c src/context.c src/elf.c src/fs.c src/host.c src/cinterface.c
OBJS = $(SRCS:.c=.o)

.PHONY: all lib test clean
all: $(OBJS)

lib: libminiboxhost.so
libminiboxhost.so: $(OBJS)
	$(CC) -shared -o $@ $(OBJS) $(LDFLAGS)

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

test/test_memblock: test/test_memblock.c src/sha256.o src/pal_linux.o src/memblock.o src/tripguard.o
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

test: test/test_memblock
	./test/test_memblock

clean:
	rm -f $(OBJS) test/test_memblock libminiboxhost.so
