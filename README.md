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

- `runtime/` - the sandbox host (from BizHawk's waterboxhost, Rust). This is
  the REFERENCE implementation; the plan of record is a C/C++ port (see
  Roadmap) validated differentially against it, after which this becomes the
  cross-check implementation.
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
   bit-exactly against the Rust reference over shared guests and inputs).
   Phase 1: memory block + dirty tracking + savestates + ELF load + syscall
   core (enough for single-threaded C guests, e.g. miniHawk's synth core).
   Phase 2: green threads/futex, VFS extras, cothread support (real cores).
2. Guest ABI v1 + conformance tests, runnable entirely without miniHawk.
3. Machine specification document - the frozen contract both implementations
   are held to.
