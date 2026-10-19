WINDOWS BUILD NOTES
====================

This document describes building Dork Core (Dorkcoin) for Windows using the CMake + depends workflow and the existing MSVC helper. The previous autotools-based configure steps are deprecated; use CMake for native and cross builds.

Build options (summary)
- Cross-compile on Linux using the mingw-w64 toolchain + depends (recommended for release ZIPs).
- Build on Windows using WSL + mingw-w64 (the depends cross-build approach works in WSL).
- Build on Windows with Microsoft Visual Studio (see build_msvc/README.md).

Prerequisites (on Ubuntu/WSL)
-----------------------------
Install general build tools and cmake:

```bash
sudo apt update
sudo apt install build-essential libtool autotools-dev automake pkg-config bsdmainutils cmake curl git python3
```

If you plan to create a Windows installer (NSIS):

```bash
sudo apt install nsis
```

Cross-compile with depends (64-bit Windows example)
---------------------------------------------------
1. Build depends for the mingw target:

```bash
cd depends
make HOST=x86_64-w64-mingw32 -j$(nproc)
cd ..
```

2. Configure and build with CMake, pointing at the depends-provided toolchain file:

```bash
mkdir -p build-win64
cmake -S . -B build-win64 \
  -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_TOOLCHAIN_FILE=$(pwd)/depends/x86_64-w64-mingw32/share/toolchain.cmake
cmake --build build-win64 -- -j$(nproc)
```

Notes:
- The exact toolchain filename and location may vary by depends layout and triplet. Inspect depends/<triplet>/share/ for the correct file name and adapt the -DCMAKE_TOOLCHAIN_FILE path accordingly.
- For WSL, ensure the source tree is on the Linux filesystem (e.g., under /home or /usr/src) and not on /mnt/c or other Windows-mounted paths to avoid issues with autoconf-generated host tools and file permissions.

Using mingw-w64 directly (without depends)
-----------------------------------------
If you prefer to use a system-installed mingw-w64 toolchain, install the cross-compiler (example on Ubuntu):

```bash
sudo apt install g++-mingw-w64-x86-64-posix
```

Then point CMake at the appropriate toolchain file for your cross toolchain, or set CC/CXX and other environment variables as needed.

Native Windows (MSVC) build
---------------------------
For native builds with Microsoft Visual Studio / MSBuild, see build_msvc/README.md in this repository. The build_msvc helper will generate Visual Studio project files from the CMake/Makefile infrastructure and documents required MSVC/Qt/packaging steps.

Installers and deployment
-------------------------
After a successful build you can:
- Use cmake --install to install into a staging directory.
- Create release ZIP archives from the compiled executables.
- Use NSIS (via make deploy or an appropriate CPack generator) to build an installer. See CI/packaging scripts or the upstream documentation for examples.

Further reading
---------------
- depends/README.md in this repository for depends usage and triplet/toolchain details.
- build_msvc/README.md for Visual Studio/MSVC-specific instructions.
- Upstream Bitcoin Core build docs for additional examples and rationale.
