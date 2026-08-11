# Host unit tests: link the host statically (they reach internal APIs via
# minibox_internal.h) and run without any guest.

test_memblock = executable('test_memblock', 'test_memblock.c',
  dependencies: minibox_host_static_dep,
)
test('memblock', test_memblock,
  suite: 'unit',
  timeout: 60,
)
