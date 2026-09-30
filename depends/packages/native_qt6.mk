package=native_qt6
$(package)_version=6.11.2
$(package)_download_path=https://download.qt.io/archive/qt/6.11/$($(package)_version)/submodules
$(package)_file_name=qtbase-everywhere-src-$($(package)_version).tar.xz
$(package)_sha256_hash=5b2e00eccaf5a4d8c14134ffa0ea8dfd0a35ae1ffc7f8d87fa4305a1ed23cf22
$(package)_tools_file=qttools-everywhere-src-$($(package)_version).tar.xz
$(package)_tools_hash=9ea75af35c512f7e09e61c8c3af3997f13b4d43bb099cf43fcec470126b4041e
$(package)_extra_sources=$($(package)_tools_file)

define $(package)_set_vars
$(package)_config_opts=-G "Unix Makefiles" -DCMAKE_BUILD_TYPE=Release -DBUILD_SHARED_LIBS=OFF
$(package)_config_opts+=-DQT_BUILD_TESTS=OFF -DQT_BUILD_EXAMPLES=OFF -DCMAKE_CXX_STANDARD=17
$(package)_config_opts+=-DFEATURE_gui=OFF -DFEATURE_widgets=OFF -DFEATURE_dbus=OFF
$(package)_config_opts+=-DFEATURE_icu=OFF -DFEATURE_openssl=OFF -DFEATURE_sql=OFF
$(package)_config_opts+=-DFEATURE_system_zlib=OFF -DFEATURE_system_pcre2=OFF -DFEATURE_zstd=OFF
$(package)_config_opts+=-DINSTALL_LIBEXECDIR=bin
ifeq ($(build_os),darwin)
$(package)_config_opts+=-DFEATURE_framework=OFF -DCMAKE_OSX_DEPLOYMENT_TARGET=$(OSX_MIN_VERSION)
endif
endef

define $(package)_fetch_cmds
$(call fetch_file,$(package),$($(package)_download_path),$($(package)_file_name),$($(package)_file_name),$($(package)_sha256_hash)) && \
$(call fetch_file,$(package),$($(package)_download_path),$($(package)_tools_file),$($(package)_tools_file),$($(package)_tools_hash))
endef

define $(package)_extract_cmds
  echo "$($(package)_sha256_hash)  $($(package)_source)" > hashes && \
  echo "$($(package)_tools_hash)  $($(package)_source_dir)/$($(package)_tools_file)" >> hashes && \
  $(build_SHA256SUM) -c hashes && \
  mkdir qtbase qttools && \
  tar --strip-components=1 -xf $($(package)_source) -C qtbase && \
  tar --strip-components=1 -xf $($(package)_source_dir)/$($(package)_tools_file) -C qttools
endef

define $(package)_config_cmds
  $($(package)_cmake) -S qtbase -B build-base $($(package)_config_opts)
endef

define $(package)_build_cmds
  $(MAKE) -C build-base && \
  DESTDIR=$($(package)_staging_dir) cmake --install build-base && \
  $($(package)_cmake) -S qttools -B build-tools -G "Unix Makefiles" \
    -DCMAKE_PREFIX_PATH=$($(package)_staging_prefix_dir) -DCMAKE_BUILD_TYPE=Release \
    -DQT_BUILD_TESTS=OFF -DQT_BUILD_EXAMPLES=OFF -DFEATURE_linguist=ON \
    -DFEATURE_assistant=OFF -DFEATURE_designer=OFF -DFEATURE_qdoc=OFF \
    -DFEATURE_clang=OFF -DFEATURE_qtattributionsscanner=OFF -DFEATURE_qtplugininfo=OFF && \
  $(MAKE) -C build-tools
endef

define $(package)_stage_cmds
  DESTDIR=$($(package)_staging_dir) cmake --install build-tools
endef
