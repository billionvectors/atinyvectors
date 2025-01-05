#!/bin/bash

# Default to "Release" build and "true" for no-cache
BUILD_TYPE="Release"
TAG="atinyvectors:release"
OUTPUT_DIR="./output/"  # Directory to store the extracted file
LIBRARY_PATH="/app/build/libatinyvectors.so"  # Path of the library in the container
NO_CACHE="true"
OPT_LEVEL="avx2"

rm -rf build/

mkdir -p $OUTPUT_DIR

# Parse arguments for build type, no-cache option, and optimization level
for arg in "$@"
do
    case $arg in
        debug)
        BUILD_TYPE="Debug"
        TAG="atinyvectors:debug"
        shift
        ;;
        --no-cache*)
        NO_CACHE="${arg#*=}"
        shift
        ;;
        --opt-level=*)
        OPT_LEVEL="${arg#*=}"
        shift
        ;;
        *)
        # unknown option
        ;;
    esac
done

echo "OPT_LEVEL is set to ${OPT_LEVEL}. Possible options are [generic, avx2, avx512]."

# Build the Docker image with or without --no-cache based on the NO_CACHE variable
if [ "$NO_CACHE" == "true" ]; then
    docker build --no-cache -t $TAG --build-arg BUILD_TYPE=$BUILD_TYPE --build-arg OPT_LEVEL=$OPT_LEVEL .
else
    docker build -t $TAG --build-arg BUILD_TYPE=$BUILD_TYPE --build-arg OPT_LEVEL=$OPT_LEVEL .
fi

# Create a container and extract the .so file
CONTAINER_ID=$(docker create $TAG)

# Copy the shared library from the container to the host machine
docker cp $CONTAINER_ID:$LIBRARY_PATH $OUTPUT_DIR

# Clean up the container
docker rm $CONTAINER_ID

# Print success message
echo "libatinyvectors.so has been copied to $OUTPUT_DIR"
