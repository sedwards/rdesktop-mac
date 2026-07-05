# Building rdesktop-mac Natively on macOS

This document provides step-by-step instructions for building the native macOS port of `rdesktop` (`rdesktop-mac`).

---

## TLS Engine Options

The macOS port supports two separate security/TLS engines:

### A. OpenSSL 3.x Engine (Recommended)
* **Description**: Links against Homebrew's modern OpenSSL library.
* **Pros**: Outstanding connection stability, robust TLS state management, standard compliance, and highly verbose diagnostics.
* **Requirements**: Homebrew OpenSSL.

### B. Secure Transport Engine (Dependency-Free)
* **Description**: Uses Apple's native `Security.framework` (`SSLHandshake`, `SSLRead`, `SSLWrite`) directly.
* **Pros**: Light footprint, requires zero external libraries or package managers.

---

## Build Prerequisites

1. **Xcode Command Line Tools** (Required for both methods):
   Install them by running:
   ```bash
   xcode-select --install
   ```

2. **Homebrew OpenSSL** (Required only for the **OpenSSL** method):
   If you wish to use the recommended OpenSSL build, install it via Homebrew:
   ```bash
   brew install openssl@3
   ```

---

## Compilation Instructions

The build system utilizes a standard `Makefile`. You can build for either engine:

### 1. Compile with OpenSSL (Recommended)

To compile using the modern OpenSSL 3.x engine (uses `-DUSE_OPENSSL` and links against Homebrew's OpenSSL paths):

Configure the `Makefile` CFLAGS and LDFLAGS (this is configured in the default `Makefile` of this workspace):
```makefile
CFLAGS  += -DUSE_OPENSSL -I/usr/local/opt/openssl@3/include
LDFLAGS += -L/usr/local/opt/openssl@3/lib -lssl -lcrypto
```

Clean and build:
```bash
make clean
make
```

---

### 2. Compile with Native Secure Transport (Dependency-Free)

To compile without Homebrew dependencies using macOS native Secure Transport, remove `-DUSE_OPENSSL`, the OpenSSL include paths, and `-lssl -lcrypto` links:

```makefile
CFLAGS  = -O2 -Wall -Wextra -mmacosx-version-min=10.15 -DHAVE_CONFIG_H -DKEYMAP_PATH=\"$(KEYMAP_PATH)\"
LDFLAGS = -mmacosx-version-min=10.15 -framework Kerberos -liconv -framework Cocoa -framework CoreGraphics -framework Carbon -framework Security
```

Clean and build:
```bash
make clean
make
```

---

## Running the Client

Upon successful compilation, the output binary `rdesktop-mac` will be generated in the root directory:

```bash
./rdesktop-mac -u <username> -p <password> <host_ip>:3389
```

---

## Future Packaging & Bundling Plans

To deliver `rdesktop-mac` as a self-contained, standard macOS `.app` bundle, the project aims to remove the runtime dependency on Homebrew's OpenSSL. 

### Planned Migration:
* **Bundled Cryptography**: Compile and embed static libraries of **OpenSSL** and/or **GnuTLS** inside the application bundle (e.g. under `rdesktop-mac.app/Contents/Frameworks/`).
* **Zero Host Prerequisites**: This will remove the requirement for end users to have Homebrew or `/usr/local/opt/openssl@3` installed, enabling a simple drag-and-drop installation.
