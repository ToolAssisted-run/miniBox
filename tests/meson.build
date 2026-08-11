# miniBox tests wired as `meson test`.
#
# Two tiers:
#  - host unit tests: link the host statically, no guest needed (always run).
#  - system tests: require a gcc-built guest .wbx, which needs the guest
#    toolchain (musl+emulibc). Built via custom_target when the toolchain has
#    been bootstrapped; skipped with a notice otherwise.

# ---- host unit tests ----
subdir('unit')

# ---- guest system tests (need the bootstrapped musl; emulibc.o is meson-built) ----
fs = import('fs')
musl_gcc = meson.project_source_root() / 'sysroot' / 'bin' / 'musl-gcc'

if fs.exists(musl_gcc)
  subdir('conformance')
  subdir('threads')
else
  message('Guest toolchain not bootstrapped (sysroot/bin/musl-gcc ' +
          'missing); skipping guest system tests. Run source/guest/build-toolchain.sh')
endif
