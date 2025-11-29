cmake_minimum_required(VERSION 3.15)
project(CYDevice VERSION 1.0.0 LANGUAGES CXX)

# Set C++ standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Set output directories
# For Visual Studio generators, these are per-configuration
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

# For Visual Studio generators, also set per-configuration output directories
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG ${CMAKE_BINARY_DIR}/bin/Debug)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE ${CMAKE_BINARY_DIR}/bin/Release)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_DEBUG ${CMAKE_BINARY_DIR}/lib/Debug)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_RELEASE ${CMAKE_BINARY_DIR}/lib/Release)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_DEBUG ${CMAKE_BINARY_DIR}/lib/Debug)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_RELEASE ${CMAKE_BINARY_DIR}/lib/Release)

# Set MSVC runtime library (can be overridden by command line)
# Options: MultiThreaded, MultiThreadedDebug, MultiThreadedDLL, MultiThreadedDebugDLL
if(NOT DEFINED CMAKE_MSVC_RUNTIME_LIBRARY)
    if(CMAKE_BUILD_TYPE STREQUAL "Debug")
        set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreadedDebug" CACHE STRING "MSVC runtime library")
    else()
        set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded" CACHE STRING "MSVC runtime library")
    endif()
endif()
set_property(CACHE CMAKE_MSVC_RUNTIME_LIBRARY PROPERTY STRINGS 
    "MultiThreaded" "MultiThreadedDebug" "MultiThreadedDLL" "MultiThreadedDebugDLL")

# Get project root directory (now CMAKE_SOURCE_DIR is the project root)
get_filename_component(PROJECT_ROOT "${CMAKE_SOURCE_DIR}" ABSOLUTE)

# Include directories
set(INCLUDE_DIRS
    ${PROJECT_ROOT}/Inc
    ${PROJECT_ROOT}/Src
    ${PROJECT_ROOT}/ThirdParty
    ${PROJECT_ROOT}/ThirdParty/CYCoroutine
    ${PROJECT_ROOT}/ThirdParty/CYCoroutine/Inc
    ${PROJECT_ROOT}/ThirdParty/CYLogger
    ${PROJECT_ROOT}/ThirdParty/CYLogger/Inc
    ${PROJECT_ROOT}/ThirdParty/libyuv
    ${PROJECT_ROOT}/ThirdParty/libyuv/include
    ${PROJECT_ROOT}/ThirdParty/libsamplerate
)

# Source files
set(SOURCES
    ${PROJECT_ROOT}/Src/Capture/Win/WinDeviceCaptrue.cpp
    ${PROJECT_ROOT}/Src/Common/CYStringHelper.cpp
    ${PROJECT_ROOT}/Src/Common/Win/CaptureFilter/CaptureFilter.cpp
    ${PROJECT_ROOT}/Src/Control/CYDeviceControl.cpp
    ${PROJECT_ROOT}/Src/CYDeviceFactory.cpp
    ${PROJECT_ROOT}/Src/CYDeviceHelper.cpp
    ${PROJECT_ROOT}/Src/CYDeviceImpl.cpp
)

# Header files (for IDE)
set(HEADERS
    ${PROJECT_ROOT}/Inc/CYDevice/CYDeviceDefine.hpp
    ${PROJECT_ROOT}/Inc/CYDevice/CYDeviceFatory.hpp
    ${PROJECT_ROOT}/Inc/CYDevice/CYDeviceHelper.hpp
    ${PROJECT_ROOT}/Inc/CYDevice/ICYDevice.hpp
    ${PROJECT_ROOT}/Src/Capture/IDeviceCapture.hpp
    ${PROJECT_ROOT}/Src/Capture/Win/DShowCommonDefine.hpp
    ${PROJECT_ROOT}/Src/Capture/Win/ReSampleRateDefine.hpp
    ${PROJECT_ROOT}/Src/Capture/Win/WinDeviceCaptrue.hpp
    ${PROJECT_ROOT}/Src/Common/CYDevicePrivDefine.hpp
    ${PROJECT_ROOT}/Src/Common/CYStringHelper.hpp
    ${PROJECT_ROOT}/Src/Common/Win/CaptureFilter/CaptureFilter.h
    ${PROJECT_ROOT}/Src/Common/Win/IDeviceSource.h
    ${PROJECT_ROOT}/Src/Common/Win/IVideoCaptureFilter.h
    ${PROJECT_ROOT}/Src/Control/CYDeviceControl.hpp
    ${PROJECT_ROOT}/Src/CYDeviceImpl.hpp
)

# Create static library
add_library(CYDevice STATIC ${SOURCES} ${HEADERS})

# Set target properties
target_include_directories(CYDevice PUBLIC ${INCLUDE_DIRS})

# Preprocessor definitions
target_compile_definitions(CYDevice PRIVATE
    _CRT_SECURE_NO_WARNINGS
    DSHOWPLUGIN_EXPORTS
    CYDEVICE_USE_DLL
    CYDEVICE_EXPORTS
)

# Windows specific settings
if(WIN32)
    target_compile_definitions(CYDevice PRIVATE
        WIN32
        _WINDOWS
    )
    
    # Link Windows libraries
    target_link_libraries(CYDevice PRIVATE
        OBSApi
        strmiids
    )
    
    # Link libsamplerate (Debug version has 'd' suffix)
    if(CMAKE_BUILD_TYPE STREQUAL "Debug")
        target_link_libraries(CYDevice PRIVATE libsamplerated)
    else()
        target_link_libraries(CYDevice PRIVATE libsamplerate)
    endif()
    
    # Set Windows SDK version
    set(CMAKE_SYSTEM_VERSION 10.0)
endif()

# Compiler options
if(MSVC)
    target_compile_options(CYDevice PRIVATE
        /W3
        /sdl
        /permissive-
    )
else()
    target_compile_options(CYDevice PRIVATE
        -Wall
        -Wextra
        -Wpedantic
    )
endif()

# Debug configuration
target_compile_definitions(CYDevice PRIVATE
    $<$<CONFIG:Debug>:_DEBUG>
    $<$<CONFIG:Release>:NDEBUG>
)

# Set output name for Debug (add 'D' suffix)
set_target_properties(CYDevice PROPERTIES
    DEBUG_POSTFIX "D"
)

# Set MSVC runtime library for the target
if(MSVC)
    set_property(TARGET CYDevice PROPERTY MSVC_RUNTIME_LIBRARY ${CMAKE_MSVC_RUNTIME_LIBRARY})
endif()

# Install rules
install(TARGETS CYDevice
    ARCHIVE DESTINATION lib
    LIBRARY DESTINATION lib
    RUNTIME DESTINATION bin
)

install(DIRECTORY ${PROJECT_ROOT}/Inc/CYDevice
    DESTINATION include
    FILES_MATCHING PATTERN "*.hpp"
)

