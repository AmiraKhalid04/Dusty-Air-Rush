#!/usr/bin/env bash
set -euo pipefail

# Build the game for Linux using only project-relative paths.
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT_DIR="$(cd "${SCRIPT_DIR}/.." && pwd)"

BUILD_DIR_NAME="${1:-build-linux}"
BUILD_TYPE="${2:-Release}"

BUILD_DIR="${ROOT_DIR}/${BUILD_DIR_NAME}"

cmake -S "${ROOT_DIR}" -B "${BUILD_DIR}" -DCMAKE_BUILD_TYPE="${BUILD_TYPE}"
cmake --build "${BUILD_DIR}" --config "${BUILD_TYPE}" -j

echo "Build complete: ${ROOT_DIR}/bin/GAME_APPLICATION"
