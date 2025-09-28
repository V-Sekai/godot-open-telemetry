#!/bin/bash

echo "Building OpenTelemetry C++ libraries..."

cd thirdparty/opentelemetry-cpp

if [ ! -d "build" ]; then
    mkdir build
    cd build
    cmake .. -DBUILD_SHARED_LIBS=OFF -DBUILD_TESTING=OFF -DWITH_OTLP=ON -DWITH_OTLP_GRPC=ON -DCMAKE_BUILD_TYPE=Release
    cmake --build . --config Release
else
    echo "Build directory already exists, skipping build"
fi

echo "OpenTelemetry C++ libraries built successfully!"