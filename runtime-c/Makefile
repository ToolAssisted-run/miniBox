# miniBox runtime (C port) - phase 1 build.
# Targets: `make test` runs the standalone memory-block self-test.
CC ?= gcc
CFLAGS ?= -O2 -g -Wall -Wextra -Iinclude -std=c11
LDFLAGS ?=

SRCS = src/sha256.c src/pal_linux.c src/memblock.c src/tripguard.c
OBJS = $(SRCS:.c=.o)

.PHONY: all test clean
all: $(OBJS)

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

test/test_memblock: test/test_memblock.c $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

test: test/test_memblock
	./test/test_memblock

clean:
	rm -f $(OBJS) test/test_memblock
