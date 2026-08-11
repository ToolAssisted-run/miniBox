# The waterbox guest toolchain, exposed to meson consumers.
#
# The toolchain (musl in extern/, emulibc in extern/) is bootstrapped once by
# build-toolchain.sh (GCC path, no clang). This file does NOT rebuild it - it
# locates the built sysroot (at build/sysroot) and exports the pieces a consumer needs to compile
# a .wbx guest via a custom_target:
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

fs = import('fs')

waterbox_sysroot     = meson.project_source_root() / 'build' / 'sysroot'
waterbox_musl_gcc    = waterbox_sysroot / 'bin' / 'musl-gcc'
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

# Only musl needs the autotools bootstrap; emulibc.o is a meson build artifact
# (compiled with musl-gcc + the guest flags) so the guest side is meson-native.
waterbox_toolchain_ready = fs.exists(waterbox_musl_gcc)
if waterbox_toolchain_ready
  waterbox_emulibc_ct = custom_target('emulibc.c.o',
    input: waterbox_emulibc_dir / 'emulibc.c',
    output: 'emulibc.c.o',
    command: [waterbox_musl_gcc, '-c', waterbox_guest_cflags, '-std=c11', '-o', '@OUTPUT@', '@INPUT@'],
  )
  waterbox_emulibc_obj = waterbox_emulibc_ct
else
  waterbox_emulibc_obj = waterbox_emulibc_dir / 'obj' / 'release' / 'emulibc.c.o'  # placeholder
  message('Guest toolchain not bootstrapped; run source/guest/build-toolchain.sh ' +
          '(builds musl with gcc). Guest .wbx targets will be unavailable.')
endif

# Emit a cross-file for consumers who want to build the guest as a cross project.
if waterbox_toolchain_ready
  configure_file(
    input: 'waterbox-guest.ini.in',
    output: 'waterbox-guest.ini',
    configuration: {
      'MUSL_GCC': waterbox_musl_gcc,
      'EMULIBC_INC': waterbox_emulibc_dir,
      'EMULIBC_OBJ': waterbox_emulibc_ct.full_path(),
      'LINKSCRIPT': waterbox_linkscript,
    },
  )
endif
