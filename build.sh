#!/usr/bin/env sh
set -eu

VCPKG_INSTALLED="${PWD}/vcpkg/installed/x64-linux"
PKGCONFIG_PATHS="${VCPKG_INSTALLED}/lib/pkgconfig:${VCPKG_INSTALLED}/share/pkgconfig"

if [ ! -d "${VCPKG_INSTALLED}" ]; then
  echo "Error: ${VCPKG_INSTALLED} not found. Run ./install.sh first."
  exit 1
fi

if ! command -v pkg-config >/dev/null 2>&1; then
  echo "Error: pkg-config is required. Install it first (apk add pkgconf)."
  exit 1
fi

export PKG_CONFIG_PATH="${PKGCONFIG_PATHS}:${PKG_CONFIG_PATH:-}"

if ! pkg-config --exists glfw3 glew freetype-gl freetype2; then
  echo "Error: Missing pkg-config metadata for glfw3/glew/freetype-gl/freetype2 in vcpkg x64-linux."
  echo "Run ./install.sh and verify vcpkg installation completed successfully."
  exit 1
fi

mkdir -p build

CC="${CC:-cc}"
CFLAGS="-std=c99 -Wall -Wextra -O2 -Isrc"
PKG_CFLAGS="$(pkg-config --cflags glfw3 glew freetype-gl freetype2)"
PKG_LIBS="$(pkg-config --libs glfw3 glew freetype-gl freetype2)"

set -x
${CC} ${CFLAGS} ${PKG_CFLAGS} \
  -o build/soundboard \
  src/main.c src/renderer.c src/soundboard.c src/callbacks.c \
  ${PKG_LIBS} -lGLX -lm -pthread -ldl
set +x

echo "Build successful: build/soundboard"
echo "Run it with: cd build && ./soundboard"
