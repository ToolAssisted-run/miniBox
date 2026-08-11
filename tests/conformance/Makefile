# Build the conformance guest (guest.wbx) with the gcc toolchain path only -
# no clang, no lld, no LLVM libc++/compiler-rt. Demonstrates that a pure-C
# waterbox guest builds with gcc + musl-gcc + GNU ld through the linkscript.
#
# Prereq: the toolchain is built (toolchain/musl via wbox_build.sh, and
# toolchain/emulibc via make). Point TOOLCHAIN at it (default: ../../toolchain).
TOOLCHAIN ?= $(abspath ../../toolchain)
SYSROOT   := $(TOOLCHAIN)/sysroot
MUSL_GCC  := $(SYSROOT)/bin/musl-gcc
EMULIBC   := $(TOOLCHAIN)/emulibc
LINKSCRIPT := $(TOOLCHAIN)/linkscript.T

# The waterbox guest flags (from common.mak COMMONFLAGS), minus C++/LTO.
CFLAGS := -fvisibility=hidden -I$(EMULIBC) -Wall -mcmodel=large \
	-mstack-protector-guard=global -fno-pic -fno-pie -fcf-protection=none \
	-O2 -DNDEBUG -std=c11

# Link: static, non-PIE, the linkscript, emulibc's object. -lgcc supplies any
# compiler builtins (this guest needs none, but it is the gcc-path analogue of
# the clang path's -lclang_rt.builtins).
LDFLAGS := -static -no-pie -Wl,--eh-frame-hdr,-O2 -T $(LINKSCRIPT)
EMULIBC_OBJ := $(EMULIBC)/obj/release/emulibc.c.o

guest.wbx: guest.c $(EMULIBC_OBJ)
	$(MUSL_GCC) -o $@ $(CFLAGS) $(LDFLAGS) guest.c $(EMULIBC_OBJ) -lgcc

$(EMULIBC_OBJ):
	$(MAKE) -C $(EMULIBC)

.PHONY: clean
clean:
	rm -f guest.wbx
