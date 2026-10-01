# KomodoOcean (komodo-qt) #

![Downloads](https://img.shields.io/github/downloads/DeckerSU/KomodoOcean/total)

![](./doc/images/komodo-qt-promo-2020-01.jpg)

## Overview ##

KomodoOcean, also known as Komodo-QT, is the first native graphical wallet for the Komodo ecosystem, which includes the KMD coin and its assetchains (ACs). Built with the Qt framework, it offers an easy-to-use interface for managing Komodo assets. With KomodoOcean, users can send and receive KMD and interact with assetchains (ACs), view their transaction history, and access various features of the [Komodo Platform](https://komodoplatform.com/).

While the Komodo assetchains provide advanced privacy features, the main KMD coin does not include these privacy options. KomodoOcean stands out as a pioneering Qt-based wallet for a ZCash fork, especially since ZCash itself still does not have a native Qt wallet.

KomodoOcean is available on three OS platforms: `Windows`, `Linux`, and `macOS`.

This README describes the historical KomodoOcean project. For the maintained
KmdClassic build configurations and scripts, see [HOW-TO-BUILD.md](HOW-TO-BUILD.md).

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


