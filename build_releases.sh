#!/usr/bin/env bash
# Use the same Ubuntu 22.04 builder as build-project.yml.
set -euo pipefail
cd "$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

BUILD_IMAGE="${BUILD_IMAGE:-kmdclassic-qt6-builder}"
BUILD_JOBS="${BUILD_JOBS:-2}"
docker build --tag "$BUILD_IMAGE" .github/actions/build-project-docker
docker run --rm \
    --mount "type=bind,source=$PWD,target=$PWD" --workdir "$PWD" \
    --env "BUILDER_UID=$(id -u)" --env "BUILDER_GID=$(id -g)" \
    --env "BUILD_JOBS=$BUILD_JOBS" "$BUILD_IMAGE"
