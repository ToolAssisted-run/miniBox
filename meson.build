# miniBox - a determinism sandbox (the waterbox for miniHawk).
#
# This top-level project builds the host runtime and wires the tests. The guest
# toolchain (musl + emulibc) and guest cores are built through the waterbox-guest
# cross machinery under source/guest/ (see source/guest/meson.build and
# source/guest/waterbox-guest.ini.in), which is separate because a guest is compiled
# for the sandbox's fake machine, not the host.
#
# As a subproject:
#   minibox = subproject('miniBox')
#   dep = minibox.get_variable('minibox_host_dep')
project('miniBox', 'c',
  version: '0.1.0',
  license: 'MIT',
  default_options: ['c_std=c11', 'warning_level=2', 'default_library=both'],
)

subdir('source/host')

# ---------------------------------------------------------------------------
# Guest toolchain: build the vendored musl (extern/musl) into the build dir.
#
# musl is autotools (configure + make install), so it is driven here at the
# top level - where a custom_target's working directory equals the build root,
# which is what musl's out-of-tree `configure` writes into. Everything lands
# under the build dir; there is no separate bootstrap script and no manual step.
#
# The guest libc is always built with the host gcc (the guest is a fake-machine
# target, not a host binary), so this is skipped for cross builds (e.g. the
# Windows host-DLL compile-check), which do not build or run guests.
# ---------------------------------------------------------------------------
build_guest_toolchain = not meson.is_cross_build()

if build_guest_toolchain
  waterbox_sysroot = meson.current_build_dir() / 'guest-sysroot'
  musl_src         = meson.current_source_dir() / 'extern' / 'musl'
  # The frozen musl CFLAGS (from extern/musl/wbox_configure.sh).
  musl_cflags = '-Werror -fvisibility=hidden -mcmodel=large ' + \
                '-mstack-protector-guard=global -fno-pic -fno-pie -fcf-protection=none'

  # Phase 1: configure musl out-of-tree. Writes config.mak + a Makefile symlink
  # into the build root (the custom_target cwd).
  musl_config = custom_target('musl-config',
    output: 'config.mak',
    command: [
      find_program(musl_src / 'configure'),
      '--srcdir=' + musl_src,
      '--target=waterbox', '--build=waterbox', '--disable-shared',
      '--prefix=' + waterbox_sysroot,
      '--bindir=' + meson.current_build_dir(),      # musl-gcc -> a trackable build-root path
      '--libdir=' + waterbox_sysroot / 'lib',
      '--includedir=' + waterbox_sysroot / 'include',
      '--syslibdir=' + waterbox_sysroot / 'syslib',
    ],
    env: {'CC': 'gcc', 'CFLAGS': musl_cflags},
  )

  # Phase 2: build + install musl (reads config.mak from the same build root).
  # The wrapper lands at <build>/musl-gcc; headers/libs under guest-sysroot/.
  waterbox_musl_gcc = custom_target('musl-gcc',
    input: musl_config,
    output: 'musl-gcc',
    command: [find_program('make'), '-j', 'install'],
    console: true,
  )
endif

subdir('source/guest')   # exposes the waterbox_guest_* variables (guest .wbx build)

# Tests are opt-in for subproject consumers; on by default for a standalone build.
if not meson.is_subproject()
  subdir('tests')
endif
