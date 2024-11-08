# Use an official Ubuntu 20.04 image
FROM ubuntu:20.04

# Set environment variables to prevent interactive prompts during package installation
ENV DEBIAN_FRONTEND=noninteractive

# Install dependencies and prerequisites for adding Kitware repository
RUN apt-get update && apt-get install -y \
    build-essential \
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
    libssl-dev \
    libomp-dev \
    cargo \
    apt-transport-https \
    ca-certificates \
    gnupg \
    software-properties-common && \
    # Add Kitware APT repository for the latest CMake
    wget -O - https://apt.kitware.com/keys/kitware-archive-latest.asc | gpg --dearmor -o /usr/share/keyrings/kitware-archive-keyring.gpg && \
    echo "deb [signed-by=/usr/share/keyrings/kitware-archive-keyring.gpg] https://apt.kitware.com/ubuntu/ focal main" | tee /etc/apt/sources.list.d/kitware.list >/dev/null && \
    apt-get update && \
    apt-get install -y cmake && \
    # Clean up to reduce image size
    rm -rf /var/lib/apt/lists/*

# Set default build type to Release if not specified
ARG BUILD_TYPE=Release

# Create a working directory
WORKDIR /app

# Copy the project files to the working directory
COPY . .

# Build project using CMake
RUN mkdir -p build && cd build \
    && cmake -DCMAKE_C_COMPILER=clang -DCMAKE_CXX_COMPILER=clang++ -DCMAKE_BUILD_TYPE=${BUILD_TYPE} .. \
    && make -j$(nproc)
