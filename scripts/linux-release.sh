#!/bin/bash
set -e
set -x

VERSION="v0.1.1-linux64"

PROJECT_ROOT="$(pwd)/.."
SRC_DIR="$PROJECT_ROOT/src"
BIN_DIR="$PROJECT_ROOT/bin"
RELEASE_DIR="$PROJECT_ROOT/release/vidglue-$VERSION"

EXE_NAME="vidglue"
EXE_PATH="$BIN_DIR/$EXE_NAME"

rm -rf "$BIN_DIR" "$RELEASE_DIR"
mkdir -p "$BIN_DIR" "$RELEASE_DIR"

CPP_FILES=$(ls "$SRC_DIR"/*.cpp 2>/dev/null)

g++ $CPP_FILES \
    -o "$EXE_PATH" \
    -O2 \
    -fopenmp \
    -std=c++17 \
    $(pkg-config --cflags --libs \
        libavformat \
        libavcodec \
        libavutil \
        libswscale \
        x264)

cp "$EXE_PATH" "$RELEASE_DIR/"
cp "$PROJECT_ROOT/config.json" "$RELEASE_DIR/"

echo "Build complete"