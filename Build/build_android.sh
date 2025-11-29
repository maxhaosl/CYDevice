#!/bin/bash

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(dirname "$SCRIPT_DIR")"
CMAKE_SOURCE_DIR="$PROJECT_ROOT"
OUTPUT_BASE="$PROJECT_ROOT/Bin"
mkdir -p "$OUTPUT_BASE"

if ! command -v cmake >/dev/null 2>&1; then
    echo "CMake is required but was not found in PATH."
    exit 1
fi

ANDROID_API_LEVEL_DEFAULT=${ANDROID_API_LEVEL:-"23"}
BUILD_TYPES=(${CYDEVICE_BUILD_TYPES:-Release})
ANDROID_ABIS=(${CYDEVICE_ANDROID_ABIS:-arm64-v8a armeabi-v7a x86_64 x86})

detect_jobs() {
    if command -v nproc >/dev/null 2>&1; then
        nproc && return
    fi
    if command -v sysctl >/dev/null 2>&1; then
        sysctl -n hw.ncpu 2>/dev/null && return
    fi
    echo 4
}
BUILD_JOBS=${BUILD_JOBS:-$(detect_jobs)}

detect_android_sdk() {
    local -a candidates=(
        "${ANDROID_SDK_ROOT:-}"
        "${ANDROID_HOME:-}"
        "${HOME}/Library/Android/sdk"
        "${HOME}/Android/Sdk"
        "${HOME}/Android/sdk"
        "/usr/local/share/android-sdk"
        "/opt/android-sdk"
    )
    local path
    for path in "${candidates[@]}"; do
        if [ -n "$path" ] && [ -d "$path" ]; then
            echo "$path"
            return
        fi
    done
}

ndk_has_toolchain() {
    local ndk_root=$1
    if [ -n "$ndk_root" ] && [ -f "$ndk_root/build/cmake/android.toolchain.cmake" ]; then
        return 0
    fi
    return 1
}

detect_android_ndk() {
    local sdk_root=$1
    local -a candidates=(
        "${ANDROID_NDK_HOME:-}"
        "${ANDROID_NDK_ROOT:-}"
        "${ANDROID_NDK:-}"
    )
    local path
    for path in "${candidates[@]}"; do
        if [ -n "$path" ] && [ -d "$path" ] && ndk_has_toolchain "$path"; then
            echo "$path"
            return
        fi
    done

    if [ -d "$sdk_root/ndk" ]; then
        local candidate
        for candidate in $(ls -1 "$sdk_root/ndk" | sort -r); do
            local full_path="$sdk_root/ndk/$candidate"
            if [ -d "$full_path" ] && ndk_has_toolchain "$full_path"; then
                echo "$full_path"
                return
            fi
        done
    fi

    if [ -d "$sdk_root/ndk-bundle" ] && ndk_has_toolchain "$sdk_root/ndk-bundle"; then
        echo "$sdk_root/ndk-bundle"
    fi
}

prepare_android_toolchain() {
    if [ "${ANDROID_SETUP_STATE:-}" = "ready" ]; then
        return 0
    fi

    local sdk_root
    sdk_root=$(detect_android_sdk)
    if [ -z "$sdk_root" ]; then
        echo "Android SDK not found. Install it or set ANDROID_SDK_ROOT."
        exit 1
    fi

    local ndk_root
    ndk_root=$(detect_android_ndk "$sdk_root")
    if [ -z "$ndk_root" ]; then
        echo "Android NDK not found under $sdk_root. Install it via sdkmanager."
        exit 1
    fi

    export ANDROID_SDK_ROOT="$sdk_root"
    export ANDROID_HOME="$sdk_root"
    export ANDROID_NDK_HOME="$ndk_root"
    export ANDROID_NDK_ROOT="$ndk_root"
    ANDROID_SETUP_STATE="ready"

    echo "Using Android SDK: $ANDROID_SDK_ROOT"
    echo "Using Android NDK: $ANDROID_NDK_HOME"
}

android_api_for_abi() {
    local abi=$1
    local min_api
    case "$abi" in
        armeabi-v7a|x86) min_api=19 ;;
        *) min_api=21 ;;
    esac

    local requested="$ANDROID_API_LEVEL_DEFAULT"
    if ! [[ $requested =~ ^[0-9]+$ ]]; then
        requested=$min_api
    fi

    if [ "$requested" -lt "$min_api" ]; then
        echo "$min_api"
    else
        echo "$requested"
    fi
}

build_slice() {
    local abi=$1
    local build_type=$2

    local api_level
    api_level=$(android_api_for_abi "$abi")
    local build_dir="$SCRIPT_DIR/build_android_${abi}_${build_type}"

    echo "=== Android / $abi / $build_type (API $api_level) ==="

    cmake -S "$CMAKE_SOURCE_DIR" \
          -B "$build_dir" \
          -DCMAKE_TOOLCHAIN_FILE="$ANDROID_NDK_HOME/build/cmake/android.toolchain.cmake" \
          -DANDROID_ABI="$abi" \
          -DANDROID_PLATFORM="android-$api_level" \
          -DANDROID_STL=c++_static \
          -DCMAKE_BUILD_TYPE="$build_type"

    cmake --build "$build_dir" --target CYDevice --parallel "$BUILD_JOBS"

    local abi_dir="$OUTPUT_BASE/Android/$abi/$build_type"
    mkdir -p "$abi_dir"
    
    # Copy library
    local lib_name
    if [ "$build_type" = "Debug" ]; then
        lib_name="libCYDeviceD.a"
    else
        lib_name="libCYDevice.a"
    fi
    
    local lib_found=0
    if [ -f "$build_dir/lib/$lib_name" ]; then
        cp "$build_dir/lib/$lib_name" "$abi_dir/"
        lib_found=1
    elif [ -f "$build_dir/lib/$build_type/$lib_name" ]; then
        cp "$build_dir/lib/$build_type/$lib_name" "$abi_dir/"
        lib_found=1
    else
        local found_lib=$(find "$build_dir" -name "$lib_name" -type f | head -1)
        if [ -n "$found_lib" ]; then
            cp "$found_lib" "$abi_dir/"
            lib_found=1
        fi
    fi
    
    if [ "$lib_found" -eq 1 ]; then
        echo "Library copied to: $abi_dir/$lib_name"
    else
        echo "WARNING: Library file not found: $lib_name"
    fi
    
    echo "Artifacts: $abi_dir"
}

prepare_android_toolchain

echo "========================================"
echo "Building CYDevice for Android"
echo "Output root: $OUTPUT_BASE"
echo "ABIs: ${ANDROID_ABIS[*]}"
echo "Build types: ${BUILD_TYPES[*]}"
echo "========================================"

for build_type in "${BUILD_TYPES[@]}"; do
    for abi in "${ANDROID_ABIS[@]}"; do
        build_slice "$abi" "$build_type"
    done
done

echo "Android builds complete. See $OUTPUT_BASE/Android/* for artifacts."

