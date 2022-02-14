project('PION', 'cpp',
  default_options: [
    'warning_level=2',
    'cpp_std=c++11',
    'cpp_eh=none',
    'cpp_rtti=false'
  ])

cpp = meson.get_compiler('cpp')

subproject('NDNph', default_options: [
  'unittest=disabled',
  'programs=enabled',
])
NDNph_lib = dependency('NDNph')
NDNph = declare_dependency(
  dependencies: [NDNph_lib],
  compile_args: ['-include', 'NDNph-config.h', '-DNDNPH_WANT_CLI'],
)

mbedcrypto = cpp.find_library('mbedcrypto', has_headers: ['mbedtls/ecdh.h'])

subdir('src')
pion_lib = static_library('pion', pion_files, dependencies: [NDNph])

lib_dep = declare_dependency(
  include_directories: include_directories('src'),
  dependencies: [NDNph, mbedcrypto])

subdir('programs')
