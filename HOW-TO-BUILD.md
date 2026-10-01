# Building KmdClassic

The maintained build configurations are:

| Target | Build machine and toolchain | Script | Output |
| --- | --- | --- | --- |
| Linux x86_64, Qt 6 GUI | Ubuntu 22.04 x86_64, GCC 11.4 | `zcutil/build-qt.sh` | Wallet, daemon and command-line utilities |
| Windows x86_64, Qt 6 GUI | Ubuntu 22.04 x86_64, MinGW-w64 8.0 / GCC 10 POSIX | `zcutil/build-win.sh` | Wallet, daemon and command-line utilities (`.exe`) |
| macOS arm64, Qt 6 GUI | Native Apple Silicon, Xcode / Apple Clang | `zcutil/build-mac-arm.sh` | Wallet, daemon, command-line utilities and `.app` |
| Linux x86_64, no GUI | Native Linux, GCC | `zcutil/build.sh` | Daemon and command-line utilities; Qt is not built |

All maintained GUI configurations use Qt **6.11.2** from `depends` and C++17.
Linux and Windows use GCC; macOS uses Apple Clang. A system Qt installation
is not required. The dependency recipes pin source versions and checksums.

Linux and Windows builds have been validated on Ubuntu 22.04. The Linux GUI
passed startup and regression tests. On Windows 11, the wallet passed isolated
regtest startup and process-exit checks using RPC stop and window close, and
the TLS cleanup regression test passed. The Qt 6 Windows target requires
Windows 10 version 1809 or newer; Windows 10 runtime testing remains outstanding.

The native macOS build and GUI regression tests passed on an M2 with macOS
26.4.1, Apple Clang 21.0.0 and SDK 26.4. The deployment target is macOS 13.0;
running on macOS 13 has not been tested. Building requires SDK 14 or newer.

Intel macOS, Linux-to-macOS cross-compilation, Linux ARM cross-compilation,
Android, Qt 5 GUI builds and Debian package generation are outside the
maintained configurations. Their old entry-point scripts have been removed.
The remaining legacy dependency recipes do not imply support for those targets.

## Source checkout

