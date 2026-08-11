# Attribution and third-party licenses

miniBox is assembled from several upstream projects, each retaining its own
authorship and license. This file inventories every component, who authored it,
and under what terms. All components are permissively licensed and mutually
compatible; the combined work and miniBox's own new code are provided under the
MIT License (see LICENSE).

## Components vendored in this repository

### The waterbox (BizHawk) - MIT

The sandbox design and the bulk of the imported code come from the waterbox
subsystem of BizHawk (https://github.com/TASEmulators/BizHawk), Copyright (c)
the BizHawk team and contributors, MIT License. This covers:

- `runtime/` - the sandbox host (originally `waterbox/waterboxhost`, Rust).
- `runtime/src/context/interop.s` and `interop.bin` - the stack-switch blob.
- `toolchain/emulibc/` - the guest support library (ECL_* macros, allocators,
  `__wbxsysinfo`).
- `toolchain/libcxx/` - the LLVM sysroot build scripts (the scripts are
  BizHawk's; the libraries they build are LLVM's, see below).
- `toolchain/linkscript.T`, `toolchain/common.mak` - the guest build machinery.
- `toolchain/libco/amd64.c` - the waterbox-specific rewrite of libco's amd64
  backend (derived from byuu's libco, see below).
- `docs/Notes on Debugging.md`.

### musl libc - MIT (Rich Felker and contributors)

`toolchain/musl` is a git submodule of a waterbox-retargeted fork of musl libc
(https://github.com/nattthebear/musl, forked from https://musl.libc.org),
Copyright (c) 2005-2020 Rich Felker and the musl contributors, MIT License. The
full text and complete author list are in that submodule's `COPYRIGHT` file. The
waterbox-arch additions in the fork (the `waterbox` architecture directory, the
syscall trampoline glue) are contributed under the same MIT terms.

### libco - public domain (byuu)

`toolchain/libco/libco.h` and the original libco design are by byuu (Near),
released into the public domain (see the header of each file: "license: public
domain"). `toolchain/libco/amd64.c` is the waterbox rewrite of the amd64 backend
(BizHawk, MIT), built on that public-domain base.

## Components fetched or built at build time (NOT vendored here)

### LLVM libc++ / libc++abi / libunwind / compiler-rt - Apache-2.0 WITH LLVM exception

`toolchain/libcxx/*.sh` sparse-clone the LLVM project
(https://github.com/llvm/llvm-project) at a pinned tag and build the guest C++
standard library, unwinder, and compiler builtins. Those sources are Copyright
(c) the LLVM contributors, licensed Apache-2.0 with the LLVM exception. They are
downloaded during a guest toolchain build, not stored in this repository; the
license applies to the built guest artifacts.

### Rust crate dependencies of the runtime - MIT / Apache-2.0

`runtime/Cargo.toml` depends on third-party crates (bitflags, page_size,
lazy_static, itertools, goblin, anyhow, sha2, and platform crates winapi / libc),
each MIT or Apache-2.0 licensed by its respective authors. They are fetched by
cargo at build time and pinned in `Cargo.lock`, not vendored here. The planned
C/C++ port of the runtime removes these dependencies.

## miniBox new work - MIT (Sergio Martin, 2026)

New code authored for miniBox - the machine specification (MACHINE-SPEC.md), the
C/C++ runtime port, the reorganization, conformance tests, and build glue - is
Copyright (c) 2026 Sergio Martin, MIT License.
