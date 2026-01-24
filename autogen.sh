#!/bin/bash

# autogen.sh - Automated configuration and build script for rdesktop on macOS
# This script creates a minimal build configuration linking to native macOS libraries

set -e

echo "Configuring rdesktop for macOS..."

# Clean previous configurations
rm -rf autom4te.cache
rm -f config.h

# Generate autotools files (based on bootstrap)
echo "Generating autotools files..."
autoupdate -f
aclocal --force -I m4
autoheader
autoreconf -i
glibtoolize

# Configure with minimal options for macOS
echo "Running configure with macOS-optimized settings..."
./configure \
    --disable-credssp \
    --disable-smartcard \
    --with-sound=no \
    --without-x \
    CC=clang \
    CFLAGS="-O2 -Wall -Wextra -mmacosx-version-min=10.15" \
    LDFLAGS="-mmacosx-version-min=10.15"

# Build the project
echo "Building rdesktop..."
make clean
make -j$(sysctl -n hw.ncpu)

echo "Build complete. Binary available at ./rdesktop"