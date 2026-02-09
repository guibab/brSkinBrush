project('brSkinBrush', 'cpp')

maya_dep = dependency('maya')
maya_name_suffix = maya_dep.get_variable('name_suffix')
maya_version = maya_dep.get_variable('maya_version')

source_files = [
  'src/skinBrushContext.cpp',
  'src/skinBrushTool.cpp',
  'src/functions.cpp',
  'src/pluginMain.cpp',
  'src/skinBrushCmd.cpp',
  'src/skinBrushContextSetFlags.cpp',
  'src/skinBrushLegacy.cpp',
]


gl_dep = dependency('gl', method : 'system')
if build_machine.system() == 'windows'
  cmplr = meson.get_compiler('cpp')
  glu_dep = cmplr.find_library('glu32', required : true)
else
  glu_dep = dependency('glu')
endif


outlib = shared_library(
  meson.project_name(),
  source_files,
  install: true,
  install_dir : meson.global_source_root() / 'output_Maya' + maya_version,
  include_directories : include_directories(['src']),
  dependencies : [maya_dep, gl_dep, glu_dep],
  name_prefix : '',
  name_suffix : maya_name_suffix,
)
