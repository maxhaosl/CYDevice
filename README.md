# CYDevice

[English](README.md) | [中文](README_zh.md)

CYDevice is a cross-platform audio and video collection library designed for capturing audio and video data from system devices. It provides a unified API for device enumeration, initialization, and data capture across multiple platforms.

## Features

- **Cross-platform Support**: Windows (DirectShow), with planned support for macOS, iOS, Linux, and Android
- **Audio & Video Capture**: Simultaneous capture of audio and video streams from system devices
- **Device Enumeration**: Query available audio/video input devices
- **Callback-based Architecture**: Real-time data delivery through callback interfaces
- **C++20 Standard**: Modern C++ implementation with coroutine support
- **Static Library**: Lightweight static library for easy integration

## Supported Platforms

- **Windows**: x86, x64 (Visual Studio 2019/2022)
  - Runtime Libraries: MT (MultiThreaded), MD (MultiThreadedDLL)
  - Build Types: Debug, Release
- **macOS**: arm64, x86_64, universal (planned)
- **iOS**: arm64, x86_64 simulator, universal (planned)
- **Linux**: x86_64, x86 (planned)
- **Android**: arm64-v8a, armeabi-v7a, x86_64, x86 (planned)

## Dependencies

- **CYCoroutine**: C++20 coroutine library (bundled in `ThirdParty/CYCoroutine`)
- **CYLogger**: Cross-platform logging library (bundled in `ThirdParty/CYLogger`)
- **libyuv**: Video format conversion library (bundled in `ThirdParty/libyuv`)
- **libsamplerate**: Audio resampling library (bundled in `ThirdParty/libsamplerate`)
- **CMake 3.15+**: Build system
- **C++20-compatible compiler**: MSVC 19.3x, Clang 14+, GCC 11+

## Build Requirements

### Windows
- Visual Studio 2019/2022 with C++ desktop development workload
- CMake 3.15 or later
- Windows SDK 10.0

### macOS/iOS
- Xcode command-line tools
- CMake 3.15 or later
- macOS 11.0+ (for macOS builds)
- iOS 14.0+ (for iOS builds)

### Linux
- GCC or Clang compiler
- CMake 3.15 or later
- Build essentials (`build-essential` on Debian/Ubuntu)

### Android
- Android SDK + NDK (r26b+)
- CMake 3.15 or later
- NDK toolchain will be auto-detected from common SDK locations

## Quick Start

### Building on Windows

Use the provided batch script to build all configurations:

```batch
cd Build
build_all_windows.bat
```

This will generate libraries in `Bin/Windows/{x86|x64}/{MD|MT}/{Debug|Release}/`

### Building on Linux

```bash
cd Build
./build_linux.sh [Release|Debug] [x86_64|x86|arm64]
```

Output will be in `Bin/Linux/{arch}/{config}/`

### Building on macOS

```bash
cd Build
./build_mac.sh
```

Output will be in `Bin/macOS/{arch}/{config}/` with universal binaries in `Bin/macOS/universal/{config}/`

### Building for iOS

```bash
cd Build
./build_ios.sh
```

Output will be in `Bin/iOS/{arch}/{config}/` with universal binaries in `Bin/iOS/universal/{config}/`

### Building for Android

```bash
cd Build
./build_android.sh
```

Output will be in `Bin/Android/{abi}/{config}/`

## Project Structure

```
CYDevice/
├── Bin/                    # Build output directory
├── Build/                  # Build scripts
│   ├── Windows/           # Visual Studio project files
│   ├── build_all_windows.bat
│   ├── build_linux.sh
│   ├── build_mac.sh
│   ├── build_ios.sh
│   └── build_android.sh
├── Inc/                    # Public headers
│   └── CYDevice/
│       ├── ICYDevice.hpp
│       ├── CYDeviceDefine.hpp
│       ├── CYDeviceHelper.hpp
│       └── CYDeviceFatory.hpp
├── Src/                    # Source files
│   ├── Capture/           # Capture implementation
│   ├── Common/            # Common utilities
│   ├── Control/           # Control logic
│   └── CYDeviceImpl.cpp
├── ThirdParty/            # Third-party dependencies
│   ├── CYCoroutine/
│   ├── CYLogger/
│   ├── libyuv/
│   └── libsamplerate/
├── Samples/               # Example applications
│   └── CYDeviceTest/
├── CMakeLists.txt         # CMake build configuration
└── README.md
```

