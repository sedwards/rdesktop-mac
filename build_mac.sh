#!/bin/bash
set -e

# build_mac.sh
# Automates the clean build process of rdesktop-mac bundle using the nested OpenSSL.

WORKSPACE_DIR="$(pwd)"

echo "=== starting rdesktop-mac build process ==="

# 1. Run configure if Makefile doesn't exist
if [ ! -f "${WORKSPACE_DIR}/Makefile" ]; then
    echo "=> running configure..."
    ./configure
fi

# 2. Clean workspace to ensure no dirty state
echo "=> cleaning workspace..."
make clean

# 3. Compile and assemble the app bundle
# Note: The root Makefile will automatically configure and build the nested OpenSSL
# inside deps/nla2/deps/openssl-3.0.15/ if it hasn't been built yet.
echo "=> building rdesktop-mac.app (includes compiling nested OpenSSL)..."
make bundle

echo "=== build process completed successfully! ==="
echo "You can run the client using: open rdesktop-mac.app"
