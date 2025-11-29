#!/bin/bash

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(dirname "$SCRIPT_DIR")"
CMAKE_SOURCE_DIR="$PROJECT_ROOT"
OUTPUT_BASE="$PROJECT_ROOT/Bin"
mkdir -p "$OUTPUT_BASE"

BUILD_TYPE=${1:-Release}
RAW_ARCH=${2:-$(uname -m)}

canonicalize_linux_arch() {
    local token lower
    token=$(printf '%s' "$1" | xargs)
    lower=$(printf '%s' "$token" | tr '[:upper:]' '[:lower:]')
    case "$lower" in
        x86_64|amd64|x64)
            echo "x86_64"
            ;;
        i386|i686|x86)
            echo "x86"
            ;;
        arm64|aarch64)
            echo "arm64"
            ;;
        *)
            echo "$token"
            ;;
    esac
}

detect_jobs() {
    if command -v nproc >/dev/null 2>&1; then
        nproc && return
    fi
    if command -v sysctl >/dev/null 2>&1; then
        sysctl -n hw.logicalcpu 2>/dev/null && return
    fi
    echo 4
}

detect_compiler() {
    local override="$1" required="$2" resolved
    if [ -n "$override" ]; then
        echo "$override"
        return
    fi
    if command -v "$required" >/dev/null 2>&1; then
        resolved=$(command -v "$required")
        echo "$resolved"
        return
    fi
    echo ""
}

TARGET_ARCH=$(canonicalize_linux_arch "$RAW_ARCH")
CC_BIN=$(detect_compiler "${CYDEVICE_CC:-}" "gcc")
CXX_BIN=$(detect_compiler "${CYDEVICE_CXX:-}" "g++")

if [ -z "$CC_BIN" ] || [ -z "$CXX_BIN" ]; then
    echo "Error: GCC toolchain not found (set CYDEVICE_CC/CYDEVICE_CXX to override)." >&2
    exit 1
fi

if [ "$TARGET_ARCH" = "x86" ]; then
    ARCH_FLAG_DESC="(32-bit)"
elif [ "$TARGET_ARCH" = "x86_64" ]; then
    ARCH_FLAG_DESC="(64-bit)"
else
    ARCH_FLAG_DESC=""
fi

echo "========================================"
echo "Building CYDevice for Linux"
echo "Build Type: $BUILD_TYPE"
if [ "$RAW_ARCH" != "$TARGET_ARCH" ]; then
    echo "Target Architecture: $TARGET_ARCH $ARCH_FLAG_DESC (requested: $RAW_ARCH)"
else
    echo "Target Architecture: $TARGET_ARCH $ARCH_FLAG_DESC"
fi
echo "========================================"

BUILD_SUBDIR="build_linux_${TARGET_ARCH}_${BUILD_TYPE}"
BUILD_PATH="$SCRIPT_DIR/$BUILD_SUBDIR"
mkdir -p "$BUILD_PATH"

CMAKE_ARGS=(
    -S "$CMAKE_SOURCE_DIR"
    -B "$BUILD_PATH"
    "-DCMAKE_C_COMPILER=$CC_BIN"
    "-DCMAKE_CXX_COMPILER=$CXX_BIN"
    "-DCMAKE_BUILD_TYPE=$BUILD_TYPE"
    "-DCMAKE_CXX_STANDARD=20"
    "-DCMAKE_CXX_STANDARD_REQUIRED=ON"
)

C_FLAGS=()
CXX_FLAGS=()
EXE_LINKER_FLAGS=()

if [ "$TARGET_ARCH" = "x86" ]; then
    C_FLAGS+=("-m32")
    CXX_FLAGS+=("-m32")
    EXE_LINKER_FLAGS+=("-m32")
elif [ "$TARGET_ARCH" = "x86_64" ]; then
    C_FLAGS+=("-m64")
    CXX_FLAGS+=("-m64")
fi

if [ ${#C_FLAGS[@]} -gt 0 ]; then
    CMAKE_ARGS+=("-DCMAKE_C_FLAGS=${C_FLAGS[*]}")
fi
if [ ${#CXX_FLAGS[@]} -gt 0 ]; then
    CMAKE_ARGS+=("-DCMAKE_CXX_FLAGS=${CXX_FLAGS[*]}")
fi
if [ ${#EXE_LINKER_FLAGS[@]} -gt 0 ]; then
    CMAKE_ARGS+=("-DCMAKE_EXE_LINKER_FLAGS=${EXE_LINKER_FLAGS[*]}")
fi

cmake "${CMAKE_ARGS[@]}"
cmake --build "$BUILD_PATH" --target CYDevice --parallel "$(detect_jobs)"

OUTPUT_DIR="$OUTPUT_BASE/Linux/$TARGET_ARCH/$BUILD_TYPE"
mkdir -p "$OUTPUT_DIR"

# Copy library files
if [ "$BUILD_TYPE" = "Debug" ]; then
    LIB_NAME="libCYDeviceD.a"
else
    LIB_NAME="libCYDevice.a"
fi

# Find and copy library (try multiple possible locations)
LIB_FOUND=0
if [ -f "$BUILD_PATH/lib/$LIB_NAME" ]; then
    cp "$BUILD_PATH/lib/$LIB_NAME" "$OUTPUT_DIR/"
    LIB_FOUND=1
elif [ -f "$BUILD_PATH/lib/$BUILD_TYPE/$LIB_NAME" ]; then
    cp "$BUILD_PATH/lib/$BUILD_TYPE/$LIB_NAME" "$OUTPUT_DIR/"
    LIB_FOUND=1
else
    # Try to find it anywhere in build directory
    FOUND_LIB=$(find "$BUILD_PATH" -name "$LIB_NAME" -type f | head -1)
    if [ -n "$FOUND_LIB" ]; then
        cp "$FOUND_LIB" "$OUTPUT_DIR/"
        LIB_FOUND=1
    fi
fi

if [ "$LIB_FOUND" -eq 1 ]; then
    echo "Library copied to: $OUTPUT_DIR/$LIB_NAME"
else
    echo "WARNING: Library file not found: $LIB_NAME"
    echo "Searched in: $BUILD_PATH/lib/ and $BUILD_PATH/lib/$BUILD_TYPE/"
fi

echo "CYDevice artifacts staged at: $OUTPUT_DIR"

