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

# Build godot-cpp
echo "Building godot-cpp..."
cd /home/vscode/godot-cpp

if [ ! -d "bin" ]; then
    scons platform=linux generate_bindings=yes
else
    echo "godot-cpp already built, skipping build"
fi

cd /workspaces/${localWorkspaceFolderBasename}

echo "Development environment setup complete!"
echo "Godot binary: /home/vscode/godot/bin/godot.linuxbsd.editor.x86_64"
echo "godot-cpp library: /home/vscode/godot-cpp/bin/libgodot-cpp.linuxbsd.template_release.x86_64.a"
echo "You can now build and test the OpenTelemetry module"
