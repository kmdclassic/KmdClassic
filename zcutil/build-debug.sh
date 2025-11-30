#!/usr/bin/env bash

set -eu -o pipefail

function cmd_pref() {
    if type -p "$2" > /dev/null; then
        eval "$1=$2"
    else
        eval "$1=$3"
    fi
}

function gprefix() {
    cmd_pref "$1" "g$2" "$2"
}

gprefix READLINK readlink
cd "$(dirname "$("$READLINK" -f "$0")")/.."

: "${MAKE:=make}"
: "${BUILD:=$(./depends/config.guess)}"
: "${HOST:=$BUILD}"
: "${CONFIGURE_FLAGS:=}"

if [ "x$*" = 'x--help' ]
then
    cat <<EOF
Usage:
$0 --help
  Show this help message and exit.
$0 [ --enable-lcov || --disable-tests ] [ --disable-mining ] [ --enable-proton ] [ --disable-libs ] [ MAKEARGS... ]
  Build KmdClassic and most of its transitive dependencies from
  source. MAKEARGS are applied to both dependencies and KmdClassic itself.
  If --enable-lcov is passed, KmdClassic is configured to add coverage
  instrumentation, thus enabling "make cov" to work.
  If --disable-tests is passed instead, the KmdClassic tests are not built.
  If --disable-mining is passed, KmdClassic is configured to not build any mining
  code. It must be passed after the test arguments, if present.
  If --enable-proton is passed, KmdClassic is configured to build the Apache Qpid Proton
  library required for AMQP support. This library is not built by default.
  It must be passed after the test/mining arguments, if present.
EOF
    exit 0
fi

set -x

LCOV_ARG=''
HARDENING_ARG='--enable-hardening'
TEST_ARG=''
if [ "x${1:-}" = 'x--enable-lcov' ]; then
    LCOV_ARG='--enable-lcov'
    HARDENING_ARG='--disable-hardening'
    shift
elif [ "x${1:-}" = 'x--disable-tests' ]; then
    TEST_ARG='--enable-tests=no'
    shift
fi

MINING_ARG=''
if [ "x${1:-}" = 'x--disable-mining' ]; then
    MINING_ARG='--enable-mining=no'
    shift
fi

PROTON_ARG='--enable-proton=no'
if [ "x${1:-}" = 'x--enable-proton' ]; then
    PROTON_ARG=''
    shift
fi

PREFIX="$(pwd)/depends/$BUILD/"

HOST="$HOST" BUILD="$BUILD" "$MAKE" "$@" -C ./depends/ V=1
./autogen.sh
CXXFLAGS='-g -O0' ./configure --prefix="${PREFIX}" --with-gui=qt5 --disable-bip70 --enable-tests=yes --enable-wallet=yes "$HARDENING_ARG" "$LCOV_ARG" "$TEST_ARG" "$MINING_ARG" "$PROTON_ARG" ${CONFIGURE_FLAGS}
"$MAKE" "$@" V=1
