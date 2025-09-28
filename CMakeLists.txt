cmake_minimum_required(VERSION 3.20)
project(opentelemetry)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Godot-cpp setup
set(GODOT_CPP_DIR "${CMAKE_CURRENT_SOURCE_DIR}/thirdparty/godot-cpp")
set(GODOT_CPP_BUILD_DIR "${CMAKE_CURRENT_BINARY_DIR}/godot-cpp")
add_subdirectory("${GODOT_CPP_DIR}" "${GODOT_CPP_BUILD_DIR}" EXCLUDE_FROM_ALL)

# Abseil setup (vendored)
set(ABSL_CPP_DIR "${CMAKE_CURRENT_SOURCE_DIR}/thirdparty/abseil_vendor")
add_subdirectory(${ABSL_CPP_DIR} EXCLUDE_FROM_ALL)

# OpenTelemetry setup
set(OPENTELEMETRY_CPP_DIR "${CMAKE_CURRENT_SOURCE_DIR}/thirdparty/opentelemetry-cpp")
set(OPENTELEMETRY_CPP_BUILD_DIR "${CMAKE_BINARY_DIR}/opentelemetry-cpp")

# Force PIC globally for OpenTelemetry since we link static libs into shared lib
set(OTEL_PIC_ORIGINAL_STATE ${CMAKE_POSITION_INDEPENDENT_CODE})
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# Set OpenTelemetry build options
set(WITH_OTLP ON CACHE BOOL "Build OTLP exporter")
set(WITH_OTLP_GRPC ON CACHE BOOL "Build OTLP gRPC exporter")
set(WITH_OTLP_HTTP ON CACHE BOOL "Build OTLP HTTP exporter")
set(BUILD_SHARED_LIBS OFF CACHE BOOL "Build static libs")
set(BUILD_TESTING OFF CACHE BOOL "Build tests")

# Fix for ARM64 builds - disable AES hardware acceleration which uses SSE4.1
# This prevents build failures on Apple Silicon and ARM64 platforms
if(CMAKE_SYSTEM_PROCESSOR MATCHES "arm64|aarch64|ARM64")
    message(STATUS "ARM64 detected, disabling AES hardware acceleration")
    set(ABSL_RANDOM_DISABLE_AES_HARDWARE_ACCELERATION ON CACHE BOOL "" FORCE)
    set(ABSL_RANDOM_RANDEN_USE_HWAES OFF CACHE BOOL "" FORCE)
endif()

add_subdirectory("${OPENTELEMETRY_CPP_DIR}" "${OPENTELEMETRY_CPP_BUILD_DIR}" EXCLUDE_FROM_ALL)

# Restore original PIC state
set(CMAKE_POSITION_INDEPENDENT_CODE ${OTEL_PIC_ORIGINAL_STATE})

# Extension library
add_library(opentelemetry_gdextension SHARED
    opentelemetry_wrapper.cpp
    open_telemetry.cpp
    register_types.cpp
)

# Enable exceptions for files that need JSON parsing
set_source_files_properties(opentelemetry_wrapper.cpp PROPERTIES COMPILE_FLAGS "-fexceptions")

target_include_directories(opentelemetry_gdextension PUBLIC
    ${GODOT_CPP_DIR}/include
    ${GODOT_CPP_DIR}/gen/include
    ${GODOT_CPP_DIR}/gdextension
    ${OPENTELEMETRY_CPP_DIR}/api/include
    ${OPENTELEMETRY_CPP_DIR}/sdk/include
)

target_link_directories(opentelemetry_gdextension PUBLIC
    ${OPENTELEMETRY_CPP_BUILD_DIR}/lib
)

target_link_libraries(opentelemetry_gdextension
    godot-cpp
    opentelemetry_api
    opentelemetry_sdk
    opentelemetry_exporter_ostream_span
    opentelemetry_exporter_otlp_grpc
    opentelemetry_exporter_otlp_http
    opentelemetry_resources
)

# Set properties
set_target_properties(opentelemetry_gdextension PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
    PREFIX ""
)

# Set platform-specific output names
if (PLATFORM STREQUAL "linuxbsd")
    if (ARCH STREQUAL "x86_64")
        set_target_properties(opentelemetry_gdextension PROPERTIES
            OUTPUT_NAME "opentelemetry.linux.x86_64"
        )
    else()
        set_target_properties(opentelemetry_gdextension PROPERTIES
            OUTPUT_NAME "opentelemetry.linux.arm64"
        )
    endif()
else()
    # Default to macos arm64 for now
    set_target_properties(opentelemetry_gdextension PROPERTIES
        OUTPUT_NAME "opentelemetry.macos.arm64"
    )
endif()
