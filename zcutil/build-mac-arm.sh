#!/usr/bin/env bash
set -euo pipefail

cd "$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)/.."
SOURCE_DIR="$PWD"

if [[ "${1:-}" == --help ]]; then
    cat <<'EOF'
Usage: ./zcutil/build-mac-arm.sh [MAKEARGS...]
Build the Qt 6 wallet natively on Apple Silicon using Xcode and depends.
Prerequisites: brew install autoconf automake libtool pkg-config coreutils cmake make
Overrides: MAKE, JOBS, BUILD_DIR, OSX_MIN_VERSION, CXXFLAGS, CONFIGURE_FLAGS.
The default application build directory is build-qt6-mac-arm.
EOF
    exit 0
fi

if [[ "$(uname -s)" != Darwin || "$(uname -m)" != arm64 ]]; then
    echo "This script requires a native arm64 macOS shell (not Rosetta)." >&2
    exit 1
fi

# Non-interactive SSH sessions do not normally load Homebrew's shell setup.
if [[ -d /opt/homebrew/bin ]]; then
    export PATH="/opt/homebrew/bin:/opt/homebrew/sbin:$PATH"
fi
MAKE="${MAKE:-gmake}"
for tool in "$MAKE" cmake python3 autoconf automake glibtoolize pkg-config xcrun; do
    if ! command -v "$tool" >/dev/null 2>&1; then
        echo "Missing build tool: $tool (see --help)." >&2
        exit 1
    fi
done
xcrun --show-sdk-path >/dev/null

# Keep the depends prefix stable across macOS updates, and use native arm64 Rust.
HOST=aarch64-apple-darwin
BUILD="$HOST"
OSX_MIN_VERSION="${OSX_MIN_VERSION:-13.0}"
export MACOSX_DEPLOYMENT_TARGET="$OSX_MIN_VERSION"
if [[ -z "${JOBS:-}" ]]; then
    JOBS=$(( $(sysctl -n hw.memsize) / 3221225472 ))
    (( JOBS >= 1 )) || JOBS=1
    CPU_COUNT="$(sysctl -n hw.ncpu)"
    (( JOBS <= CPU_COUNT )) || JOBS="$CPU_COUNT"
fi
BUILD_DIR="${BUILD_DIR:-$SOURCE_DIR/build-qt6-mac-arm}"
mkdir -p "$BUILD_DIR"
BUILD_DIR="$(cd "$BUILD_DIR" && pwd)"

"$MAKE" -C depends BUILD="$BUILD" HOST="$HOST" OSX_MIN_VERSION="$OSX_MIN_VERSION" \
    NO_PROTON=1 -j"$JOBS" "$@"
./autogen.sh
cd "$BUILD_DIR"
# CONFIGURE_FLAGS intentionally accepts a list of additional configure options.
CONFIG_SITE="$SOURCE_DIR/depends/$HOST/share/config.site" \
    "$SOURCE_DIR/configure" --build="$BUILD" --host="$HOST" \
    --disable-tests --disable-bench --disable-bip70 --with-gui=qt6 \
    ${CONFIGURE_FLAGS:-} CXXFLAGS="${CXXFLAGS:--O2 -g0}"
"$MAKE" -j"$JOBS" "$@"
"$MAKE" -j"$JOBS" appbundle OSX_APP=KmdClassic-Qt.app
# Seal the completed bundle for local use without a Developer ID certificate.
codesign --force --sign - "$BUILD_DIR/KmdClassic-Qt.app"
