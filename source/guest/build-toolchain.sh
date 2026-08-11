#!/bin/sh
# Bootstraps musl (extern/musl, the guest libc) with GCC into source/guest/sysroot. This is the
# one-time toolchain build a core author runs before meson can compile any .wbx
# guest. emulibc.o is built by meson itself (source/guest/meson.build), so only
# musl - which has its own autotools build - is bootstrapped here.
#
# GCC-only path: no clang, no lld, no LLVM. (libcxx/ is only needed for C++
# guests and is not built here.)
set -e
here="$(cd "$(dirname "$0")" && pwd)"
musl="$here/../../extern/musl"
export SYSROOT="$here/sysroot"
export CC="${CC:-gcc}"

# Apply our musl patches (idempotent) before building. The submodule stays
# pinned to pristine upstream; these are the miniBox-specific fixes.
for p in "$here"/patches/*.patch; do
	[ -e "$p" ] || continue
	if git -C "$musl" apply --reverse --check "$p" >/dev/null 2>&1; then
		echo "patch already applied: $(basename "$p")"
	else
		echo "applying patch: $(basename "$p")"
		git -C "$musl" apply "$p"
	fi
done

if [ ! -f "$SYSROOT/lib/libc.a" ]; then
	echo "building musl (CC=$CC) -> $SYSROOT ..."
	( cd "$musl" && ./wbox_configure.sh && ./wbox_build.sh )
else
	echo "musl already built ($SYSROOT/lib/libc.a); skipping"
fi

echo "toolchain ready: $SYSROOT/bin/musl-gcc (emulibc.o is built by meson)"
