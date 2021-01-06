project('ndn-onboarding', 'cpp',
  default_options: [
    'warning_level=2',
    'cpp_std=c++11',
    'cpp_eh=none',
    'cpp_rtti=false'
  ])

NDNph = declare_dependency(
  include_directories: '/usr/local/include',
  compile_args: ['-include', 'NDNph-config.h', '-DNDNPH_WANT_CLI']
)

mbedcrypto = meson.get_compiler('cpp').find_library('mbedcrypto')

subdir('src')
ndnob_lib = static_library('ndnob', ndnob_files, dependencies: [NDNph])

lib_dep = declare_dependency(
  include_directories: include_directories('src'),
  dependencies: [NDNph, mbedcrypto])

subdir('programs')
