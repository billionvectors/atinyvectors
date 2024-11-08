#!/bin/bash

# Define the installation directory for dependencies
INSTALL_DIR="$(pwd)/dependency"  # Set to the 'lib' folder in the current directory
FAISS_VERSION="v1.9.0"  # Specify the desired version of Faiss (tag)

# Check if Faiss is already installed
if pkg-config --exists faiss; then
    echo "Faiss is already installed."
    exit 0
fi

# Install dependencies
echo "Faiss not found. Installing dependencies..."

# Clone Faiss if not already cloned
if [ ! -d "faiss" ]; then
    git clone --depth 1 --branch $FAISS_VERSION https://github.com/facebookresearch/faiss.git
    cd faiss
    git checkout -b temp-branch  # Create a temporary branch to avoid detached HEAD
else
    echo "Faiss directory already exists. Using existing repository."
    cd faiss
fi

# Create a build directory in the correct location
mkdir -p build
cd build

# Configure, build, and install Faiss
cmake .. -DCMAKE_BUILD_TYPE=Release \
      -DCMAKE_INSTALL_PREFIX=$INSTALL_DIR \
      -DFAISS_ENABLE_GPU=OFF \
      -DFAISS_ENABLE_RAFT=OFF \
      -DFAISS_ENABLE_PYTHON=OFF \
      -DFAISS_ENABLE_C_API=OFF \
      -DFAISS_USE_LTO=OFF

# Build and install
make -j$(nproc)
make install