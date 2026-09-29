package=native_meson
$(package)_version=1.5.2
$(package)_download_path=https://github.com/mesonbuild/meson/releases/download/$($(package)_version)
$(package)_file_name=meson-$($(package)_version).tar.gz
$(package)_sha256_hash=f955e09ab0d71ef180ae85df65991d58ed8430323de7d77a37e11c9ea630910b
$(package)_dependencies=native_ninja

# Meson is pure Python; keep its modules beside the entry point.
define $(package)_stage_cmds
  mkdir -p $($(package)_staging_prefix_dir)/lib/meson $($(package)_staging_prefix_dir)/bin && \
  cp -r meson.py mesonbuild $($(package)_staging_prefix_dir)/lib/meson/ && \
  ln -s ../lib/meson/meson.py $($(package)_staging_prefix_dir)/bin/meson
endef
