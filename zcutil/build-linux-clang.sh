#!/usr/bin/env bash

# Using this build setup, the dependencies will be built with gcc/g++, but the application(s)
# itself (komodod and others) will be built with clang/clang++. This behavior can be changed, of course,
# but it’s something to keep in mind, especially when considering build consistency or debugging
# toolchain-related issues.

set -eu -o pipefail

# Function to assign a variable based on the existence of a command
function cmd_pref() {
    if type -p "$2" > /dev/null; then
        eval "$1=$2"
    else
        eval "$1=$3"
    fi
}

# If a g-prefixed version of the command exists, use it preferentially.
function gprefix() {
    cmd_pref "$1" "g$2" "$2"
}

gprefix READLINK readlink
cd "$(dirname "$("$READLINK" -f "$0")")/.."

# Set MAKE variable if not already set
if [[ -z "${MAKE-}" ]]; then
    MAKE=make
fi

# Set BUILD variable if not already set
if [[ -z "${BUILD-}" ]]; then
    BUILD="$(./depends/config.guess)"
fi
# Set HOST variable if not already set
if [[ -z "${HOST-}" ]]; then
    HOST="$BUILD"
fi

# Set CONFIGURE_FLAGS variable if not already set
if [[ -z "${CONFIGURE_FLAGS-}" ]]; then
    CONFIGURE_FLAGS=""
fi

# Use clang/clang++ compilers
export CC=clang
export CXX=clang++

# Set compilation flags for release build including -DNDEBUG to disable asserts
export CXXFLAGS='-fPIC -g -O3'

# Display clang version before starting the build
echo "Clang version:"
clang --version

if [ "x$*" = 'x--help' ]
then
    cat <<EOF
Usage:
$0 --help
  Show this help message and exit.
$0 [ --enable-lcov || --disable-tests ] [ --disable-mining ] [ --enable-proton ] [ --disable-libs ] [ MAKEARGS... ]
  Build the project and its dependencies from source.
  MAKEARGS are applied to both dependencies and the project itself.
EOF
    exit 0
fi

set -x

# Process arguments: enable lcov support or disable tests
LCOV_ARG=''
HARDENING_ARG='--enable-hardening'
TEST_ARG=''
if [ "x${1:-}" = 'x--enable-lcov' ]
then
    LCOV_ARG='--enable-lcov'
    HARDENING_ARG='--disable-hardening'
    shift
elif [ "x${1:-}" = 'x--disable-tests' ]
then
    TEST_ARG='--enable-tests=no'
    shift
fi

# Process argument to disable mining code
MINING_ARG=''
if [ "x${1:-}" = 'x--disable-mining' ]
then
    MINING_ARG='--enable-mining=no'
    shift
fi

# Process argument to enable Proton support
PROTON_ARG='--enable-proton=no'
if [ "x${1:-}" = 'x--enable-proton' ]
then
    PROTON_ARG=''
    shift
fi

PREFIX="$(pwd)/depends/$BUILD/"

# Build dependencies
HOST="$HOST" BUILD="$BUILD" "$MAKE" "$@" -C ./depends/ V=1

# Run autogen to prepare the build system
./autogen.sh

#sed -i.bak -e 's/gcc -m64/clang/g' -e 's/g++ -m64/clang++/g' "$PWD/depends/$HOST/share/config.site"
sed -i.bak -E 's/^(CC)="[^"]*"/\1="clang"/; s/^(CXX)="[^"]*"/\1="clang++"/' "$PWD/depends/$HOST/share/config.site"

# Run configure script with the necessary options and compiler flags
CONFIG_SITE="$PWD/depends/$HOST/share/config.site" \
./configure --prefix="${PREFIX}" --with-gui=qt5 --disable-bip70 --enable-tests=no --enable-wallet=yes "$HARDENING_ARG" "$LCOV_ARG" "$TEST_ARG" "$MINING_ARG" "$PROTON_ARG" $CONFIGURE_FLAGS

# Build the project
"$MAKE" "$@" V=1