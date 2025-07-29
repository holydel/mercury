#!/bin/bash

# Create build directory
mkdir -p build-linux
cd build-linux

# Configure and build
cmake ..
make

echo "Build complete! Executable is in build-linux/bin/testbed" 