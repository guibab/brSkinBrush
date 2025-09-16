project('brSkinBrush', 'cpp')

maya_dep = dependency('maya')
maya_name_suffix = maya_dep.get_variable('name_suffix')
maya_version = maya_dep.get_variable('maya_version')

source_files = [
  'src/blurRelaxNode.cpp',
  'src/fastMayaRelax.cpp',
  'src/fastRelax.cpp',
  'src/pluginRegister.cpp',
]

outlib = shared_library(
  meson.project_name(),
  source_files,
  install: true,
  install_dir : meson.global_source_root() / 'output_Maya' + maya_version,
  include_directories : include_directories(['src']),
  dependencies : maya_dep,
  name_prefix : '',
  name_suffix : maya_name_suffix,
)
