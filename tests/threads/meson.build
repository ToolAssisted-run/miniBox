# Green-thread system test: a pthreaded guest (mutex + condvar + join) driven
# through the host. Requires the guest toolchain (guarded in ../meson.build).

guest_threads_wbx = custom_target('guest_threads.wbx',
  input: 'guest_threads.c',
  output: 'guest_threads.wbx',
  command: [
    waterbox_musl_gcc, waterbox_guest_cflags, waterbox_guest_ldflags,
    '-std=c11', '-o', '@OUTPUT@', '@INPUT@', waterbox_emulibc_obj, '-lgcc',
  ],
)

run_threads = executable('run_threads', 'run_threads.c',
  dependencies: minibox_host_dep,
)
test('threads', run_threads,
  args: [guest_threads_wbx],
  suite: 'system',
  timeout: 120,
)
