FROM mcr.microsoft.com/devcontainers/cpp:ubuntu

# Install additional dependencies for OpenTelemetry C++
RUN apt-get update && export DEBIAN_FRONTEND=noninteractive \
    && apt-get -y install --no-install-recommends \
        cmake \
        build-essential \
        pkg-config \
        libprotobuf-dev \
        protobuf-compiler \
        libgrpc++-dev \
        protobuf-compiler-grpc \
        git \
    && apt-get autoremove -y && apt-get clean -y && rm -rf /var/lib/apt/lists/*

# Install nlohmann-json
RUN apt-get update && apt-get install -y nlohmann-json3-dev

USER vscode