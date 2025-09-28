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
        just \
        ninja-build \
        yasm \
    && apt-get autoremove -y && apt-get clean -y && rm -rf /var/lib/apt/lists/*

# Install nlohmann-json
RUN apt-get update && apt-get install -y nlohmann-json3-dev

# Install git-subrepo
RUN git clone https://github.com/ingydotnet/git-subrepo.git ~/git-subrepo && \
    cd ~/git-subrepo && \
    make install && \
    cd ~ && \
    rm -rf ~/git-subrepo

# Clone Godot 4.5
USER vscode

# Clone godot-cpp 4.5 for GDExtension development
RUN git clone --depth 1 --branch 4.5 https://github.com/godotengine/godot-cpp.git /home/vscode/godot-cpp

# Set up godot-cpp build environment
WORKDIR /home/vscode/godot-cpp
