#!/bin/bash

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(dirname "$SCRIPT_DIR")"
CMAKE_SOURCE_DIR="$PROJECT_ROOT"
OUTPUT_BASE="$PROJECT_ROOT/Bin"
mkdir -p "$OUTPUT_BASE"

if [[ "$(uname -s)" != "Darwin" ]]; then
    echo "This script must be run on macOS."
    exit 1
fi

if ! command -v cmake >/dev/null 2>&1; then
    echo "CMake is required but was not found in PATH."
    exit 1
fi

if ! xcode-select -p >/dev/null 2>&1; then
    echo "Xcode command line tools are missing. Run 'xcode-select --install' first."
    exit 1
fi

MACOS_DEPLOYMENT_TARGET=${MACOS_DEPLOYMENT_TARGET:-"11.0"}
BUILD_TYPES=(${CYDEVICE_BUILD_TYPES:-Release})
MAC_ARCHES=(${CYDEVICE_MAC_ARCHES:-arm64 x86_64})

detect_jobs() {
    if command -v sysctl >/dev/null 2>&1; then
        sysctl -n hw.ncpu 2>/dev/null && return
    fi
    if command -v nproc >/dev/null 2>&1; then
        nproc && return
    fi
    echo 4
}
BUILD_JOBS=${BUILD_JOBS:-$(detect_jobs)}

build_slice() {
    local arch=$1
    local build_type=$2

    local build_dir="$SCRIPT_DIR/build_macos_${arch}_${build_type}"

    echo "=== macOS / $arch / $build_type ==="

    cmake -S "$CMAKE_SOURCE_DIR" \
          -B "$build_dir" \
          -DCMAKE_BUILD_TYPE="$build_type" \
          -DCMAKE_OSX_DEPLOYMENT_TARGET="$MACOS_DEPLOYMENT_TARGET" \
          -DCMAKE_OSX_ARCHITECTURES="$arch"

    cmake --build "$build_dir" --target CYDevice --parallel "$BUILD_JOBS"

    local arch_dir="$OUTPUT_BASE/macOS/$arch/$build_type"
    mkdir -p "$arch_dir"
    
    # Copy library
    local lib_name
    if [ "$build_type" = "Debug" ]; then
        lib_name="libCYDeviceD.a"
    else
        lib_name="libCYDevice.a"
    fi
    
    local lib_found=0
    if [ -f "$build_dir/lib/$lib_name" ]; then
        cp "$build_dir/lib/$lib_name" "$arch_dir/"
        lib_found=1
    elif [ -f "$build_dir/lib/$build_type/$lib_name" ]; then
        cp "$build_dir/lib/$build_type/$lib_name" "$arch_dir/"
        lib_found=1
    else
        local found_lib=$(find "$build_dir" -name "$lib_name" -type f | head -1)
        if [ -n "$found_lib" ]; then
            cp "$found_lib" "$arch_dir/"
            lib_found=1
        fi
    fi
    
    if [ "$lib_found" -eq 1 ]; then
        echo "Library copied to: $arch_dir/$lib_name"
    else
        echo "WARNING: Library file not found: $lib_name"
    fi
    
    echo "Artifacts: $arch_dir"
}

combine_universal() {
    local build_type=$1

    if ! command -v lipo >/dev/null 2>&1; then
        echo "Skipping macOS universal ${build_type}: lipo not found."
        return
    fi

    local lib_name
    if [ "$build_type" = "Debug" ]; then
        lib_name="libCYDeviceD.a"
    else
        lib_name="libCYDevice.a"
    fi

    local src_arm="$OUTPUT_BASE/macOS/arm64/$build_type/$lib_name"
    local src_x86="$OUTPUT_BASE/macOS/x86_64/$build_type/$lib_name"

    if [ ! -f "$src_arm" ] || [ ! -f "$src_x86" ]; then
        echo "Skipping macOS $build_type universal library: missing slices."
        [ ! -f "$src_arm" ] && echo "  Missing: $src_arm"
        [ ! -f "$src_x86" ] && echo "  Missing: $src_x86"
        return
    fi

    local dest_dir="$OUTPUT_BASE/macOS/universal/$build_type"
    mkdir -p "$dest_dir"
    local dest="$dest_dir/$lib_name"

    echo "Creating macOS $build_type universal library..."
    lipo -create "$src_arm" "$src_x86" -output "$dest"

    echo "Universal artifact: $dest"
}

echo "========================================"
echo "Building CYDevice for macOS"
echo "Output root: $OUTPUT_BASE"
echo "Architectures: ${MAC_ARCHES[*]}"
echo "Build types: ${BUILD_TYPES[*]}"
echo "========================================"

for build_type in "${BUILD_TYPES[@]}"; do
    for arch in "${MAC_ARCHES[@]}"; do
        build_slice "$arch" "$build_type"
    done
    combine_universal "$build_type"
done

echo "macOS builds complete. See $OUTPUT_BASE/macOS/* for artifacts."

