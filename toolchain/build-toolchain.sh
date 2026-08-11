#!/bin/sh
# Bootstraps musl (the guest libc) with GCC into toolchain/sysroot. This is the
# one-time toolchain build a core author runs before meson can compile any .wbx
# guest. emulibc.o is built by meson itself (toolchain/meson.build), so only
# musl - which has its own autotools build - is bootstrapped here.
#
# GCC-only path: no clang, no lld, no LLVM. (libcxx/ is only needed for C++
# guests and is not built here.)
set -e
here="$(cd "$(dirname "$0")" && pwd)"
export SYSROOT="$here/sysroot"
export CC="${CC:-gcc}"

if [ ! -f "$SYSROOT/lib/libc.a" ]; then
	echo "building musl (CC=$CC) -> $SYSROOT ..."
	( cd "$here/musl" && ./wbox_configure.sh && ./wbox_build.sh )
else
	echo "musl already built ($SYSROOT/lib/libc.a); skipping"
fi

echo "toolchain ready: $SYSROOT/bin/musl-gcc (emulibc.o is built by meson)"
