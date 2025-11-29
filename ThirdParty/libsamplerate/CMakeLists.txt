cmake_minimum_required(VERSION 3.15)

# Set MSVC runtime library before project() if provided
if(MSVC AND DEFINED CMAKE_MSVC_RUNTIME_LIBRARY)
    set(CMAKE_MSVC_RUNTIME_LIBRARY ${CMAKE_MSVC_RUNTIME_LIBRARY} CACHE STRING "MSVC runtime library")
endif()

project(libsamplerate VERSION 0.2.2 LANGUAGES C)

# Set C standard
set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)

# Source files
set(LIBSAMPLERATE_SOURCES
    samplerate.c
    src_linear.c
    src_sinc.c
    src_zoh.c
)

# Header files (for IDE)
set(LIBSAMPLERATE_HEADERS
    samplerate.h
    common.h
    config.h
    float_cast.h
    fastest_coeffs.h
    high_qual_coeffs.h
    mid_qual_coeffs.h
)

# Create static library
add_library(libsamplerate STATIC
    ${LIBSAMPLERATE_SOURCES}
    ${LIBSAMPLERATE_HEADERS}
)

# Include directories
target_include_directories(libsamplerate
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
        $<INSTALL_INTERFACE:include>
)

# Compiler options
if(MSVC)
    target_compile_options(libsamplerate PRIVATE
        /W3
        /wd4996  # Disable deprecated function warnings
    )
    # MSVC has lrint and lrintf as intrinsic functions, so define HAVE_LRINT and HAVE_LRINTF
    # to use the built-in implementations instead of custom ones
    target_compile_definitions(libsamplerate PRIVATE
        HAVE_LRINT=1
        HAVE_LRINTF=1
    )
else()
    target_compile_options(libsamplerate PRIVATE
        -Wall
        -Wextra
    )
endif()

# Debug configuration - add 'd' suffix
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    set_target_properties(libsamplerate PROPERTIES
        DEBUG_POSTFIX "d"
    )
endif()

# MSVC runtime library setting
# The CMAKE_MSVC_RUNTIME_LIBRARY should be set before project() command
# If not set, default to MultiThreadedDLL
if(MSVC)
    if(NOT DEFINED CMAKE_MSVC_RUNTIME_LIBRARY)
        set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreadedDLL" CACHE STRING "MSVC runtime library")
    endif()
    
    # Apply to the target (this will use the value set above)
    set_property(TARGET libsamplerate PROPERTY 
        MSVC_RUNTIME_LIBRARY ${CMAKE_MSVC_RUNTIME_LIBRARY}
    )
endif()

# Install rules
install(TARGETS libsamplerate
    ARCHIVE DESTINATION lib
    LIBRARY DESTINATION lib
    RUNTIME DESTINATION bin
)

install(FILES samplerate.h
    DESTINATION include
)

# Export for use in parent projects
set_target_properties(libsamplerate PROPERTIES
    VERSION ${PROJECT_VERSION}
    SOVERSION ${PROJECT_VERSION_MAJOR}
)
