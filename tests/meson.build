# miniBox tests wired as `meson test`.
#
# Two tiers:
#  - host unit tests: link the host statically, no guest needed (always run).
#  - system tests: run a gcc-built guest .wbx through the host. The guest
#    toolchain (musl + emulibc) is built by meson itself, so these run whenever
#    the guest toolchain is built - i.e. native builds, not cross builds.

# ---- host unit tests ----
subdir('unit')

# ---- guest system tests (musl+emulibc are meson-built; native only) ----
if build_guest_toolchain
  subdir('conformance')
  subdir('threads')
  # C++ guests need the (optional) C++ guest toolchain
  if waterbox_cpp_guest
    subdir('cpp')
  endif
endif
