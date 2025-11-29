cmake_minimum_required(VERSION 3.15)
project(CYDeviceTest VERSION 1.0.0 LANGUAGES CXX RC)

# Set C++ standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Get project root directory (three levels up from Samples/CYDeviceTest)
get_filename_component(PROJECT_ROOT "${CMAKE_SOURCE_DIR}/../.." ABSOLUTE)

# Include directories
set(INCLUDE_DIRS
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${PROJECT_ROOT}/Inc
    ${PROJECT_ROOT}/ThirdParty/libyuv/include
)

# Source files
set(SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/CYDeviceTest.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/CYDeviceTestDlg.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/CVideoRender.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Render/GDIPlusRender.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/pch.cpp
)

# Resource files
set(RESOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/CYDeviceTest.rc
)

# Header files (for IDE)
set(HEADERS
    ${CMAKE_CURRENT_SOURCE_DIR}/CYDeviceTest.h
    ${CMAKE_CURRENT_SOURCE_DIR}/CYDeviceTestDlg.h
    ${CMAKE_CURRENT_SOURCE_DIR}/CVideoRender.h
    ${CMAKE_CURRENT_SOURCE_DIR}/Render/GDIPlusRender.h
    ${CMAKE_CURRENT_SOURCE_DIR}/Render/MemDC.h
    ${CMAKE_CURRENT_SOURCE_DIR}/pch.h
    ${CMAKE_CURRENT_SOURCE_DIR}/framework.h
    ${CMAKE_CURRENT_SOURCE_DIR}/targetver.h
    ${CMAKE_CURRENT_SOURCE_DIR}/resource.h
)

# Create executable
add_executable(CYDeviceTest WIN32 ${SOURCES} ${HEADERS} ${RESOURCES})

# Set target properties
target_include_directories(CYDeviceTest PRIVATE ${INCLUDE_DIRS})

# Preprocessor definitions
target_compile_definitions(CYDeviceTest PRIVATE
    _WINDOWS
    $<$<CONFIG:Debug>:_DEBUG>
    $<$<CONFIG:Release>:NDEBUG>
)

# Windows specific settings
if(WIN32)
    target_compile_definitions(CYDeviceTest PRIVATE
        WIN32
    )
    
    # Link Windows libraries
    target_link_libraries(CYDeviceTest PRIVATE
        strmiids
        ole32
        oleaut32
        gdiplus
    )
    
    # Link CYDevice library
    if(CMAKE_BUILD_TYPE STREQUAL "Debug")
        target_link_libraries(CYDeviceTest PRIVATE
            CYDeviceD
        )
    else()
        target_link_libraries(CYDeviceTest PRIVATE
            CYDevice
        )
    endif()
    
    # Set Windows SDK version
    set(CMAKE_SYSTEM_VERSION 10.0)
    
    # Use MFC statically
    set(CMAKE_MFC_FLAG 2)  # Static MFC
endif()

# Compiler options
if(MSVC)
    target_compile_options(CYDeviceTest PRIVATE
        /W3
        /sdl
        /permissive-
    )
    
    # Set MSVC runtime library for the target
    if(DEFINED CMAKE_MSVC_RUNTIME_LIBRARY)
        set_property(TARGET CYDeviceTest PROPERTY MSVC_RUNTIME_LIBRARY ${CMAKE_MSVC_RUNTIME_LIBRARY})
    endif()
else()
    target_compile_options(CYDeviceTest PRIVATE
        -Wall
        -Wextra
    )
endif()

# Set output directory to match library structure
set_target_properties(CYDeviceTest PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY_DEBUG ${CMAKE_BINARY_DIR}/bin/Debug
    RUNTIME_OUTPUT_DIRECTORY_RELEASE ${CMAKE_BINARY_DIR}/bin/Release
)

# Set library search path - use CYDevice_DIR if provided, otherwise construct path
if(DEFINED CYDevice_DIR)
    get_filename_component(LIB_DIR "${CYDevice_DIR}" ABSOLUTE)
    target_link_directories(CYDeviceTest PRIVATE ${LIB_DIR})
else()
    # Try to find library in standard location
    if(CMAKE_BUILD_TYPE STREQUAL "Debug")
        set(LIB_DIR "${PROJECT_ROOT}/Bin/Windows/${CMAKE_SYSTEM_PROCESSOR}/${CMAKE_MSVC_RUNTIME_LIBRARY}/Debug")
    else()
        set(LIB_DIR "${PROJECT_ROOT}/Bin/Windows/${CMAKE_SYSTEM_PROCESSOR}/${CMAKE_MSVC_RUNTIME_LIBRARY}/Release")
    endif()
    target_link_directories(CYDeviceTest PRIVATE ${LIB_DIR})
endif()

