#!/usr/bin/env bash

# Build the given target

set -euo pipefail

PROJECT_DIR=$(
  cd "$(dirname "$0")"/../../..
  pwd
)

conan build "${PROJECT_DIR}"
