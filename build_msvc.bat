@echo off
echo Building Soundboard with MSVC (cl.exe)...

REM Check if dependencies exist
if not exist "deps\glfw" (
    echo Error: GLFW not found. Please run setup_dependencies.bat first.
    pause
    exit /b 1
)

if not exist "deps\glew" (
    echo Error: GLEW not found. Please run setup_dependencies.bat first.
    pause
    exit /b 1
)

if not exist "deps\dirent" (
    echo Error: dirent.h not found. Please run setup_dependencies.bat first.
    pause
    exit /b 1
)

REM Set up Visual Studio environment
call "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat" 2>nul
if %ERRORLEVEL% NEQ 0 (
    call "C:\Program Files (x86)\Microsoft Visual Studio\2019\Community\VC\Auxiliary\Build\vcvars64.bat" 2>nul
    if %ERRORLEVEL% NEQ 0 (
        call "C:\Program Files (x86)\Microsoft Visual Studio\2017\Community\VC\Auxiliary\Build\vcvars64.bat" 2>nul
        if %ERRORLEVEL% NEQ 0 (
            echo Error: Visual Studio not found. Please install Visual Studio or ensure it's in the correct path.
            pause
            exit /b 1
        )
    )
)

REM Set compiler and flags
set CC=cl
set CFLAGS=/std:c11 /W3 /O2 /DGLEW_STATIC
set INCLUDES=/I"deps\glfw\include" /I"deps\glew\include" /I"deps\dirent"
set LINK_LIBS=deps\glfw\lib-vc2022\glfw3_mt.lib deps\glew\lib\Release\x64\glew32s.lib opengl32.lib gdi32.lib user32.lib kernel32.lib winmm.lib

REM Create output directory
if not exist "build" mkdir build

REM Compile
echo Compiling main.c...
%CC% %CFLAGS% %INCLUDES% main.c /Fe:build\soundboard.exe /link %LINK_LIBS%

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
)

pause
