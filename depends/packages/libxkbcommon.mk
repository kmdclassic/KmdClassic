package=libxkbcommon
$(package)_version=0.10.0
$(package)_download_path=https://xkbcommon.org/download/
$(package)_file_name=$(package)-$($(package)_version).tar.xz
$(package)_sha256_hash=57c3630cdc38fb4734cd57fa349e92244f5ae3862813e533cedbd86721a0b6f2
$(package)_dependencies=libxcb native_meson

define $(package)_config_cmds
  CC="$($(package)_cc)" CFLAGS="$($(package)_cflags) $($(package)_cppflags)" \
  LDFLAGS="$($(package)_ldflags)" $(build_prefix)/bin/meson setup build \
    --prefix=$(host_prefix) --libdir=lib --buildtype=release --default-library=shared \
    -Denable-docs=false -Denable-wayland=false -Denable-x11=true
endef

define $(package)_build_cmds
  $(build_prefix)/bin/ninja -C build
endef

define $(package)_stage_cmds
  DESTDIR=$($(package)_staging_dir) $(build_prefix)/bin/ninja -C build install
endef
