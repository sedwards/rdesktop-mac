# Building rdesktop-mac Natively on macOS

This document provides step-by-step instructions for building the native macOS port of `rdesktop` (`rdesktop-mac`) without relying on Homebrew or third-party package managers.

---

## Architecture Overview

Normally, `rdesktop` depends on several external libraries for cryptography, ASN.1 parsing, and TLS connection handling:
- **GnuTLS** (Secure communication)
- **libtasn1** (ASN.1 structure parsing)
- **nettle** / **hogweed** (Cryptographic primitives)
- **GMP** (GNU Multiple Precision Arithmetic Library)

### The Native macOS Approach
To achieve a lightweight, native, and dependency-free build on macOS, this port implements a custom security bridge:
1. **Secure Transport (`Security.framework`)**: In [tcp.c](file:///Users/sedwards/source/rdesktop-mac/tcp.c), when building for macOS (`__APPLE__`), the client utilizes macOS's native **Secure Transport** APIs (`SSLHandshake`, `SSLRead`, `SSLWrite`) rather than OpenSSL or GnuTLS.
2. **Compatibility Stubs (`macssl.c` & `macssl.h`)**: The files [macssl.c](file:///Users/sedwards/source/rdesktop-mac/macssl.c) and [macssl.h](file:///Users/sedwards/source/rdesktop-mac/macssl.h) stub out all GnuTLS and ASN.1 functions (e.g., `gnutls_init`, `asn1_der_decoding`). Since the client uses Secure Transport, it does not actually call these functions, but they must exist for the project to compile and link.
3. **No External Libraries**: Because of the Secure Transport native implementation and the compatibility stubs, the compiled binary does **not** link against GnuTLS, libtasn1, nettle, hogweed, or GMP.

---

## Patches Applied for Dependency-Free Build

To avoid requiring these third-party libraries during the configuration stage:
1. **`configure.ac`**: Patched to bypass package checks (`PKG_CHECK_MODULES`) and search checks (`AC_SEARCH_LIBS`) for GMP, libtasn1, nettle, hogweed, and GnuTLS when compiling on macOS (darwin).
2. **`Makefile.in`**: Removed hardcoded path references to `/opt/homebrew` (which caused builds to fail on Intel Macs) and removed `-lgmp` linkage since the native port does not use it.

---

## Native macOS Menubar & Settings Menu

A native macOS menubar is constructed programmatically during launch when running in GUI mode:
- **Application Menu**: Contains standard "About rdesktop-mac", "Settings..." (mapped to `Cmd+,`), and standard Hide/Quit items.
- **Connection Menu**:
  - **Connect...** (`Cmd+N`): Opens the connection setup window.
  - **Disconnect** (`Cmd+D`): Disconnects an active remote desktop connection (dynamically enabled only when a session is active).
- **Settings Menu**:
  - **Show All Settings**: Opens the main configuration dialog and expands all advanced options.
  - **Quick Toggles**: Provides direct checkboxes in the menu bar to toggle settings:
    - *Full-screen Mode*
    - *Enable RDP Compression*
    - *Persistent Bitmap Caching*
    - *Use Local Mouse Cursor*
    - *Attach to Console*
    - *Verbose Logging*

The menubar automatically synchronizes item checkmarks dynamically with the GUI connection window options via `validateMenuItem:`.

---

## Build Prerequisites

To compile the native client, you only need the standard Apple build tools:
- **Xcode Command Line Tools** (which provide `clang`, `make`, `autoconf`, `automake`, `libtool`, etc.)
  To install them, run:
  ```bash
  xcode-select --install
  ```

---

## Compilation Instructions

The project provides a helper script [autogen.sh](file:///Users/sedwards/source/rdesktop-mac/autogen.sh) that bootstraps the configuration files, configures the build system with optimized settings for macOS, and compiles the binary.

### 1. Automatic Build (Recommended)
Simply run the bootstrap and build script in the terminal:
```bash
./autogen.sh
```

Upon success, you will find the compiled native binary `rdesktop-mac` in the root directory:
```bash
./rdesktop-mac <server_ip_or_hostname>
```

---

### 2. Manual Build (Under the Hood)
If you prefer to run the build steps manually, execute the following commands in the workspace root directory:

#### Step A: Bootstrap the Build System
Generate the `configure` script and header templates from `configure.ac`:
```bash
rm -rf autom4te.cache
rm -f config.h
autoupdate -f
aclocal --force -I m4
autoheader
autoreconf -i
glibtoolize
```

#### Step B: Run Configure
Configure the project with macOS-native optimizations and disable Linux-specific subsystems:
```bash
./configure \
    --disable-credssp \
    --disable-smartcard \
    --with-sound=no \
    --without-x \
    CC=clang \
    CFLAGS="-O2 -Wall -Wextra -mmacosx-version-min=10.15" \
    LDFLAGS="-mmacosx-version-min=10.15"
```

#### Step C: Build the Project
Compile the code using all available CPU cores:
```bash
make clean
make -j$(sysctl -n hw.ncpu)
```

The output binary `rdesktop-mac` will be generated in the root folder.
