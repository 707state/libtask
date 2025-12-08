project('libtask',['c'],version: '0.1',meson_version:'>= 1.3.0',default_options:['warning_level=3'])
dependencies=[
    dependency('liburing')
]
sources=[
  'fd.c',
  'channel.c',
  'net.c',
  'qlock.c',
  'rendez.c',
  'task.c',
  'taskimpl.h'
]
inc_dir=include_directories('.')
executable(
  'primes',
  sources: sources+'primes.c',
  include_directories: inc_dir,
)
executable(
  'tcpproxy',
  sources: sources+'tcpproxy.c',
  include_directories: inc_dir
)
