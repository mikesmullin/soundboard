@echo off
echo Installing dependencies for Soundboard project...

REM Check if vcpkg directory already exists
if exist "vcpkg" (
    echo vcpkg directory already exists.
    cd vcpkg
    git pull
    cd ..
) else (
    echo Cloning vcpkg...
    git clone https://github.com/Microsoft/vcpkg.git
    if %ERRORLEVEL% NEQ 0 (
        echo Failed to clone vcpkg. Please check your git installation and internet connection.
        pause
        exit /b 1
    )
)

REM Bootstrap vcpkg
echo Bootstrapping vcpkg...
cd vcpkg
call bootstrap-vcpkg.bat
if %ERRORLEVEL% NEQ 0 (
    echo Failed to bootstrap vcpkg.
    pause
    exit /b 1
)

REM Integrate vcpkg
echo Integrating vcpkg...
.\vcpkg integrate install

REM Install GLFW3
echo Installing GLFW3...
.\vcpkg install glfw3:x64-windows

if %ERRORLEVEL% NEQ 0 (
    echo Failed to install GLFW3.
    pause
    exit /b 1
)

REM Install GLEW
echo Installing GLEW...
.\vcpkg install glew:x64-windows

if %ERRORLEVEL% NEQ 0 (
    echo Failed to install GLEW.
    pause
    exit /b 1
)

cd ..

REM Create directories for local dependencies
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
echo vcpkg has been installed and configured with:
echo   - GLFW3 (x64-windows)
echo   - GLEW (x64-windows)
echo.
echo Local dependencies:
echo   deps/
echo   └── dirent/
echo       └── dirent.h
echo.
echo You can now run build.bat to compile your project.

pause
