@echo off
echo Killing any existing soundboard process...
taskkill /F /IM soundboard.exe 2>nul

echo Building Soundboard project...

REM Check if vcpkg dependencies exist
if not exist "vcpkg\installed\x64-windows-static\lib\glfw3.lib" (
    echo Error: GLFW static library not found in vcpkg. Please run install.bat first.
    pause
    exit /b 1
)

if not exist "vcpkg\installed\x64-windows-static\lib\glew32.lib" (
    echo Error: GLEW static library not found in vcpkg. Please run install.bat first.
    pause
    exit /b 1
)

if not exist "vcpkg\installed\x64-windows-static\lib\freetype-gl.lib" (
    echo Error: FreeType-GL library not found in vcpkg. Please run install.bat first.
    pause
    exit /b 1
)

if not exist "vcpkg\installed\x64-windows-static\include\dirent.h" (
    echo Error: dirent.h not found in vcpkg. Please run install.bat first.
    pause
    exit /b 1
)

REM Set vcpkg paths
set VCPKG_ROOT=%CD%\vcpkg
set VCPKG_INSTALLED=%VCPKG_ROOT%\installed\x64-windows-static

REM Set compiler and flags
set CC=clang
set CFLAGS=-std=c99 -Wall -Wextra -O2
set INCLUDES=-I"src" -I"%VCPKG_INSTALLED%\include"
set LIBS=-L"%VCPKG_INSTALLED%\lib"
set LINK_LIBS="%VCPKG_INSTALLED%\lib\glfw3.lib" "%VCPKG_INSTALLED%\lib\glew32.lib" "%VCPKG_INSTALLED%\lib\freetype-gl.lib" "%VCPKG_INSTALLED%\lib\freetype.lib" "%VCPKG_INSTALLED%\lib\libpng16.lib" "%VCPKG_INSTALLED%\lib\zlib.lib" "%VCPKG_INSTALLED%\lib\brotlidec.lib" "%VCPKG_INSTALLED%\lib\brotlicommon.lib" "%VCPKG_INSTALLED%\lib\bz2.lib" "%VCPKG_INSTALLED%\lib\OpenGL32.lib" -lwinmm -lgdi32 -luser32 -lkernel32 -lshell32

REM Create output directory
if not exist "build" mkdir build

REM Compile
echo Compiling soundboard project...
echo Using vcpkg libraries from: %VCPKG_INSTALLED%
%CC% %CFLAGS% %INCLUDES% -o build\soundboard.exe src\main.c src\renderer.c src\soundboard.c src\callbacks.c %LINK_LIBS% -Xlinker /SUBSYSTEM:WINDOWS

if %ERRORLEVEL% EQU 0 (
    echo.
    echo Build successful! Windows executable created at: build\soundboard.exe
    echo.
    echo Launching soundboard...
    cd build\
    start soundboard.exe
    cd ..
) else (
    echo.
    echo Build failed! Please check the error messages above.
)

