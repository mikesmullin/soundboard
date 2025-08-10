@echo off
echo Setting up dependencies for Soundboard project...

REM Create directories for dependencies
if not exist "deps" mkdir deps
cd deps

REM Download and build GLFW from source
echo Downloading GLFW source code...
if not exist "glfw" (
    curl -L -o glfw-source.zip "https://github.com/glfw/glfw/archive/refs/tags/3.3.8.zip"
    if exist glfw-source.zip (
        powershell -command "Expand-Archive -Path 'glfw-source.zip' -DestinationPath '.'"
        ren glfw-3.3.8 glfw-source
        del glfw-source.zip
        
        echo Building GLFW with clang...
        cd glfw-source
        mkdir build
        cd build
        
        REM Configure with CMake for MinGW/clang
        cmake .. -G "MinGW Makefiles" -DCMAKE_C_COMPILER=clang -DCMAKE_BUILD_TYPE=Release -DGLFW_BUILD_EXAMPLES=OFF -DGLFW_BUILD_TESTS=OFF -DGLFW_BUILD_DOCS=OFF
        
        REM Build GLFW
        mingw32-make
        
        cd ..\..
        
        REM Create glfw directory structure
        mkdir glfw
        mkdir glfw\include
        mkdir glfw\lib
        
        REM Copy built files
        xcopy /E /I glfw-source\include\* glfw\include\
        copy glfw-source\build\src\libglfw3.a glfw\lib\
        copy glfw-source\build\src\glfw3.dll glfw\lib\ 2>nul
        
        REM Clean up source directory
        rmdir /s /q glfw-source
        
        echo GLFW built and installed successfully.
    ) else (
        echo Failed to download GLFW source. Please check your internet connection.
    )
) else (
    echo GLFW already exists.
)

REM Download and build GLEW from source
echo Downloading GLEW source code...
if not exist "glew" (
    curl -L -o glew-source.zip "https://github.com/nigels-com/glew/archive/refs/tags/glew-2.2.0.zip"
    if exist glew-source.zip (
        powershell -command "Expand-Archive -Path 'glew-source.zip' -DestinationPath '.'"
        ren glew-glew-2.2.0 glew-source
        del glew-source.zip
        
        echo Building GLEW with clang...
        cd glew-source
        
        REM Build GLEW using the provided Makefile
        mingw32-make CC=clang
        
        cd ..
        
        REM Create glew directory structure
        mkdir glew
        mkdir glew\include
        mkdir glew\lib
        
        REM Copy built files
        xcopy /E /I glew-source\include\* glew\include\
        copy glew-source\lib\libglew32.a glew\lib\ 2>nul
        copy glew-source\lib\glew32.dll glew\lib\ 2>nul
        
        REM Clean up source directory
        rmdir /s /q glew-source
        
        echo GLEW built and installed successfully.
    ) else (
        echo Failed to download GLEW source. Please check your internet connection.
    )
) else (
    echo GLEW already exists.
)

REM Download dirent.h for Windows
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
echo Directory structure:
echo   deps/
echo   ├── glfw/
echo   │   ├── include/
echo   │   └── lib/
echo   ├── glew/
echo   │   ├── include/
echo   │   └── lib/
echo   └── dirent/
echo       └── dirent.h
echo.
echo You can now run build.bat to compile your project.

pause
