#!/usr/bin/env bash

# Configure the build system with given CMake configuration options

set -exuo pipefail

PROJECT_DIR=$(
  cd "$(dirname "$0")"/../../..
  pwd
)

conan install \
  --build=missing \
  "${PROJECT_DIR}"

# cmake \
#   -DCMAKE_CXX_COMPILER_LAUNCHER=ccache \
#   -DCMAKE_PROJECT_INCLUDE:FILEPATH="$(pwd)"/conan_paths.cmake \
#   -G Ninja \
#   "$@" \
#   "${PROJECT_DIR}"
