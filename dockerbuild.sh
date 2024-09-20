#!/bin/bash

# Default to "Release" build and "true" for no-cache
BUILD_TYPE="Release"
TAG="atinyvectors:release"
OUTPUT_DIR="./output/"  # Directory to store the extracted file
LIBRARY_PATH="/app/build/libatinyvectors.so"  # Path of the library in the container
NO_CACHE="true"

rm -rf build/

mkdir -p $OUTPUT_DIR

# Parse arguments for build type and no-cache option
for arg in "$@"
do
    case $arg in
        debug)
        BUILD_TYPE="Debug"
        TAG="atinyvectors:debug"
        shift
        ;;
        --no-cache=*)
        NO_CACHE="${arg#*=}"
        shift
        ;;
        *)
        # unknown option
        ;;
    esac
done

# Build the Docker image with or without --no-cache based on the NO_CACHE variable
if [ "$NO_CACHE" == "true" ]; then
    docker build --no-cache -t $TAG --build-arg BUILD_TYPE=$BUILD_TYPE .
else
    docker build -t $TAG --build-arg BUILD_TYPE=$BUILD_TYPE .
fi

# Create a container and extract the .so file
CONTAINER_ID=$(docker create $TAG)

# Copy the shared library from the container to the host machine
docker cp $CONTAINER_ID:$LIBRARY_PATH $OUTPUT_DIR

# Clean up the container
docker rm $CONTAINER_ID

# Print success message
echo "libatinyvectors.so has been copied to $OUTPUT_DIR"
