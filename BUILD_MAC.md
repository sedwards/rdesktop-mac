# Building rdesktop-mac Natively on macOS

This document provides step-by-step instructions for building the native macOS port of `rdesktop` (`rdesktop-mac`).

---

## TLS Engine Options

The macOS port supports two separate security/TLS engines:

### A. OpenSSL 3.x Engine (Recommended)
* **Description**: Links against Homebrew's modern OpenSSL library.
* **Pros**: Outstanding connection stability, robust TLS state management, standard compliance, and highly verbose diagnostics.
* **Requirements**: OpenSSL version 3.

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
* **Bundled Cryptography**: Compile and embed static libraries of **OpenSSL** inside the application bundle (e.g. under `rdesktop-mac.app/Contents/Frameworks/`).
* **Zero Host Prerequisites**: This will remove the requirement for end users to have Homebrew or `/usr/local/opt/openssl@3` installed, enabling a simple drag-and-drop installation.

## Errata
Intel port may have to be reverted for m4 rebuild

  ### The Root Cause & Color Space Correction

  Why it was still off: The pixel color bytes coming from RDP are packed in  BGRA  /  BGRX  order in memory ( Blue, Green, Red, Alpha/Skip ).
  
  The BitmapInfo Mismatch:
      • kCGImageAlphaNoneSkipLast  in big-endian is  RGBA  ( Red (0), Green (1), Blue (2), Skip (3) ).
      • However, when we applied  kCGBitmapByteOrder32Little  to it, the byte order was fully reversed by CoreGraphics to  ABGR  /  XBGR  ( Skip (0), Blue (1), Green (2), Red
      (3) ).
      
      • This meant Byte 0 (where we write  Blue ) was treated as the  Skip  byte (discarded!), while Byte 3 (where we write  0  for the unused byte) was treated as  Red 
      (rendering Red as  0 ).
      • As a result, the red channel was zeroed out, causing whites to appear as a light blue-green (cyan) tint and making the entire screen look dark and washed out.
      
  The Solution:
      Changed the bitmap info configuration in macwin.m (across the backing store context, image creation, and bitmap creation functions) to  kCGImageAlphaNoneSkipFirst |
      kCGBitmapByteOrder32Little .
      Under little-endian byte ordering,  NoneSkipFirst  reverses  Skip (0), Red (1), Green (2), Blue (3)  into  Blue (0), Green (1), Red (2), Skip (3) , which is the exact
      BGRX layout of our RDP pixel stream.
      
      This correctly aligns the Red, Green, and Blue color channels. Whites are now solid white and the screen brightness is fully restored.