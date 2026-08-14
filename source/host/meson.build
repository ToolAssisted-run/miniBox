# The miniBox host runtime (C port). Builds libminiboxhost and exposes a meson
# dependency so consumers (miniHawk, core packages) can use it as a subproject:
#
#   minibox = subproject('miniBox')
#   minibox_host_dep = minibox.get_variable('minibox_host_dep')
#
# Phase 1: Linux x86-64, single guest thread, C guests.

minibox_host_inc = include_directories('.')

minibox_host_sources = files(
  'sha256.c',
  'memblock.c',
  'diag.c',
  'tripguard.c',
  'interop_bin.c',
  'context.c',
  'elf.c',
  'fs.c',
  'host.c',
  'threading.c',
  'cinterface.c',
  'calling_convention_adapters.c',
)

# The PAL is platform-specific (each file self-guards with #ifdef, so listing
# both is also fine, but select the right one for clarity).
if host_machine.system() == 'windows'
  minibox_host_sources += files('pal_win.c')
else
  minibox_host_sources += files('pal_linux.c')
endif

# -fno-strict-aliasing: the memory block reinterprets guest bytes freely through
# the mirror; keep the optimizer honest about that.
minibox_host_c_args = ['-fno-strict-aliasing']

libminiboxhost = both_libraries(
  'miniboxhost',
  minibox_host_sources,
  include_directories: minibox_host_inc,
  c_args: minibox_host_c_args,
  gnu_symbol_visibility: 'default',
  install: true,
)

install_headers('minibox.h')

minibox_host_dep = declare_dependency(
  link_with: libminiboxhost.get_shared_lib(),
  include_directories: minibox_host_inc,
)
# A static variant for consumers that prefer to fold the host in (e.g. a core
# package that ships one fused native artifact).
minibox_host_static_dep = declare_dependency(
  link_with: libminiboxhost.get_static_lib(),
  include_directories: minibox_host_inc,
)
