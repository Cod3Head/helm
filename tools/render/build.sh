#!/bin/sh
# Builds the headless renderer (Linux/macOS, any C++17 compiler). Run from the repo root.
set -e
OUT=${1:-build/kindred_render}
mkdir -p "$(dirname "$OUT")"
g++ -std=c++14 -O2 -msse2 -Imopo/src -Isrc -Isrc/synthesis -Isrc/common \
  mopo/src/*.cpp src/synthesis/*.cpp src/common/helm_common.cpp tools/render/render.cpp -o "$OUT"
echo "built $OUT"
