#!/usr/bin/env bash

make -C ${PWD}/depends v=1 NO_PROTON=1 HOST=x86_64-apple-darwin -j$(sysctl -n hw.ncpu)
./autogen.sh
# -Wno-deprecated-builtins -Wno-enum-constexpr-conversion
CXXFLAGS="-g0 -O2 -Wno-unknown-warning-option" \
CONFIG_SITE="$PWD/depends/x86_64-apple-darwin/share/config.site" ./configure --disable-tests --disable-bench --with-gui=qt5 --disable-bip70
make -j$(sysctl -n hw.ncpu) # V=1
