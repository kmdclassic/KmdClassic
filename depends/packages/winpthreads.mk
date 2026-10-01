package=winpthreads
$(package)_version=8.0.0
$(package)_download_path=https://github.com/mingw-w64/mingw-w64/archive/refs/tags
$(package)_download_file=v$($(package)_version).tar.gz
$(package)_file_name=mingw-w64-$($(package)_version).tar.gz
$(package)_sha256_hash=93341e2505964cf235cb2eee706d18c6e197fb8e227fe68b3292018cefa93fd7
$(package)_build_subdir=mingw-w64-libraries/winpthreads
$(package)_patches=tls-destructor-order.patch

# Keep the runtime compatible with the supported MinGW-w64 8 / GCC 10
# toolchain, with the upstream fix for GCC's emulated TLS destruction order.
define $(package)_set_vars
$(package)_config_opts=--disable-shared --enable-static
endef

define $(package)_preprocess_cmds
  patch -p1 -i $($(package)_patch_dir)/tls-destructor-order.patch
endef

define $(package)_config_cmds
  $($(package)_autoconf)
endef

define $(package)_build_cmds
  $(MAKE)
endef

define $(package)_stage_cmds
  $(MAKE) DESTDIR=$($(package)_staging_dir) install
endef

define $(package)_postprocess_cmds
  rm lib/*.la
endef
