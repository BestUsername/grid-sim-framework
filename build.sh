#!/usr/bin/env bash

set -euo pipefail

repo_root="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
build_dir="${BUILD_DIR:-"${repo_root}/build"}"
build_type="${CMAKE_BUILD_TYPE:-Debug}"
parallelism="${CMAKE_BUILD_PARALLEL_LEVEL:-2}"

cmake -S "${repo_root}" -B "${build_dir}" -DCMAKE_BUILD_TYPE="${build_type}"
cmake --build "${build_dir}" --parallel "${parallelism}"
