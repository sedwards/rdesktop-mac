#!/bin/sh
set -e
echo "Generating build files using autoconf..."
autoheader
autoconf
echo "Done. Now run './configure' and 'make'."
