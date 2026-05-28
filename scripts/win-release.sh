#!/bin/bash
set -e
set -x  # show commands

VERSION="v0.1.2-win64"

PROJECT_ROOT="$(pwd)/.."
SRC_DIR="$PROJECT_ROOT/src"
BIN_DIR="$PROJECT_ROOT/bin"
RELEASE_ROOT_DIR="$PROJECT_ROOT/release"
RELEASE_DIR="$RELEASE_ROOT_DIR/vidglue-$VERSION"
EXE_NAME=vidglue
EXE_PATH="$BIN_DIR/$EXE_NAME"
CONFIG_PATH="$PROJECT_ROOT/config.json"

# FFmpeg DLLs location for MinGW64
FFMPEG_BIN="/c/msys64/mingw64/bin"

# directory prepping
rm -rf "$BIN_DIR" "$RELEASE_DIR"
mkdir -p "$BIN_DIR" "$RELEASE_DIR"

# compile
CPP_FILES=$(ls "$SRC_DIR"/*.cpp 2>/dev/null)
if [ -z "$CPP_FILES" ]; then
    echo "No source files found in $SRC_DIR"
    exit 1
fi

g++ $CPP_FILES \
    -o "$EXE_PATH" \
    -fopenmp \
    $(pkg-config --cflags --libs libavformat libavcodec libavutil libswscale) \
    -static-libgcc -static-libstdc++

# copy executable and config
cp "$EXE_PATH" "$RELEASE_DIR/"
cp "$CONFIG_PATH" "$RELEASE_DIR/"

# copy FFmpeg + other MinGW64 DLLs for dynamic linking
DLLS=(
    avcodec-*.dll
    avformat-*.dll
    avutil-*.dll
    swscale-*.dll
    libwinpthread-1.dll
)

for dll_pattern in "${DLLS[@]}"; do
    matches=($FFMPEG_BIN/$dll_pattern)
    if [ -e "${matches[0]}" ]; then
        for f in "${matches[@]}"; do
            cp "$f" "$RELEASE_DIR/"
        done
    else
        echo "Warning: $dll_pattern not found, skipping."
    fi
done

# create zip of vidglue release
ZIP_FILE="$RELEASE_ROOT_DIR/vidglue-$VERSION.zip"
if command -v zip >/dev/null 2>&1; then
    rm -f "$ZIP_FILE"
    cd "$RELEASE_ROOT_DIR"
    zip -r "$ZIP_FILE" vidglue-$VERSION -x "*.DS_Store"
    echo "Zip created: $ZIP_FILE"
fi

echo "Release folder ready: $RELEASE_DIR"
ls -lh "$RELEASE_DIR"