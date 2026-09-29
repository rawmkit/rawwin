project(
  'rawwin',
  'c',
  version: '0.1.0',
  meson_version: '>=1.6.0',
  default_options: ['c_std=c99', 'warning_level=3'],
)

add_project_arguments('-D_POSIX_C_SOURCE=200809L', language: 'c')
add_project_arguments('-DVERSION="' + meson.project_version() + '"', language: 'c')

subdir('src')
subdir('docs')
subdir('tests')
