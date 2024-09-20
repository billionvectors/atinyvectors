# Use an official image with cmake and g++ preinstalled
FROM ubuntu:20.04

# Set environment variables to prevent interactive prompt during package installation
ENV DEBIAN_FRONTEND=noninteractive

# Install dependencies
RUN apt-get update && apt-get install -y \
    build-essential \
    cmake \
    git \
    wget \
    curl \
    xz-utils \
    libsqlite3-dev \
    clang \
    zlib1g-dev \
    libbz2-dev \
    liblzma-dev \
    pkg-config \
    libzstd-dev \
    libssl-dev

# Set default build type to Debug if not specified
ARG BUILD_TYPE=Debug

# Create a working directory
WORKDIR /app

# Copy the project files to the working directory
COPY . .

# Build project using CMake
RUN mkdir -p build && cd build \
    && cmake -DCMAKE_C_COMPILER=clang -DCMAKE_CXX_COMPILER=clang++ -DCMAKE_BUILD_TYPE=${BUILD_TYPE} .. \
    && make -j$(nproc)