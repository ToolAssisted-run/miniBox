# miniBox

The waterbox for miniHawk: a determinism sandbox in which ALL miniHawk cores
run (miniHawk is waterbox-only by design - see the miniHawk repository's
docs/design-principles.md, "THE WATERBOX-ONLY REDESIGN", and
docs/waterbox-analysis.md for the full architecture analysis).

A guest core is a static, fixed-base x86-64 ELF (".wbx") compiled against a
retargeted musl libc whose syscalls are plain calls into the host. The host
implements a FROZEN, VERSIONED machine specification: fixed address layout,
a deterministic syscall surface (constant clock, no entropy, no host
filesystem), page-granular dirty tracking against a sealed post-init
baseline, and whole-machine savestates. The same .wbx runs bit-identically
on every host OS.

Derived from the BizHawk project's waterbox
(github.com/TASEmulators/BizHawk). miniBox combines several upstream projects,
each under its own permissive license (BizHawk's waterbox - MIT; musl libc -
MIT, Rich Felker et al.; libco - public domain, byuu; LLVM libc++ etc. -
Apache-2.0 w/ LLVM exception, built not vendored). See LICENSE and
ATTRIBUTION.md for the full component-by-component breakdown.

## Layout

- `host/` - the sandbox host, in C. This is the product: builds
  libminiboxhost on Linux (validated) and Windows (cross-compiled via
  mingw-w64; see cross/mingw-w64.ini). It is a from-scratch C port of
  BizHawk's Rust waterboxhost (which remains the historical reference,
  available in the BizHawk repo and in this repo's git history); no Rust or
  nightly toolchain is needed to build or use miniBox.
- `toolchain/` - the guest toolchain (the core-author kit): the waterbox-arch
  musl fork (submodule, nattthebear/musl @ 2063abc4), emulibc (ECL_* macros,
  sealed/invisible/plain allocators, __wbxsysinfo), libco (cothreads),
  libcxx build scripts (LLVM sysroot with random_device/tz compiled out),
  linkscript.T (fixed base 0x36f00000000, .sealed/.invis sections), and
  common.mak.
- `managed/` - the C# host layer as imported from BizHawk (WaterboxHost,
  the wbx_* ABI declarations, the WaterboxCore base and guest ABI). To be
  adapted into miniHawk's generic-adapter model.
- `docs/` - imported notes.

## Consumption

miniHawk consumes this repository as a submodule at `extern/miniBox` and
builds the runtime with its own meson dual-target arrangement, shipping it
with the frontend's OS-dependent artifacts. Core authors use `toolchain/` to
compile their emulation source into a platform-neutral `.wbx`.

## Roadmap

1. C/C++ port of the runtime (drops the nightly-Rust requirement; validated
   against the Rust reference over shared guests and inputs).
   Phase 1 DONE (host/): memory block + dirty tracking + savestates +
   ELF load + interop/thunks + VFS + the syscall core; links as
   libminiboxhost.so (all 16 wbx_* exports, no external deps). A gcc-built
   conformance guest (tests/conformance/) runs end-to-end - Init, guest
   syscalls, sealed/invisible memory, seal, and a savestate round-trip all
   pass. GCC-only: no clang/lld/LLVM needed for pure-C guests.
   Phase 2 IN PROGRESS: green threads/futex/clone DONE (threading.c;
   cooperative scheduler, futex WAIT/WAKE/REQUEUE/PI, clone, thread-set
   savestate) - validated with a pthreaded guest (mutex + condvar + join +
   savestate). Remaining: the Windows PAL (VEH fault handler + guard-page
   stack tracking + CreateFileMapping) and the MsHostSysVGuest win64<->sysv64
   ABI adapter - written from the Rust reference but only runtime-validatable
   on Windows; cothread support for C++ cores.
2. Guest ABI v1 + conformance tests, runnable entirely without miniHawk.
3. Machine specification document - the frozen contract both implementations
   are held to.
