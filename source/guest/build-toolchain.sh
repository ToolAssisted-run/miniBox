#!/bin/sh
# Bootstraps musl (extern/musl, the guest libc) with GCC into <repo>/build/sysroot. This is the
# one-time toolchain build a core author runs before meson can compile any .wbx
# guest. emulibc.o is built by meson itself (source/guest/meson.build), so only
# musl - which has its own autotools build - is bootstrapped here.
#
# GCC-only path: no clang, no lld, no LLVM. (libcxx/ is only needed for C++
# guests and is not built here.)
set -e
here="$(cd "$(dirname "$0")" && pwd)"
root="$(cd "$here/../.." && pwd)"
musl="$root/extern/musl"
export SYSROOT="$root/build/sysroot"
export CC="${CC:-gcc}"

# extern/musl is vendored (not a submodule): the waterbox-retargeted musl fork
# with miniBox's fixes already applied in the source tree. See ATTRIBUTION.md
# for the divergence from pristine upstream. Nothing to patch here anymore.

if [ ! -f "$SYSROOT/lib/libc.a" ]; then
	echo "building musl (CC=$CC) -> $SYSROOT ..."
	( cd "$musl" && ./wbox_configure.sh && ./wbox_build.sh )
else
	echo "musl already built ($SYSROOT/lib/libc.a); skipping"
fi

echo "toolchain ready: $SYSROOT/bin/musl-gcc (emulibc.o is built by meson)"
