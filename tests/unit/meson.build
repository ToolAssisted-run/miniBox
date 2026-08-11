# Host unit tests: link the host statically (they reach internal APIs via
# minibox_internal.h) and run without any guest.

unit_tests = ['test_memblock', 'test_sha256', 'test_fs', 'test_savestate']

foreach t : unit_tests
  exe = executable(t, t + '.c', dependencies: minibox_host_static_dep)
  test(t, exe, suite: 'unit', timeout: 60)
endforeach
