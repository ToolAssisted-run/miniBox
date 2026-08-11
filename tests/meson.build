# miniBox tests wired as `meson test`.
#
# Two tiers:
#  - host unit tests: link the host statically, no guest needed (always run).
#  - system tests: require a gcc-built guest .wbx, which needs the guest
#    toolchain (musl+emulibc). Built via custom_target when the toolchain has
#    been bootstrapped; skipped with a notice otherwise.

# ---- host unit tests ----
subdir('unit')

# ---- conformance system test (needs the guest toolchain) ----
fs = import('fs')
musl_gcc = meson.project_source_root() / 'toolchain' / 'sysroot' / 'bin' / 'musl-gcc'
emulibc_obj = meson.project_source_root() / 'toolchain' / 'emulibc' / 'obj' / 'release' / 'emulibc.c.o'

if fs.exists(musl_gcc) and fs.exists(emulibc_obj)
  subdir('conformance')
else
  message('Guest toolchain not built (toolchain/sysroot/bin/musl-gcc missing); ' +
          'skipping guest system tests. Run: meson compile -C <build> toolchain')
endif
