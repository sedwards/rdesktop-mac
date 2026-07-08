#!/bin/bash
set -e

# build_mac.sh
# Automates the clean build process of dependencies and the rdesktop-mac bundle.

WORKSPACE_DIR="$(pwd)"
OPENSSL_SRC_DIR="${WORKSPACE_DIR}/deps/openssl-3.0.15"
OPENSSL_BUILD_DIR="${WORKSPACE_DIR}/deps/openssl_build"
NLA2_DIR="${WORKSPACE_DIR}/deps/nla2"

echo "=== starting rdesktop-mac build process ==="

# 1. Build local OpenSSL 3.x if not already present
if [ ! -f "${OPENSSL_BUILD_DIR}/lib/libssl.a" ] || [ ! -f "${OPENSSL_BUILD_DIR}/lib/libcrypto.a" ]; then
    echo "=> configuring and building static openssl 3.0.15..."
    cd "${OPENSSL_SRC_DIR}"
    
    # Configure for static libraries without shared libraries or tests
    ./Configure no-shared no-tests "--prefix=${OPENSSL_BUILD_DIR}"
    
    echo "=> compiling openssl (this may take a few minutes)..."
    make clean
    make -j$(sysctl -n hw.ncpu)
    make install_sw
    
    cd "${WORKSPACE_DIR}"
    echo "=> openssl build successful."
else
    echo "=> static openssl already built in deps/openssl_build. Skipping."
fi

# 2. Run configure if Makefile doesn't exist
if [ ! -f "${WORKSPACE_DIR}/Makefile" ]; then
    echo "=> running configure..."
    ./configure
fi

# 3. Clean workspace to ensure no dirty state
echo "=> cleaning workspace..."
make clean

# 4. Compile and assemble the app bundle
echo "=> building rdesktop-mac.app..."
make bundle

echo "=== build process completed successfully! ==="
echo "You can run the client using: open rdesktop-mac.app"
