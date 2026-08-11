# miniBox - a determinism sandbox (the waterbox for miniHawk).
#
# This top-level project builds the host runtime and wires the tests. The guest
# toolchain (musl + emulibc) and guest cores are built through the waterbox-guest
# cross machinery under toolchain/ (see toolchain/meson.build and
# toolchain/waterbox-guest.ini.in), which is separate because a guest is compiled
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

subdir('host')
subdir('toolchain')   # exposes the waterbox_guest_* variables (guest .wbx build)

# Tests are opt-in for subproject consumers; on by default for a standalone build.
if not meson.is_subproject()
  subdir('tests')
endif
