#!/bin/bash

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(dirname "$SCRIPT_DIR")"
CMAKE_SOURCE_DIR="$PROJECT_ROOT"
OUTPUT_BASE="$PROJECT_ROOT/Bin"
mkdir -p "$OUTPUT_BASE"

if [[ "$(uname -s)" != "Darwin" ]]; then
    echo "iOS builds require a macOS host."
    exit 1
fi

if ! command -v cmake >/dev/null 2>&1; then
    echo "CMake is required but was not found in PATH."
    exit 1
fi

if ! command -v xcodebuild >/dev/null 2>&1; then
    echo "Xcode command line tools are missing. Run 'xcode-select --install' first."
    exit 1
fi

IOS_DEPLOYMENT_TARGET=${IOS_DEPLOYMENT_TARGET:-"14.0"}
BUILD_TYPES=(${CYDEVICE_BUILD_TYPES:-Release})
IOS_ARCHES=(${CYDEVICE_IOS_ARCHES:-arm64 x86_64})

detect_jobs() {
    if command -v sysctl >/dev/null 2>&1; then
        sysctl -n hw.ncpu 2>/dev/null && return
    fi
    echo 4
}
BUILD_JOBS=${BUILD_JOBS:-$(detect_jobs)}

ios_sysroot_for_arch() {
    local arch=$1
    case "$arch" in
        arm64) echo "iphoneos" ;;
        x86_64|arm64-simulator) echo "iphonesimulator" ;;
        *)
            echo "Unsupported iOS architecture: $arch"
            exit 1
            ;;
    esac
}

build_slice() {
    local arch=$1
    local build_type=$2

    local build_dir="$SCRIPT_DIR/build_ios_${arch}_${build_type}"
    local sysroot
    sysroot=$(ios_sysroot_for_arch "$arch")

    echo "=== iOS / $arch / $build_type ==="

    cmake -S "$CMAKE_SOURCE_DIR" \
          -B "$build_dir" \
          -DCMAKE_SYSTEM_NAME=iOS \
          -DCMAKE_OSX_ARCHITECTURES="$arch" \
          -DCMAKE_OSX_SYSROOT="$sysroot" \
          -DCMAKE_OSX_DEPLOYMENT_TARGET="$IOS_DEPLOYMENT_TARGET" \
          -DCMAKE_BUILD_TYPE="$build_type"

    cmake --build "$build_dir" --target CYDevice --parallel "$BUILD_JOBS"

    local arch_dir="$OUTPUT_BASE/iOS/$arch/$build_type"
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
        echo "Skipping iOS universal ${build_type}: lipo not found."
        return
    fi

    local lib_name
    if [ "$build_type" = "Debug" ]; then
        lib_name="libCYDeviceD.a"
    else
        lib_name="libCYDevice.a"
    fi

    local -a candidate_arches=("${IOS_ARCHES[@]}")
    local canonical
    for canonical in arm64 x86_64 arm64-simulator; do
        local seen=0
        local existing
        for existing in "${candidate_arches[@]}"; do
            if [ "$existing" = "$canonical" ]; then
                seen=1
                break
            fi
        done
        if [ "$seen" -eq 0 ]; then
            candidate_arches+=("$canonical")
        fi
    done

    local -a inputs=()
    local arch
    for arch in "${candidate_arches[@]}"; do
        local lib_path="$OUTPUT_BASE/iOS/$arch/$build_type/$lib_name"
        if [ -f "$lib_path" ]; then
            local duplicate=0
            if [ "${#inputs[@]}" -gt 0 ]; then
                local existing_path
                for existing_path in "${inputs[@]}"; do
                    if [ "$existing_path" = "$lib_path" ]; then
                        duplicate=1
                        break
                    fi
                done
            fi
            [ "$duplicate" -eq 1 ] && continue
            inputs+=("$lib_path")
        fi
    done

    if [ ${#inputs[@]} -lt 2 ]; then
        echo "Skipping iOS $build_type universal library: need at least two slices."
        return
    fi

    local dest_dir="$OUTPUT_BASE/iOS/universal/$build_type"
    mkdir -p "$dest_dir"
    local dest="$dest_dir/$lib_name"

    echo "Creating iOS $build_type universal library (${#inputs[@]} slices)..."
    if [ "${#inputs[@]}" -gt 0 ]; then
        lipo -create "${inputs[@]}" -output "$dest"
    fi

    echo "Universal artifact: $dest"
}

echo "========================================"
echo "Building CYDevice for iOS"
echo "Output root: $OUTPUT_BASE"
echo "Architectures: ${IOS_ARCHES[*]}"
echo "Build types: ${BUILD_TYPES[*]}"
echo "========================================"

for build_type in "${BUILD_TYPES[@]}"; do
    for arch in "${IOS_ARCHES[@]}"; do
        build_slice "$arch" "$build_type"
    done
    combine_universal "$build_type"
done

echo "iOS builds complete. See $OUTPUT_BASE/iOS/* for artifacts."

