#!/bin/bash
set -e
set -x  # show commands

# -----------------------
# Config
# -----------------------
PROJECT_ROOT="$(pwd)/.."
SRC_DIR="$PROJECT_ROOT/src"
BIN_DIR="$PROJECT_ROOT/bin"
RELEASE_ROOT_DIR="$PROJECT_ROOT/release"
RELEASE_DIR="$RELEASE_ROOT_DIR/vidglue-win64"
EXE_NAME=vidglue
EXE_PATH="$BIN_DIR/$EXE_NAME"

# FFmpeg DLLs location
FFMPEG_BIN="/c/msys64/mingw64/bin"

# -----------------------
# Step 1: Prepare folders
# -----------------------
rm -rf "$BIN_DIR" "$RELEASE_DIR"
mkdir -p "$BIN_DIR" "$RELEASE_DIR"

# -----------------------
# Step 2: Compile C++ project
# -----------------------
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

# -----------------------
# Step 3: Copy executable
# -----------------------
cp "$EXE_PATH" "$RELEASE_DIR/"

# -----------------------
# Step 4: Copy FFmpeg + MinGW DLLs
# -----------------------
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

# -----------------------
# Step 5: Create zip of vidglue folder
# -----------------------
ZIP_FILE="$RELEASE_ROOT_DIR/vidglue-win64.zip"
if command -v zip >/dev/null 2>&1; then
    rm -f "$ZIP_FILE"
    cd "$RELEASE_ROOT_DIR"
    zip -r "$ZIP_FILE" vidglue-win64 -x "*.DS_Store"
    echo "Zip created: $ZIP_FILE"
fi

# -----------------------
# Done
# -----------------------
echo "Release folder ready: $RELEASE_DIR"
ls -lh "$RELEASE_DIR"