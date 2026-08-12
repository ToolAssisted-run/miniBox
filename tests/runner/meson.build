# The guest runner, built on every target including cross builds. The guest .wbx
# it takes is OS-independent, so CI builds one on Linux and runs it through the
# Windows host (under wine, and natively on a Windows runner).
run_guest = executable('run_guest', 'run_guest.c',
  dependencies: minibox_host_dep,
  install: false,
)
