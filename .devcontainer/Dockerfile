FROM mcr.microsoft.com/devcontainers/cpp:ubuntu

# Install additional dependencies for OpenTelemetry C++ and Godot
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
        python3 \
        python3-pip \
        scons \
        libx11-dev \
        libxcursor-dev \
        libxinerama-dev \
        libgl1-mesa-dev \
        libglu1-mesa-dev \
        libasound2-dev \
        libpulse-dev \
        libudev-dev \
        libxi-dev \
        libxrandr-dev \
        yasm \
    && apt-get autoremove -y && apt-get clean -y && rm -rf /var/lib/apt/lists/*

# Install nlohmann-json
RUN apt-get update && apt-get install -y nlohmann-json3-dev

# Clone Godot 4.5
USER vscode
RUN git clone --depth 1 --branch 4.5 https://github.com/godotengine/godot.git /home/vscode/godot

# Set up Godot build environment
WORKDIR /home/vscode/godot
RUN python3 -m pip install --user scons
ENV PATH="/home/vscode/.local/bin:${PATH}"