@echo off
echo Setting up dependencies using vcpkg for Soundboard project...

REM Check if vcpkg is installed
where vcpkg >nul 2>&1
if %ERRORLEVEL% NEQ 0 (
    echo Error: vcpkg not found in PATH. Please install vcpkg first.
    echo.
    echo To install vcpkg:
    echo 1. Clone vcpkg: git clone https://github.com/Microsoft/vcpkg.git
    echo 2. Run bootstrap: .\vcpkg\bootstrap-vcpkg.bat
    echo 3. Add vcpkg to PATH or run: .\vcpkg\vcpkg integrate install
    echo.
    pause
    exit /b 1
)

echo Installing GLFW3 and GLEW using vcpkg...

REM Install GLFW3
echo Installing GLFW3...
vcpkg install glfw3:x64-windows

if %ERRORLEVEL% NEQ 0 (
    echo Failed to install GLFW3. Please check vcpkg installation.
    pause
    exit /b 1
)

REM Install GLEW
echo Installing GLEW...
vcpkg install glew:x64-windows

if %ERRORLEVEL% NEQ 0 (
    echo Failed to install GLEW. Please check vcpkg installation.
    pause
    exit /b 1
)

REM Create directories for dependencies
if not exist "deps" mkdir deps
cd deps

REM Download dirent.h for Windows (vcpkg doesn't have this)
echo Downloading dirent.h for Windows...
if not exist "dirent" (
    mkdir dirent
    cd dirent
    curl -L -o dirent.h "https://raw.githubusercontent.com/tronkko/dirent/master/include/dirent.h"
    if exist dirent.h (
        echo dirent.h downloaded successfully.
    ) else (
        echo Failed to download dirent.h. Please check your internet connection.
    )
    cd ..
) else (
    echo dirent.h already exists.
)

cd ..

echo.
echo Dependencies setup complete!
echo.
echo vcpkg has installed:
echo   - GLFW3 (x64-windows)
echo   - GLEW (x64-windows)
echo.
echo Local dependencies:
echo   deps/
echo   └── dirent/
echo       └── dirent.h
echo.
echo You can now run build_vcpkg.bat to compile your project.

pause