Use the revision you intend to build from the
[KmdClassic repository](https://github.com/kmdclassic/KmdClassic). The commands
below assume a checkout containing the Qt 6 integration and are run from its
root directory. Do not mix build products from different target platforms.

## Linux prerequisites

For the validated Ubuntu 22.04 build environment:

```sh
sudo apt-get update
sudo apt-get install build-essential pkg-config libc6-dev m4 g++-multilib \
  autoconf automake libtool ncurses-dev unzip git python3 bison zlib1g-dev \
  wget curl ca-certificates libcurl4-gnutls-dev bsdmainutils cmake
```

CMake 3.22 or newer is required for Qt 6. Meson, Ninja, Qt and the required
third-party libraries are built through `depends`. Rust is also supplied by
`depends`; a separate rustup installation is not needed for these builds.

The first build downloads and compiles dependencies. Choose the job count
according to available RAM as well as CPU count; the examples use two jobs.

## Linux Qt 6 wallet

```sh
./zcutil/build-qt.sh -j2
```

Outputs include:

- `src/qt/kmdclassic-qt`
- `src/kmdclassicd`
- `src/kmdclassic-cli`
- `src/kmdclassic-tx`
- `src/wallet-utility`

## Linux daemon without GUI

Use the same Linux prerequisites and:

```sh
./zcutil/build.sh -j2
```

This selects `NO_QT=1` for dependencies and `--with-gui=no` for the
application. It builds `kmdclassicd` and the command-line utilities under
`src/`, without the Qt wallet.

## Windows via MinGW-w64

Install the Linux prerequisites above, then the cross-toolchain:

```sh
sudo apt-get install mingw-w64
sudo update-alternatives --set x86_64-w64-mingw32-gcc /usr/bin/x86_64-w64-mingw32-gcc-posix
sudo update-alternatives --set x86_64-w64-mingw32-g++ /usr/bin/x86_64-w64-mingw32-g++-posix
./zcutil/build-win.sh -j2
```

The POSIX thread variant is required. Outputs include
`src/qt/kmdclassic-qt.exe`, `src/kmdclassicd.exe`,
`src/kmdclassic-cli.exe`, `src/kmdclassic-tx.exe` and
`src/wallet-utility.exe`. Qt and the MinGW runtime are linked statically.
`depends` supplies a patched winpthreads runtime so that Qt thread-local
cleanup does not hang process exit with the supported GCC 10 toolchain.
See [doc/build-qt6.md](doc/build-qt6.md) for the upstream fix and regression test.

## Native macOS on Apple Silicon

Use a native arm64 shell, with Xcode installed and selected by
`xcode-select`. Install the build tools through Homebrew:

```sh
brew install autoconf automake libtool pkg-config coreutils cmake make
./zcutil/build-mac-arm.sh
```

The script adds the standard Apple Silicon Homebrew paths, so it also works
over SSH. By default it limits parallel jobs to the CPU count and one job per
3 GiB of physical RAM. To set the job count explicitly:

```sh
JOBS=2 ./zcutil/build-mac-arm.sh
```

The output directory is `build-qt6-mac-arm`, including:

- `build-qt6-mac-arm/src/qt/kmdclassic-qt`
- `build-qt6-mac-arm/src/kmdclassicd` and the command-line utilities
- `build-qt6-mac-arm/KmdClassic-Qt.app`

The bundle has an ad-hoc signature for local use. Developer ID signing and
notarization are not performed by this script.

## Build directories and switching targets

`build.sh`, `build-qt.sh` and `build-win.sh` configure and build in the source
tree. Use separate checkouts for different configurations, or run
`make distclean` in a previously configured tree before switching targets.

For simultaneous Linux and Windows builds from one source checkout, follow
the [separate build-directory instructions](doc/build-qt6.md). The directory
from which `configure` runs determines where application objects and binaries
are written. `CONFIG_SITE` selects dependencies and the toolchain, not the
output directory.

The macOS script creates its separate application build directory
automatically; `BUILD_DIR` can override it. Dependencies for all configurations
retain their own work directories and target prefixes under `depends`.

See [doc/build-qt6.md](doc/build-qt6.md) for Qt configuration details, the
platform-specific patches, script overrides and GUI regression-test commands.

## CI and Docker build environment

The `build-project.yml` workflow builds Linux x86_64 and Windows x86_64 Qt 6
wallets on a self-hosted Linux x86_64 runner with Docker. It runs for pull
requests from this repository and on manual dispatch. Fork pull requests are
excluded from the self-hosted runner. The manual `jobs` input controls parallel
compilation (default: two jobs).

The shared Docker image uses Ubuntu 22.04, CMake and the POSIX MinGW toolchain.
Application builds use `build-qt6-ci-linux` and `build-qt6-ci-windows`;
artifacts are collected in `releases/linux` and `releases/windows`. Windows
artifacts also include `fetch-params.ps1`. A failed build or a missing binary
fails the job. The workflow does not cross-compile
macOS; use the native Apple Silicon script on a Mac.

To run the same container build locally as a non-root user with Docker access:

```sh
BUILD_JOBS=2 ./build_releases.sh
```

Use an unconfigured source tree, as with the separate build directories above.
The container runs the build with the caller's UID/GID so generated files
remain writable outside Docker. The local wrapper preserves the source path
inside Docker because cached dependency metadata can contain absolute paths.
Reuse a dependency cache only with the same toolchain and workspace path.
The main `Dockerfile` builds the daemon using `zcutil/build.sh`; it is separate
from this GUI build environment.

## Runtime parameters

The Zcash proving parameters are needed to run the wallet/node, not to compile
it. On the machine where the application will run, download them with:

```sh
./zcutil/fetch-params.sh
```

On Windows 10/11, open PowerShell in the repository root and run:

```powershell
powershell.exe -NoProfile -ExecutionPolicy Bypass -File .\zcutil\fetch-params.ps1
```

This uses the built-in Windows PowerShell 5.1 commands and needs no `wget`,
`curl`, package installation or administrator privileges. `ExecutionPolicy`
applies only to this invocation; it does not change the machine's policy.
Files are stored in `%APPDATA%\ZcashParams`, matching the wallet's default
parameter directory:

- `sapling-spend.params`
- `sapling-output.params`
- `sprout-groth16.params`

The script verifies SHA-256 for existing and downloaded files, skips valid
files, and retries failed downloads up to three times. A replacement is
installed only after verification; incomplete downloads are not used by the
wallet. One download process at a time can use the destination directory.
Optional `-ParamsDir` and `-Attempts` arguments override the destination and
retry count. A custom destination is useful for preparing files for another
machine; copy them into that Windows user's `%APPDATA%\ZcashParams` before
starting the wallet.

The older `.bat` downloader remains available for existing setups.

The PowerShell downloader has been checked with Windows PowerShell 5.1 on
Windows 11, including a real parameter download, existing-file verification,
checksum rejection, retries, locking and cleanup after download failures.
