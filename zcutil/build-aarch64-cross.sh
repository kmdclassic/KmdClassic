#!/usr/bin/env bash

make "$@" -C ${PWD}/depends v=1 NO_PROTON=1 HOST=aarch64-linux-gnu
./autogen.sh
CXXFLAGS="-g0 -O2 -Wno-unknown-warning-option" \
CONFIG_SITE="$PWD/depends/aarch64-linux-gnu/share/config.site" ./configure --disable-tests --disable-bench --with-gui=qt5 --disable-bip70
make "$@" # V=1
