package=qt6_translations
$(package)_version=6.11.2
$(package)_download_path=https://download.qt.io/archive/qt/6.11/$($(package)_version)/submodules
$(package)_file_name=qttranslations-everywhere-src-$($(package)_version).tar.xz
$(package)_sha256_hash=021684c1a7937a9fabc3b056a6698ad5978794caf9ac190fd6cc11399e67c014
$(package)_dependencies=native_qt6

# lrelease runs on the build machine, including for MinGW targets.
define $(package)_build_cmds
  $(build_prefix)/bin/lrelease translations/*.ts
endef

define $(package)_stage_cmds
  mkdir -p $($(package)_staging_prefix_dir)/translations && \
  cp translations/*.qm $($(package)_staging_prefix_dir)/translations/
endef
