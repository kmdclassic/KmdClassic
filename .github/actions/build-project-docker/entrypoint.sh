#!/usr/bin/env bash
set -euo pipefail

# Match the workspace owner so build products remain writable on the runner.
if [[ "$(id -u)" == 0 ]]; then
    BUILDER_NAME="${BUILDER_NAME:-builder}"
    BUILDER_UID="${BUILDER_UID:-1000}"
    BUILDER_GID="${BUILDER_GID:-1000}"
    if [[ ! "$BUILDER_UID" =~ ^[1-9][0-9]*$ || ! "$BUILDER_GID" =~ ^[1-9][0-9]*$ ]]; then
        echo "BUILDER_UID and BUILDER_GID must be nonzero numeric IDs." >&2
        exit 1
    fi
    if ! getent group "$BUILDER_GID" >/dev/null; then
        groupadd --gid "$BUILDER_GID" "$BUILDER_NAME"
    fi
    if getent passwd "$BUILDER_UID" >/dev/null; then
        BUILDER_NAME="$(getent passwd "$BUILDER_UID" | cut -d: -f1)"
    else
        useradd --create-home --shell /bin/bash --uid "$BUILDER_UID" \
            --gid "$BUILDER_GID" "$BUILDER_NAME"
    fi
    exec runuser -u "$BUILDER_NAME" -- /bin/bash "$0" "$@"
fi

WORKSPACE="$PWD"
BUILD_JOBS="${BUILD_JOBS:-2}"
if [[ ! "$BUILD_JOBS" =~ ^[1-9][0-9]*$ ]]; then
    echo "BUILD_JOBS must be a positive integer." >&2
    exit 1
fi
if [[ "$(uname -m)" != x86_64 || ! -f "$WORKSPACE/configure.ac" ]]; then
    echo "Run from the repository root on an x86_64 Linux builder." >&2
    exit 1
fi
if [[ -f "$WORKSPACE/config.status" ]]; then
    echo "The source tree is configured. Run make distclean before using separate CI build directories." >&2
    exit 1
fi

./autogen.sh
LINUX_HOST="$(./depends/config.guess)"

for target in linux windows; do
    if [[ "$target" == linux ]]; then
        host="$LINUX_HOST"
        suffix=""
        cxxflags="-O2 -g0"
        strip_tool='strip'
    else
        host=x86_64-w64-mingw32
        suffix=.exe
        cxxflags="-O2 -g0 -DCURL_STATICLIB"
        strip_tool='x86_64-w64-mingw32-strip'
    fi

    make -C depends HOST="$host" NO_PROTON=1 -j"$BUILD_JOBS"
    build_dir="$WORKSPACE/build-qt6-ci-$target"
    mkdir -p "$build_dir"
    (
        cd "$build_dir"
        CONFIG_SITE="$WORKSPACE/depends/$host/share/config.site" \
            "$WORKSPACE/configure" --with-gui=qt6 --disable-bip70 \
            --disable-tests --disable-bench CXXFLAGS="$cxxflags"
        make -j"$BUILD_JOBS"
    )

    release_dir="$WORKSPACE/releases/$target"
    mkdir -p "$release_dir"
    for binary in kmdclassicd kmdclassic-cli kmdclassic-tx wallet-utility; do
        install -m 755 "$build_dir/src/$binary$suffix" "$release_dir/$binary$suffix"
        "$strip_tool" "$release_dir/$binary$suffix"
    done
    install -m 755 "$build_dir/src/qt/kmdclassic-qt$suffix" "$release_dir/kmdclassic-qt$suffix"
    "$strip_tool" "$release_dir/kmdclassic-qt$suffix"
    if [[ "$target" == windows ]]; then
        install -m 644 zcutil/fetch-params.ps1 "$release_dir/fetch-params.ps1"
    fi
done
