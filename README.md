# Soundboard Project 🎵

A simple, fun, and easy-to-use soundboard application built with OpenGL.  
It plays `.wav` files from the same directory and displays them in a neat grid.

## ✨ Features

-   **Dynamic Grid Layout**: Sound tiles are arranged in a responsive grid.
-   **Visual Feedback**: See playback progress and get hover effects on tiles.
-   **Auto-Refresh**: Automatically detects new `.wav` files added to the directory.
-   **Scrolling Text**: Long filenames scroll like a marquee when you hover over them.
-   **Easy to Build**: Comes with simple scripts for installation and building.

## 📋 Prerequisites

-   **C compiler** with C99 support (`clang` or `gcc`)
-   **Git**
-   **pkg-config** (`pkgconf` on Alpine)
-   An **Internet connection** for downloading dependencies
-   **Linux (Alpine)** or **Windows**

## 🚀 Getting Started

### 1. Install Dependencies (Alpine Linux)

Install system tools:

```sh
apk add --no-cache build-base clang git pkgconf
```

Then install project dependencies with vcpkg:

```sh
./install.sh
```

### 2. Build the Project (Alpine Linux)

```sh
./build.sh
```

This creates `build/soundboard`.

### 3. Run the Application (Alpine Linux)

```sh
cd build
./soundboard
```

### Windows Quick Start

If you are on Windows, use the original scripts:

### 1. Install Dependencies (Windows)

First, run the installation script. This will download and set up all the necessary libraries.

```cmd
install.bat
```

This script will:
-   Clone and bootstrap the **vcpkg** package manager.
-   Install **GLFW** (for window management) and **GLEW** (for OpenGL extensions).
-   Download `dirent.h` for directory operations on Windows.

### 2. Build the Project (Windows)

Next, compile the project using the build script.

```cmd
build.bat
```

This will compile the project using Clang with static linking and create `build/soundboard.exe`.  
Because it's statically linked, you don't need to worry about any extra DLL files!

### 3. Run the Application (Windows)

Finally, run the executable from the `build` directory.

```cmd
cd build
soundboard.exe
```

## 🎧 Usage

-   Place your `.wav` files in the `build` directory alongside the executable.
-   The application will automatically find them and create clickable tiles.
-   Click on a tile to play the sound.
-   Use your mouse wheel to scroll if you have a lot of sounds.
-   Click the "Refresh" button to manually rescan for new sounds.

## 📂 Project Structure

```
Soundboard/
├── src/
│   ├── main.c             # 🚀 Main application entry point
│   ├── soundboard.c/.h    # 🔊 Core soundboard logic
│   ├── renderer.c/.h      # 🎨 OpenGL rendering functions
│   ├── callbacks.c/.h     # 🖱️ GLFW window event callbacks
│   └── shaders.h          # ✨ GLSL shader source code
├── install.bat        # 📥 Downloads and sets up dependencies
├── build.bat          # 🛠️ Builds the project with Clang
├── README.md          # 📄 This file
├── vcpkg/             # vcpkg package manager (created by install script)
├── deps/              # Local dependencies (created by install script)
│   └── dirent/        # dirent.h for Windows
└── build/             # 📦 Build output (created by build script)
    ├── soundboard.exe # ✨ Statically linked executable
    └── *.wav          # 🎵 Your sound files
```

## 🔗 Dependencies

-   **GLFW**: A cross-platform library for creating windows with OpenGL contexts.
-   **GLEW**: The OpenGL Extension Wrangler Library for managing OpenGL extensions.
-   **dirent.h**: A header for POSIX directory operations on Windows.
-   **Windows API**: Used for sound playback (`PlaySound`) and filesystem events.
-   **vcpkg**: Microsoft's C++ package manager, used for easy dependency management.

## 🤔 Troubleshooting

-   **Linux has no audio output?** Ensure `aplay` is installed (`apk add alsa-utils`).
-   **Linux font text missing?** Ensure system fonts are installed (`apk add font-dejavu`).

-   **Build fails?** Make sure Clang is installed and its `bin` directory is in your system's PATH.
-   **vcpkg fails?** Ensure Git is installed and accessible from your terminal.
-   **Dependencies fail to download?** Check your internet connection.
-   **No sounds appear?** Make sure your `.wav` files are in the `build` directory where you run the executable.

## 🤖 Credits

This repo was created entirely using Github Copilot AI models:
- Claude Sonnet 4
- Gemini 2.5 Pro (Preview)
- GPT-5 (Preview)