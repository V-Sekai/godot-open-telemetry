# Justfile for Godot OpenTelemetry module build process

# Default target
default: build

# Build Godot with the OpenTelemetry module enabled
build:
    cd {{env_var('GODOT_PATH')}} && scons platform=linuxbsd target=editor -j$(nproc) module_opentelemetry_enabled=yes

# Run the built Godot editor
run:
    {{env_var('GODOT_BINARY')}}

# Clean the Godot build
clean:
    cd {{env_var('GODOT_PATH')}} && scons -c

# Rebuild: clean and build
rebuild: clean build

# Build only the OpenTelemetry third-party libraries
build-opentelemetry:
    cd thirdparty/opentelemetry-cpp && \
    mkdir -p build && \
    cd build && \
    cmake .. -DBUILD_SHARED_LIBS=OFF -DBUILD_TESTING=OFF -DWITH_OTLP=ON -DWITH_OTLP_GRPC=ON -DCMAKE_BUILD_TYPE=Release && \
    cmake --build . --config Release

# Sync the module to Godot modules directory
sync-module:
    rsync -av --exclude='.git' --exclude='thirdparty/opentelemetry-cpp/build' . {{env_var('GODOT_PATH')}}/modules/opentelemetry/

# Full setup: sync module, build opentelemetry, build godot
setup: sync-module build-opentelemetry build