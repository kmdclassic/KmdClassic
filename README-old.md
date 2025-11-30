# KomodoOcean (komodo-qt) #

![Downloads](https://img.shields.io/github/downloads/DeckerSU/KomodoOcean/total)

![](./doc/images/komodo-qt-promo-2020-01.jpg)

## Overview ##

KomodoOcean, also known as Komodo-QT, is the first native graphical wallet for the Komodo ecosystem, which includes the KMD coin and its assetchains (ACs). Built with the Qt framework, it offers an easy-to-use interface for managing Komodo assets. With KomodoOcean, users can send and receive KMD and interact with assetchains (ACs), view their transaction history, and access various features of the [Komodo Platform](https://komodoplatform.com/).

While the Komodo assetchains provide advanced privacy features, the main KMD coin does not include these privacy options. KomodoOcean stands out as a pioneering Qt-based wallet for a ZCash fork, especially since ZCash itself still does not have a native Qt wallet.

KomodoOcean is available on three OS platforms: `Windows`, `Linux`, and `macOS`.

Use the default `static` branch and the following scripts in `./zcutil` to build, depending on your target platform and architecture:

- **Linux**:
  - `build.sh`: Native build for Linux.
  - `build-no-qt.sh`: Native build for Linux, but without Qt (produces a daemon-only version).
  - `build-aarch64-cross.sh`: Cross-compilation for ARM (aarch64) on Linux.

- **Windows**:
  - `build-win.sh`: Cross-compilation for Windows from a Linux environment.

- **MacOS**:
  - `build-mac.sh`: Native build for macOS (x86_64). Use on Intel-based Macs or run with `arch -x86_64 /bin/zsh` on Apple Silicon Macs.
  - `build-mac-cross.sh`: Cross-compilation for macOS (x86_64) from a Linux environment; produces `Mach-O 64-bit x86_64 executable` binaries.
  - `build-mac-arm.sh`: Native build for macOS aarch64. Use this on Apple Silicon Macs to produce `Mach-O 64-bit executable arm64` binaries.
  - `build-mac-arm-cross.sh`: Cross-compilation for macOS aarch64 from a Linux environment.

Or use the `static-experimental` branch to access the latest `nightly` features.

**Note**: Cross-compiled `arm64` Darwin (macOS) binaries do not include a digital signature by default. To use these binaries on macOS, they must be [signed](https://github.com/DeckerSU/KomodoOcean/wiki/F.A.Q.#q-the-zsh-killed-message-appears-on-macos-when-running-a-aarch64-apple-darwin-cross-compiled-binary) before execution. Failure to sign these binaries may result in issues with macOS security settings, preventing them from running properly.

Please note that the parent repository [ip-gpu/KomodoOcean](https://github.com/ip-gpu/KomodoOcean) is no longer maintained!

Visit `#🤝│general-support` or `#wallet-ocean-qt` channel in [Komodo Discord](https://komodoplatform.com/discord) for more information.

## Build Instructions ##

For detailed build instructions, see [HOW-TO-BUILD.md](HOW-TO-BUILD.md).

**komodo is experimental and a work-in-progress.** Use at your own risk.

## Docker ##

:whale: [deckersu/komodoocean](https://hub.docker.com/r/deckersu/komodoocean) - This Docker image provides the official KomodoOcean daemon for the Komodo blockchain platform. Komodod is the core component responsible for running a Komodo node, facilitating transaction validation, block creation, and communication within the network.

Read the description on [Docker Hub](https://hub.docker.com/r/deckersu/komodoocean) for usage examples.

## Getting started ##

### Download ZCash Parameters

Before running KomodoOcean, you need to download the ZCash cryptographic parameters. Run the following script:

```shell
./zcutil/fetch-params.sh
```

This script will download the required parameters files needed for the zero-knowledge proofs used in Komodo.

### Create komodo.conf

Before start the wallet you should [create config file](https://github.com/DeckerSU/KomodoOcean/wiki/F.A.Q.#q-after-i-start-komodo-qt-i-receive-the-following-error-error-cannot-parse-configuration-file-missing-komodoconf-only-use-keyvalue-syntax-what-should-i-do) `komodo.conf` at one of the following locations:

- Linux - `~/.komodo/komodo.conf`
- Windows - `%APPDATA%\Komodo\komodo.conf`
- MacOS - `~/Library/Application Support/Komodo/komodo.conf`

With the following content:

```
txindex=1
rpcuser=komodo
rpcpassword=local321 # don't forget to change password
rpcallowip=127.0.0.1
rpcbind=127.0.0.1
server=1
```

Bash one-liner for Linux to create `komodo.conf` with random RPC password:

```shell
mkdir -p ~/.komodo && \
RANDPASS=$(tr -dc 'a-zA-Z0-9' < /dev/urandom | head -c 16) && \
cat > ~/.komodo/komodo.conf << EOF
txindex=1
rpcuser=komodo
rpcpassword=${RANDPASS}
rpcallowip=127.0.0.1
rpcbind=127.0.0.1
server=1
EOF
```

## Developers of Qt wallet ##

- Original creator: [Ocean](https://github.com/ip-gpu) (created the first version and maintained it initially)
- Main developer: [Decker](https://github.com/DeckerSU) (maintains and develops the project to this day)
- ☕🍪 [Buy DeckerSU a tea and cookies!](https://github.com/sponsors/DeckerSU)

Special thanks to [jl777](https://github.com/jl777) and [ca333](https://github.com/ca333) for being a constant source of learning and inspiration.


