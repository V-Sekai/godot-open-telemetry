# Justfile for Godot OpenTelemetry module build process

# Default target
default: build

# Build godot-cpp bindings
build-cpp:
    cd {{env_var('GODOT_CPP_PATH')}} && scons platform=linuxbsd generate_bindings=yes

# Build the OpenTelemetry GDExtension
build:
    mkdir -p build
    cmake -S . -B build -G Ninja -DPLATFORM=linuxbsd -DTARGET=editor -DARCH=x86_64 -DCMAKE_BUILD_TYPE=Release -DCMAKE_MESSAGE_LOG_LEVEL=VERBOSE
    cmake --build build --config Release --verbose

# Run the Godot editor (assumes GODOT_BINARY is set and the extension is loaded in a project)
run:
    {{env_var('GODOT_BINARY')}}

# Clean the build directory
clean:
    rm -rf build

# Rebuild: clean and build
rebuild: clean build

# Build only the OpenTelemetry third-party libraries
build-opentelemetry:
    cd thirdparty/opentelemetry-cpp && \
    mkdir -p build && \
    cd build && \
    cmake .. -DBUILD_SHARED_LIBS=OFF -DBUILD_TESTING=OFF -DWITH_OTLP=ON -DWITH_OTLP_GRPC=ON -DCMAKE_BUILD_TYPE=Release && \
    cmake --build . --config Release

# Full setup: build cpp, build opentelemetry, build extension
setup: build-cpp build-opentelemetry build
