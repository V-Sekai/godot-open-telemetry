cmake_minimum_required(VERSION 3.20)
project(opentelemetry)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Godot-cpp setup
set(GODOT_CPP_DIR "${CMAKE_CURRENT_SOURCE_DIR}/thirdparty/godot-cpp")
set(GODOT_CPP_BUILD_DIR "${CMAKE_CURRENT_BINARY_DIR}/godot-cpp")
add_subdirectory("${GODOT_CPP_DIR}" "${GODOT_CPP_BUILD_DIR}" EXCLUDE_FROM_ALL)

include(FetchContent)

# Extension library
add_library(opentelemetry_gdextension SHARED
    open_telemetry.cpp
    register_types.cpp
    thirdparty/duckdb/duckdb.cpp
)

# Enable exceptions for files that need JSON parsing and DuckDB
set_source_files_properties(open_telemetry.cpp PROPERTIES COMPILE_FLAGS "-fexceptions")
set_source_files_properties(register_types.cpp PROPERTIES COMPILE_FLAGS "-fexceptions")
set_source_files_properties(thirdparty/duckdb/duckdb.cpp PROPERTIES COMPILE_FLAGS "-fexceptions")

target_include_directories(opentelemetry_gdextension PUBLIC
    ${GODOT_CPP_DIR}/include
    ${GODOT_CPP_DIR}/gen/include
    ${GODOT_CPP_DIR}/gdextension
    thirdparty/duckdb
)

target_link_libraries(opentelemetry_gdextension
    godot-cpp
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
