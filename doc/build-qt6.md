# Qt 6 builds (Linux and MinGW-w64)

Linux and Windows builds use Qt 6.11.2 from `depends`. Qt 6 requires C++17;
GCC remains the compiler. The application is compiled with `-std=c++17`.
The Qt 5.15.11 recipe remains available for the existing macOS/Android recipes.

On Ubuntu/Debian, install the prerequisites listed in HOW-TO-BUILD.md and:

```sh
sudo apt-get install cmake python3 bison
# For Windows cross-compilation:
sudo apt-get install mingw-w64
```

CMake 3.22 or newer is required. Qt is built statically. The Linux X11 libraries
continue to use the existing shared-library policy in depends. Meson, Ninja,
libxkbcommon and xcb-util-cursor are supplied by depends; do not install a system
Qt SDK to build this configuration.

From the repository root:

```sh
./zcutil/build-qt.sh -j8
./zcutil/build-win.sh -j8
```

These scripts configure the source tree. Clean it with `make distclean` before
switching target platforms. For separate application build directories instead:

```sh
make -C depends -j8
./autogen.sh
mkdir -p build-qt6-linux
cd build-qt6-linux
CONFIG_SITE="$PWD/../depends/x86_64-pc-linux-gnu/share/config.site" \
  ../configure --with-gui=qt6 --disable-bip70 --disable-tests --disable-bench
make -j8
cd ..

make -C depends HOST=x86_64-w64-mingw32 -j8
mkdir -p build-qt6-windows
cd build-qt6-windows
CONFIG_SITE="$PWD/../depends/x86_64-w64-mingw32/share/config.site" \
  ../configure --with-gui=qt6 --disable-bip70 --disable-tests --disable-bench \
  CXXFLAGS='-O2 -g0 -DCURL_STATICLIB'
make -j8
```

Run `make distclean` first if the source directory was previously configured.
The executables are under `src/` (the wallet is under `src/qt/`) in the chosen
build directory.

`native_qt6` builds Linux tools, including `moc`, `uic`, `rcc` and Linguist.
`qt6` builds the target libraries and plugins. This split allows cross-builds
to execute native tools instead of Windows executables. `qt6_translations`
compiles and installs the Qt translations.

Autoconf delegates Qt 6 discovery and a small link check to
`build-aux/qt6/configure.py`. It reads CMake's File API to obtain the complete
static link dependencies, including the platform plugin, and imports them into
the existing Automake build. Diagnostics go to `config.log`; generated probe
files go to `qt6-config/` in the application build directory.

Release source: https://download.qt.io/archive/qt/6.11/6.11.2/submodules/
All source archives are pinned by SHA256 in their depends recipes.

The target Qt build disables unused Wayland, OpenGL/Vulkan, SQL, printing,
session management, GIF/ICO/JPEG, VNC and TUIO support. Widgets, PNG resources, XCB (Linux), the
Windows platform plugin and Linux D-Bus support remain enabled. On MinGW,
`stack_clash_protection` is disabled to avoid an internal compiler error in
GCC 10's SEH unwind emitter; Qt's stack protector remains enabled.

The patches under `depends/patches/qt6/` allow the Windows API headers from
MinGW-w64 8.0 (as packaged with GCC 10 on Ubuntu 22.04) to
skip unavailable thread QoS and D3D12 debug-layer APIs and provide missing
Winsock option constants. The MinGW recipe also disables Qt DNS lookup,
HTTP, UDP and TLS features; the application uses local Qt sockets and its
own networking stack, with BIP70 disabled as in the previous build scripts.

Windows UI Automation (screen-reader integration), Direct2D, DirectWrite and Qt network
connectivity monitoring are disabled in this MinGW configuration because the
MinGW-w64 8.0 headers lack the required interfaces. Linux accessibility remains
enabled. Windows fonts use the GDI backend. The Qt 6 Windows target requires
Windows 10 version 1809 or newer.

Validated on Ubuntu 22.04 with GCC 11.4.0 for Linux and
`x86_64-w64-mingw32-g++-posix` 10 with MinGW-w64 8.0 headers for Windows.
Both complete depends builds and application builds passed using the commands
above, with BIP70, tests and benchmarks disabled. The Linux wallet passed
`-version` and `-help` startup checks under Xvfb. The Windows wallet is an
x86-64 GUI PE executable and imports only Windows system DLLs; Qt, libstdc++,
libgcc, libssp and winpthreads are linked statically. Windows execution has
not been tested in this Linux environment.

The address-model regression test can also be built with the core tests disabled:

```sh
make -C build-qt6-linux/src -j8 qt/test/modelindex_tests
LD_LIBRARY_PATH="$PWD/depends/x86_64-pc-linux-gnu/lib" \
  build-qt6-linux/src/qt/test/modelindex_tests
```

It uses an in-memory wallet and checks retained indexes and proxy selections
while inserting, removing and updating transparent and shielded address rows.
It also exercises Sapling address creation with unencrypted and encrypted wallets,
cancelled unlock requests, restoration of the lock state on success and failure,
and error reporting when the HD seed cannot be read. It does not access wallet
files or require a display server; Qt settings are isolated in a temporary directory.
