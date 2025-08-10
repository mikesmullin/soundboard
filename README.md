# Soundboard Project

A simple OpenGL-based soundboard application that plays .wav files from the current directory.

## Prerequisites

- Clang compiler (install via LLVM or Visual Studio Build Tools)
- Internet connection for downloading dependencies
- Windows OS

## Setup

1. **Install Dependencies**
   ```cmd
   setup_dependencies.bat
   ```
   This will download and extract:
   - GLFW 3.3.8 (window management)
   - GLEW 2.2.0 (OpenGL extensions)
   - dirent.h (directory operations for Windows)

2. **Build the Project**
   ```cmd
   build.bat
   ```
   This will compile the project using Clang and create `build/soundboard.exe`

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
├── setup_dependencies.bat    # Downloads and sets up dependencies
├── build.bat                # Builds the project with Clang
├── README.md                # This file
├── deps/                    # Dependencies (created by setup script)
│   ├── glfw/               # GLFW library
│   ├── glew/               # GLEW library
│   └── dirent/             # dirent.h for Windows
└── build/                  # Build output (created by build script)
    └── soundboard.exe      # Compiled executable
```

## Dependencies

- **GLFW**: Cross-platform library for creating windows with OpenGL contexts
- **GLEW**: OpenGL Extension Wrangler Library for loading OpenGL extensions
- **dirent.h**: POSIX directory operations for Windows compatibility
- **Windows API**: For sound playback (PlaySound function)

## Troubleshooting

- If the build fails, ensure Clang is installed and available in your PATH
- If dependencies fail to download, check your internet connection
- Make sure you have .wav files in the directory where you run the executable
