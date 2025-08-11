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
echo Installing GLFW3 (both dynamic and static)...
.\vcpkg install glfw3:x64-windows
.\vcpkg install glfw3:x64-windows-static

if %ERRORLEVEL% NEQ 0 (
    echo Failed to install GLFW3.
    pause
    exit /b 1
)

REM Install GLEW
echo Installing GLEW (both dynamic and static)...
.\vcpkg install glew:x64-windows
.\vcpkg install glew:x64-windows-static

if %ERRORLEVEL% NEQ 0 (
    echo Failed to install GLEW.
    pause
    exit /b 1
)

REM Install dirent
echo Installing dirent...
.\vcpkg install dirent:x64-windows-static

if %ERRORLEVEL% NEQ 0 (
    echo Failed to install dirent.
    pause
    exit /b 1
)

cd ..

echo.
echo Dependencies setup complete!
echo.
echo vcpkg has been installed and configured with:
echo   - GLFW3 (x64-windows)
echo   - GLEW (x64-windows)
echo   - dirent (x64-windows-static)
echo.
echo You can now run build.bat to compile your project.

pause
