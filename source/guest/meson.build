# The waterbox guest toolchain, exposed to meson consumers.
#
# musl (extern/musl, the guest libc) is built into the build dir by the
# top-level meson.build; this file builds emulibc against it and exports the
# pieces a consumer needs to compile a .wbx guest via a custom_target:
#
#   minibox = subproject('miniBox')
#   guest = custom_target('core.wbx',
#     input: 'core.c', output: 'core.wbx',
#     command: [ minibox.get_variable('waterbox_musl_gcc'),
#                minibox.get_variable('waterbox_guest_cflags'),
#                minibox.get_variable('waterbox_guest_ldflags'),
#                '-o', '@OUTPUT@', '@INPUT@',
#                minibox.get_variable('waterbox_emulibc_obj'), '-lgcc' ])
#
# A generated cross-file (waterbox-guest.ini) is also emitted for consumers that
# prefer building the guest as a separate cross-compiled meson project.
#
# All of this is native-only: a guest targets the fake machine and is built with
# the host gcc, so it is skipped for cross builds (build_guest_toolchain is set
# in the top-level meson.build).

if not build_guest_toolchain
  subdir_done()
endif

waterbox_emulibc_dir = meson.project_source_root() / 'extern' / 'emulibc'
waterbox_linkscript  = meson.current_source_dir() / 'linkscript.T'

# The frozen guest flags (BizHawk waterbox COMMONFLAGS; C, no C++/LTO).
waterbox_guest_cflags = [
  '-fvisibility=hidden', '-I' + waterbox_emulibc_dir, '-Wall', '-mcmodel=large',
  '-mstack-protector-guard=global', '-fno-pic', '-fno-pie', '-fcf-protection=none',
  '-O2', '-DNDEBUG',
]
waterbox_guest_ldflags = [
  '-static', '-no-pie', '-Wl,--eh-frame-hdr,-O2', '-T', waterbox_linkscript,
]

# emulibc.o is a meson build artifact, compiled with the just-built musl-gcc.
# Passing the musl-gcc custom_target as command[0] makes ninja build musl first.
waterbox_emulibc_ct = custom_target('emulibc.c.o',
  input: waterbox_emulibc_dir / 'emulibc.c',
  output: 'emulibc.c.o',
  command: [waterbox_musl_gcc, '-c', waterbox_guest_cflags, '-std=c11', '-o', '@OUTPUT@', '@INPUT@'],
)
waterbox_emulibc_obj = waterbox_emulibc_ct

# Emit a cross-file for consumers who want to build the guest as a cross project.
configure_file(
  input: 'waterbox-guest.ini.in',
  output: 'waterbox-guest.ini',
  configuration: {
    'MUSL_GCC': waterbox_musl_gcc.full_path(),
    'EMULIBC_INC': waterbox_emulibc_dir,
    'EMULIBC_OBJ': waterbox_emulibc_ct.full_path(),
    'LINKSCRIPT': waterbox_linkscript,
  },
)
