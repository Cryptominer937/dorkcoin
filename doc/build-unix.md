UNIX BUILD NOTES
====================

This document describes the CMake + depends build workflow for Dork Core (Dorkcoin) on Unix-like systems. The repository has moved to the Bitcoin Core 28 CMake layout; the legacy autotools (./autogen.sh && ./configure) flow is deprecated.

Prerequisites
-------------
- A C++ toolchain (gcc/clang, make or ninja)
- cmake (>= 3.16 recommended)
- python3
- build utilities (make, pkg-config, etc.)

See doc/dependencies.md and depends/README.md for details about specific library requirements and how to build them with the depends system.

Overview
--------
The recommended workflow is:

1. (Optional) Build the "depends" tree for reproducible and hermetic dependency builds (recommended for release or cross-compiles).
2. Configure the project with CMake into a separate build directory.
3. Build with cmake --build (or ninja/msbuild on Windows).

Building Depends
-----------------
To build all dependencies for a target triplet using the depends system (example: native x86_64 Linux):

```bash
cd depends
make HOST=x86_64-pc-linux-gnu -j$(nproc)
```

For cross-compiling to Windows (mingw-w64):

```bash
cd depends
make HOST=x86_64-w64-mingw32 -j$(nproc)
```

See depends/README.md for additional targets and options (NO_QT, NO_SQLITE, etc.).

Native (Linux/macOS) build with CMake
------------------------------------
A simple native build (after installing system packages or building depends):

```bash
# From repository root
mkdir -p build
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -- -j$(nproc)
# Optional: install
sudo cmake --install build --prefix=/usr
```

Notes:
- To build without the GUI, add an appropriate CMake option (see the CMake cache or project documentation). You can configure options via -D<NAME>=<VALUE> on the cmake command line or using ccmake/cmake-gui.
- To enable debugging builds: -DCMAKE_BUILD_TYPE=RelWithDebInfo or Debug.

Cross-compiling using Depends
-----------------------------
When cross-compiling, build depends for the target triplet first (see above). The depends tree provides a toolchain file and a controlled prefix which you can point CMake at.

Example (cross-compiling to 64-bit Windows with mingw-w64):

```bash
# From repository root
cd depends
make HOST=x86_64-w64-mingw32 -j$(nproc)
cd ..

mkdir -p build-win64
cmake -S . -B build-win64 \
  -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_TOOLCHAIN_FILE=$(pwd)/depends/x86_64-w64-mingw32/share/toolchain.cmake
cmake --build build-win64 -- -j$(nproc)
```

Note: The exact path/name of the toolchain file may vary by depends layout and triplet. See depends/README.md and the contents of depends/<triplet>/share/ for the precise filename to use.

Windows (MSVC) builds
---------------------
For native MSVC builds, see build_msvc/README.md. The project includes a build_msvc helper that generates Visual Studio project files and explains how to build with msbuild or Visual Studio.

Options and Feature Flags
-------------------------
CMake variables can be passed with -D<VAR>=<VALUE>. Common choices you may want to set:
- -DENABLE_WALLET=OFF (or equivalent) to build without the legacy wallet — consult the CMake cache for the exact variable name used in this tree.
- -DWITH_GUI=OFF / -DWITH_QT=OFF to skip building the Qt GUI.
- -DCPACK_GENERATOR=ZIP to adjust packaging options for deploy

If you are unsure of the exact variable names supported by the project, configure once and inspect build/CMakeCache.txt or run:

```bash
cmake -S . -B build -LAH
```

Berkeley DB and Wallet Compatibility
-----------------------------------
Legacy wallet compatibility requires Berkeley DB 6.2.x. If you need to preserve binary wallet compatibility, either install a compatible BDB from your distribution or build it in depends. See doc/build-unix.md (Berkeley DB section) and depends/README.md for more information.

Disable-wallet mode
-------------------
To build a P2P-only node without wallet support, pass the appropriate CMake flag to disable the wallet (see the project CMake options as suggested above). When the wallet is disabled, dependencies such as Berkeley DB and SQLite are not required.

Distribution-specific notes
---------------------------
Package names and package-manager commands vary between distributions. Earlier autotools-specific examples (apt-get install ... && ./autogen.sh && ./configure) have been replaced by the CMake workflow shown above. For convenience, here are common packages to install on Debian/Ubuntu for a native build:

```bash
sudo apt update
sudo apt install build-essential pkg-config cmake git libtool libevent-dev libboost-dev libssl-dev python3 qtbase5-dev qttools5-dev-tools libsqlite3-dev
```

Refer to the distribution-specific sections in the original document for older examples; for reproducible builds or cross-compiles prefer depends.

Upstream reference
------------------
These build instructions follow the CMake + depends workflow used by Bitcoin Core. For additional examples and upstream rationale, see the Bitcoin Core build documentation and the depends/README.md in this repository.
