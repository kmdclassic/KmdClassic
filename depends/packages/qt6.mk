package=qt6
$(package)_version=6.11.2
$(package)_download_path=https://download.qt.io/archive/qt/6.11/$($(package)_version)/submodules
$(package)_file_name=qtbase-everywhere-src-$($(package)_version).tar.xz
$(package)_sha256_hash=5b2e00eccaf5a4d8c14134ffa0ea8dfd0a35ae1ffc7f8d87fa4305a1ed23cf22
$(package)_dependencies=native_qt6
$(package)_patches=mingw-thread-qos.patch mingw-sdk-compat.patch
$(package)_linux_dependencies=freetype fontconfig libxcb libxkbcommon libxcb_util libxcb_util_render libxcb_util_keysyms libxcb_util_image libxcb_util_wm libxcb_util_cursor

define $(package)_set_vars
$(package)_cxxflags := $(filter-out -std=c++11,$($(package)_cxxflags))
$(package)_config_opts=-G "Unix Makefiles" -DBUILD_SHARED_LIBS=OFF
$(package)_config_opts_release=-DCMAKE_BUILD_TYPE=Release
$(package)_config_opts_debug=-DCMAKE_BUILD_TYPE=Debug
$(package)_config_opts+=-DCMAKE_CXX_STANDARD=17 -DQT_BUILD_TESTS=OFF -DQT_BUILD_EXAMPLES=OFF
$(package)_config_opts+=-DQT_HOST_PATH=$(build_prefix) -DQT_FORCE_FIND_TOOLS=ON
$(package)_config_opts+=-DCMAKE_PREFIX_PATH=$(host_prefix) -DINSTALL_LIBEXECDIR=libexec
# Qt's summary also checks INPUT_opengl, even when all OpenGL features are off.
$(package)_config_opts+=-DINPUT_opengl=no -DFEATURE_opengl=OFF -DFEATURE_dynamicgl=OFF -DFEATURE_vulkan=OFF -DFEATURE_icu=OFF
$(package)_config_opts+=-DFEATURE_openssl=OFF -DFEATURE_sql=OFF -DFEATURE_testlib=ON
$(package)_config_opts+=-DFEATURE_system_zlib=OFF -DFEATURE_system_pcre2=OFF
$(package)_config_opts+=-DFEATURE_system_png=OFF -DFEATURE_system_harfbuzz=OFF
# The wallet uses PNG resources and no printing, 3D or Wayland APIs.
$(package)_config_opts+=-DFEATURE_printsupport=OFF -DFEATURE_wayland=OFF -DFEATURE_waylandscanner=OFF
$(package)_config_opts+=-DFEATURE_gif=OFF -DFEATURE_ico=OFF -DFEATURE_jpeg=OFF
$(package)_config_opts+=-DFEATURE_sessionmanager=OFF
$(package)_config_opts+=-DFEATURE_vnc=OFF -DFEATURE_tuiotouch=OFF
$(package)_config_opts+=-DFEATURE_zstd=OFF -DFEATURE_glib=OFF -DFEATURE_cups=OFF
$(package)_config_opts_linux=-DQT_FORCE_FIND_TOOLS=OFF -DFEATURE_xcb=ON -DFEATURE_xcb_xlib=OFF -DFEATURE_xlib=OFF
$(package)_config_opts_linux+=-DFEATURE_fontconfig=ON -DFEATURE_system_freetype=ON
$(package)_config_opts_linux+=-DFEATURE_dbus=ON -DFEATURE_dbus_linked=OFF
$(package)_config_opts_linux+=-DFEATURE_eglfs=OFF -DFEATURE_linuxfb=OFF -DFEATURE_evdev=OFF
$(package)_config_opts_linux+=-DFEATURE_libinput=OFF -DFEATURE_libudev=OFF -DFEATURE_gtk3=OFF
$(package)_config_opts_darwin=-DQT_FORCE_FIND_TOOLS=OFF -DFEATURE_framework=OFF -DFEATURE_dbus=OFF
$(package)_config_opts_darwin+=-DCMAKE_OSX_DEPLOYMENT_TARGET=$(OSX_MIN_VERSION)
$(package)_config_opts_aarch64_darwin=-DCMAKE_OSX_ARCHITECTURES=arm64
# GCC 10 MinGW ICEs in SEH unwind emission with -fstack-clash-protection.
# Windows already uses stack probing; retain Qt's stack protector and CET.
$(package)_config_opts_mingw32=-DFEATURE_stack_clash_protection=OFF -DFEATURE_dbus=OFF -DFEATURE_freetype=OFF
# The application uses local sockets and its own networking/TLS stack.
$(package)_config_opts_mingw32+=-DFEATURE_dnslookup=OFF -DFEATURE_schannel=OFF -DFEATURE_ssl=OFF
$(package)_config_opts_mingw32+=-DFEATURE_http=OFF -DFEATURE_udpsocket=OFF -DFEATURE_networklistmanager=OFF
# MinGW-w64 8 lacks the UI Automation provider interfaces. Widgets do not
# require this optional screen-reader bridge or the Direct2D platform plugin.
$(package)_config_opts_mingw32+=-DFEATURE_accessibility=OFF -DFEATURE_direct2d=OFF -DFEATURE_directwrite=OFF
$(package)_config_opts_mingw32+=-DCMAKE_SYSTEM_NAME=Windows -DCMAKE_RC_COMPILER=$(host)-windres
$(package)_config_opts_mingw32+=-DCMAKE_FIND_ROOT_PATH=$(host_prefix)
$(package)_config_opts_mingw32+=-DCMAKE_FIND_ROOT_PATH_MODE_LIBRARY=ONLY -DCMAKE_FIND_ROOT_PATH_MODE_INCLUDE=ONLY
$(package)_config_opts_mingw32+=-DCMAKE_FIND_ROOT_PATH_MODE_PACKAGE=ONLY -DCMAKE_FIND_ROOT_PATH_MODE_PROGRAM=NEVER
endef

define $(package)_preprocess_cmds
  patch -p1 -i $($(package)_patch_dir)/mingw-thread-qos.patch && \
  patch -p1 -i $($(package)_patch_dir)/mingw-sdk-compat.patch
endef

define $(package)_config_cmds
  $($(package)_cmake) -S . -B build $($(package)_config_opts)
endef

define $(package)_build_cmds
  $(MAKE) -C build
endef

define $(package)_stage_cmds
  DESTDIR=$($(package)_staging_dir) cmake --install build
endef
