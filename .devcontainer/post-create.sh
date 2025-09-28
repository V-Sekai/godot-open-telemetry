#!/bin/bash

echo "Setting up Godot OpenTelemetry development environment..."

# Build OpenTelemetry C++ libraries
echo "Building OpenTelemetry C++ libraries..."
cd thirdparty/opentelemetry-cpp

if [ ! -d "build" ]; then
    mkdir build
    cd build
    cmake .. -DBUILD_SHARED_LIBS=OFF -DBUILD_TESTING=OFF -DWITH_OTLP=ON -DWITH_OTLP_GRPC=ON -DCMAKE_BUILD_TYPE=Release
    cmake --build . --config Release
else
    echo "OpenTelemetry build directory already exists, skipping build"
fi

cd /workspaces/${localWorkspaceFolderBasename}

# Set up module in Godot
echo "Setting up module in Godot..."
GODOT_MODULES_DIR="/home/vscode/godot/modules"
MODULE_NAME="opentelemetry"

if [ ! -d "$GODOT_MODULES_DIR/$MODULE_NAME" ]; then
    cp -r . "$GODOT_MODULES_DIR/$MODULE_NAME"
    echo "Module copied to Godot"
else
    echo "Module already exists in Godot, updating..."
    rsync -av --exclude='.git' --exclude='thirdparty/opentelemetry-cpp/build' . "$GODOT_MODULES_DIR/$MODULE_NAME/"
fi

# Build Godot with the module
echo "Building Godot with OpenTelemetry module..."
cd /home/vscode/godot

if [ ! -f "bin/godot.linuxbsd.editor.x86_64" ]; then
    scons platform=linuxbsd target=editor -j$(nproc) module_opentelemetry_enabled=yes
else
    echo "Godot already built, skipping build"
fi

echo "Development environment setup complete!"
echo "Godot binary: /home/vscode/godot/bin/godot.linuxbsd.editor.x86_64"
echo "You can now build and test the OpenTelemetry module"