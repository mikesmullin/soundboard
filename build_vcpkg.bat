@echo off
echo Building Soundboard project with vcpkg dependencies...

REM Check if vcpkg is installed locally or in PATH
if exist "vcpkg\vcpkg.exe" (
    set VCPKG_EXE=%CD%\vcpkg\vcpkg.exe
    set VCPKG_ROOT=%CD%\vcpkg
) else (
    where vcpkg >nul 2>&1
    if %ERRORLEVEL% NEQ 0 (
        echo Error: vcpkg not found. Please run install_vcpkg_and_deps.bat first.
        pause
        exit /b 1
    )
    REM Get vcpkg root directory from PATH
    for /f "tokens=*" %%i in ('where vcpkg') do set VCPKG_EXE=%%i
    for %%i in ("%VCPKG_EXE%") do set VCPKG_ROOT=%%~dpi
    set VCPKG_ROOT=%VCPKG_ROOT:~0,-1%
)

REM Check if local dirent.h exists
if not exist "deps\dirent" (
    echo Error: dirent.h not found. Please run install_vcpkg_and_deps.bat first.
    pause
    exit /b 1
)

REM Set vcpkg toolchain paths
set VCPKG_INSTALLED=%VCPKG_ROOT%\installed\x64-windows

REM Set compiler and flags
set CC=clang
set CFLAGS=-std=c99 -Wall -Wextra -O2 -DGLEW_STATIC
set INCLUDES=-I"%VCPKG_INSTALLED%\include" -I"deps\dirent"
set LIBS=-L"%VCPKG_INSTALLED%\lib"
set LINK_LIBS="%VCPKG_INSTALLED%\lib\glfw3.lib" "%VCPKG_INSTALLED%\lib\glew32s.lib" -lopengl32 -lgdi32 -luser32 -lkernel32 -lwinmm -lshell32

REM Create output directory
if not exist "build" mkdir build

REM Compile
echo Compiling main.c...
echo Using vcpkg libraries from: %VCPKG_INSTALLED%
%CC% %CFLAGS% %INCLUDES% -o build\soundboard.exe main.c %LINK_LIBS%

if %ERRORLEVEL% EQU 0 (
    echo.
    echo Build successful! Executable created at: build\soundboard.exe
    echo.
    echo To run the program:
    echo   cd build
    echo   soundboard.exe
    echo.
    echo Make sure you have .wav files in the same directory as the executable.
) else (
    echo.
    echo Build failed! Please check the error messages above.
    echo.
    echo Make sure you have run: setup_dependencies_vcpkg.bat
)

pause