## Usage Example

```cpp
#include "CYDevice/ICYDevice.hpp"
#include "CYDevice/CYDeviceFatory.hpp"

using namespace cry;

// Create device instance
ICYDevice* pDevice = CYDeviceFactory::CreateDevice();

// Initialize device
int16_t result = pDevice->Init(
    1920, 1080, 30,                    // Video: width, height, fps
    L"Camera Name", L"Camera ID",       // Video device
    48000,                              // Audio sample rate
    L"Microphone Name", L"Mic ID",      // Audio device
    true                                // Use render
);

if (result == CYERR_SUCESS) {
    // Implement callbacks
    class MyVideoCallback : public ICYVideoDataCallBack {
        void OnVideoData(uint8_t* pBuffer, uint32_t nWidth, uint32_t nHeight, 
                        ECYVideoType eType, uint64_t nTimeStamps) override {
            // Process video data
        }
    };
    
    class MyAudioCallback : public ICYAudioDataCallBack {
        void OnAudioData(float* pBuffer, uint32_t nNumberAudioFrames, 
                        uint32_t nChannel, uint64_t nTimeStamps) override {
            // Process audio data
        }
    };
    
    MyVideoCallback videoCb;
    MyAudioCallback audioCb;
    
    // Start capture
    pDevice->StartCapture(&audioCb, &videoCb);
    
    // ... do work ...
    
    // Stop capture
    pDevice->StopCapture();
    
    // Cleanup
    pDevice->UnInit();
}

// Destroy device
CYDeviceFactory::DestroyDevice(pDevice);
```

## API Overview

### Core Interface: `ICYDevice`

- `Init()`: Initialize device with video/audio parameters
- `UnInit()`: Cleanup and release resources
- `StartCapture()`: Start capturing with callbacks
- `StopCapture()`: Stop capturing
- `GetNextAudioBuffer()`: Get next audio buffer

### Helper Functions

- `GetDeviceList()`: Enumerate available devices (video/audio input/output)

### Error Codes

- `CYERR_SUCESS`: Operation successful
- `CYERR_FAILED`: General failure
- `CYERR_CREATE_GRAPH_FAILED`: Failed to create DirectShow graph
- `CYERR_CREATE_GRAPHBUILDER_FAILED`: Failed to create GraphBuilder
- `CYERR_REPEAT_START_CAPTURE`: Attempted to start capture while already running
- `CYEER_NOT_START_CAPTURE`: Operation requires capture to be started
- `CYERR_CAPTURE_RUN_FAILED`: Capture run failed

## Build Output Structure

Libraries are organized by platform, architecture, runtime library (Windows only), and build type:

```
Bin/
├── Windows/
│   ├── x86/
│   │   ├── MD/
│   │   │   ├── Debug/    → CYDeviceD.lib
│   │   │   └── Release/  → CYDevice.lib
│   │   └── MT/
│   │       ├── Debug/    → CYDeviceD.lib
│   │       └── Release/ → CYDevice.lib
│   └── x64/
│       ├── MD/
│       │   ├── Debug/    → CYDeviceD.lib
│       │   └── Release/  → CYDevice.lib
│       └── MT/
│           ├── Debug/    → CYDeviceD.lib
│           └── Release/  → CYDevice.lib
├── Linux/
│   └── {arch}/
│       ├── Debug/        → libCYDeviceD.a
│       └── Release/      → libCYDevice.a
├── macOS/
│   ├── {arch}/
│   │   ├── Debug/        → libCYDeviceD.a
│   │   └── Release/      → libCYDevice.a
│   └── universal/
│       ├── Debug/        → libCYDeviceD.a (universal)
│       └── Release/      → libCYDevice.a (universal)
├── iOS/
│   └── (similar to macOS)
└── Android/
    └── {abi}/
        ├── Debug/        → libCYDeviceD.a
        └── Release/      → libCYDevice.a
```

## License

CYDevice is licensed under the MIT License. See the LICENSE file for details.

## Authors

- ShiLiang.Hao <newhaosl@163.com>
- foobra <vipgs99@gmail.com>

## Version

Current version: 1.0.0

## Contributing

Contributions are welcome! Please feel free to submit pull requests or open issues for bugs and feature requests.

## Change Log

See [Change.log](Change.log) for detailed version history.

