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

This is an Autotools out-of-tree build: the directory from which `configure`
is invoked becomes the application build directory. Running `../configure`
inside `build-qt6-linux` generates Makefiles there; they read sources from the
repository and write object files, generated files and executables into the
build directory. The names `build-qt6-linux` and `build-qt6-windows` are arbitrary,
not hardcoded settings. `CONFIG_SITE` selects dependency and toolchain settings;
it does not select the build directory. Dependencies retain their own build
directories under `depends`.

Once configured, rebuild from the repository root with:

```sh
make -C build-qt6-linux -j8
make -C build-qt6-windows -j8
```

The `zcutil/build-qt.sh` and `zcutil/build-win.sh` scripts above still configure
and build in the source tree; they do not create these separate directories.

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

The Z-Send form includes a **View Notes** button. The dialog lists Sapling notes
using the wallet's `GetFilteredNotes` API, as used by `z_listunspent`, with minimum
confirmations zero and watch-only and locked notes included. Known spent notes
and conflicted transactions are excluded. Viewing notes does not unlock the
wallet or change transaction input selection.

The columns show amount, address label, address, notarization-adjusted
confirmations, status, change, transaction ID and Sapling output index.
Confirmation tooltips also show raw block confirmations. The selected note's
memo is displayed as plain UTF-8 text, or hex for binary data; the context menu
can copy the full original memo as hex. Change uses the same classification as
`z_listunspent`: the receiving address also spent notes in that transaction.
It is shown as unknown for watch-only entries, matching the RPC's omission.
Notes without a cached nullifier have an unknown spent status, so the listed
total must not be interpreted as an available balance.

For reference, `z_listunspent 0 9999999 true` returns unspent notes in both shielded
pools, while `z_listreceivedbyaddress` also includes previously spent notes.
The dialog is Sapling-only. Internal cryptographic fields (diversifier, note
randomness, commitment, nullifier and witness) are not exposed in the dialog.

The note-list tests use synthetic encrypted note outputs in an in-memory wallet:

```sh
make -C build-qt6-linux/src -j8 qt/test/viewnotes_tests
LD_LIBRARY_PATH="$PWD/depends/x86_64-pc-linux-gnu/lib" \
  build-qt6-linux/src/qt/test/viewnotes_tests
# Also check the dialog, sorting, filtering, memo display and refresh under Xvfb:
LD_LIBRARY_PATH="$PWD/depends/x86_64-pc-linux-gnu/lib" \
  xvfb-run -a build-qt6-linux/src/qt/test/viewnotes_tests --gui
```

The shutdown regression test checks RPC timer cancellation across threads and
GUI exit after core shutdown. In Qt 6, `quit()` can be vetoed by a window's
close handler; the shutdown-status window deliberately rejects close requests.
The application's shutdown-completion slot therefore exits the GUI event loop
directly after the core has stopped. RPC timer objects are cleaned up in their
owning Qt thread, even if their RPC handles are released after that thread exits.

```sh
make -C build-qt6-linux/src -j8 qt/test/shutdown_tests
LD_LIBRARY_PATH="$PWD/depends/x86_64-pc-linux-gnu/lib" \
  xvfb-run -a build-qt6-linux/src/qt/test/shutdown_tests
```

This test exercises the application's completion slot and shutdown window
without starting a node or opening wallet files. It also checks failed startup
with a visible splash screen, as happens when the data directory is already
locked: the splash must be released, the startup event loop must exit so normal
shutdown can run, and the process failure status must be preserved.
