#!/usr/bin/env sh
set -eu

echo "Installing dependencies for Soundboard (Linux)..."

if [ ! -d "vcpkg" ]; then
  echo "Cloning vcpkg..."
  git clone https://github.com/Microsoft/vcpkg.git
else
  echo "vcpkg already exists. Updating..."
  (cd vcpkg && git pull --ff-only)
fi

echo "Bootstrapping vcpkg..."
(cd vcpkg && ./bootstrap-vcpkg.sh)

echo "Installing libraries via vcpkg (x64-linux)..."
./vcpkg/vcpkg install glfw3:x64-linux glew:x64-linux freetype-gl:x64-linux

echo "Done. Run ./build.sh to compile."
