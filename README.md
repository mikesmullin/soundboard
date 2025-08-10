# Soundboard Project

A simple OpenGL-based soundboard application that plays .wav files from the current directory.

## Prerequisites

- Clang compiler (install via LLVM or Visual Studio Build Tools)
- Git (for downloading vcpkg)
- Internet connection for downloading dependencies
- Windows OS

## Setup

1. **Install Dependencies**
   ```cmd
   install.bat
   ```
   This will:
   - Clone and bootstrap vcpkg package manager
   - Install GLFW 3.x (window management) via vcpkg
   - Install GLEW 2.x (OpenGL extensions) via vcpkg
   - Download dirent.h (directory operations for Windows)

2. **Build the Project**
   ```cmd
   build.bat
   ```
   This will compile the project using Clang and create `build/soundboard.exe`
   Required DLLs are automatically copied to the build directory.

3. **Run the Application**
   ```cmd
   cd build
   soundboard.exe
   ```

## Usage

- Place .wav files in the same directory as the executable
- The application will automatically scan for .wav files and create clickable tiles
- Click on tiles to play sounds
- Use mouse wheel to scroll through the list if you have many files

## Project Structure

```
Soundboard/
├── main.c                    # Main source code
├── install.bat              # Downloads and sets up dependencies via vcpkg
├── build.bat                # Builds the project with Clang
├── README.md                # This file
├── vcpkg/                   # vcpkg package manager (created by install script)
├── deps/                    # Local dependencies (created by install script)
│   └── dirent/             # dirent.h for Windows
└── build/                  # Build output (created by build script)
    ├── soundboard.exe      # Compiled executable
    ├── glfw3.dll           # GLFW runtime library
    └── glew32.dll          # GLEW runtime library
```

## Dependencies

- **GLFW**: Cross-platform library for creating windows with OpenGL contexts (installed via vcpkg)
- **GLEW**: OpenGL Extension Wrangler Library for loading OpenGL extensions (installed via vcpkg)
- **dirent.h**: POSIX directory operations for Windows compatibility (downloaded directly)
- **Windows API**: For sound playback (PlaySound function)
- **vcpkg**: Microsoft's C++ package manager for dependency management

## Troubleshooting

- If the build fails, ensure Clang is installed and available in your PATH
- If vcpkg installation fails, ensure Git is installed and accessible
- If dependencies fail to download, check your internet connection
- Make sure you have .wav files in the directory where you run the executable
