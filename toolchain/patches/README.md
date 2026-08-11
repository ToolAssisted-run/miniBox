# musl patches

Patches applied to the `toolchain/musl` submodule (a pristine checkout of
nattthebear/musl) by `build-toolchain.sh` before building. Kept as patch files
rather than a musl fork so the submodule stays pinned to the exact upstream
commit.

- `0001-waterbox-atomics-value-not-pointer.patch` - fixes `a_inc`/`a_dec` in the
  waterbox arch, which were `*p++`/`*p--` (increment the POINTER, discarding it)
  instead of `(*p)++`/`(*p)--`. Harmless for single-threaded guests, but with
  multiple guest threads it corrupts musl's `vmlock` counter (decremented on
  every `__vm_unlock`, never incremented), so a guest that frees memory while
  multithreaded deadlocks in `__vm_wait`. Found by miniBox's threaded guest
  test. (This bug is present in the upstream waterbox musl and affects BizHawk's
  Rust host too; reported for upstream.)
