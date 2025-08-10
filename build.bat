@echo off
echo Building Soundboard project...

REM Check if vcpkg dependencies exist
if not exist "vcpkg\installed\x64-windows\lib\glfw3dll.lib" (
    echo Error: GLFW not found in vcpkg. Please run install.bat first.
    pause
    exit /b 1
)

if not exist "vcpkg\installed\x64-windows\lib\glew32.lib" (
    echo Error: GLEW not found in vcpkg. Please run install.bat first.
    pause
    exit /b 1
)

if not exist "deps\dirent" (
    echo Error: dirent.h not found. Please run install.bat first.
    pause
    exit /b 1
)

REM Set vcpkg paths
set VCPKG_ROOT=%CD%\vcpkg
set VCPKG_INSTALLED=%VCPKG_ROOT%\installed\x64-windows

REM Set compiler and flags
set CC=clang
set CFLAGS=-std=c99 -Wall -Wextra -O2
set INCLUDES=-I"%VCPKG_INSTALLED%\include" -I"deps\dirent"
set LIBS=-L"%VCPKG_INSTALLED%\lib"
set LINK_LIBS="%VCPKG_INSTALLED%\lib\glfw3dll.lib" "%VCPKG_INSTALLED%\lib\glew32.lib" "%VCPKG_INSTALLED%\lib\OpenGL32.lib" -lwinmm -lgdi32 -luser32 -lkernel32 -lshell32

REM Create output directory
if not exist "build" mkdir build

REM Compile
echo Compiling main.c...
echo Using vcpkg libraries from: %VCPKG_INSTALLED%
%CC% %CFLAGS% %INCLUDES% -o build\soundboard.exe main.c %LINK_LIBS% -Xlinker /SUBSYSTEM:CONSOLE

if %ERRORLEVEL% EQU 0 (
    echo.
    echo Copying required DLLs...
    copy "%VCPKG_INSTALLED%\bin\*.dll" "build\" >nul
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
)

pause
