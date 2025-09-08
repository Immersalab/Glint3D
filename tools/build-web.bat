@echo off
setlocal enabledelayedexpansion

REM Glint3D Web Build Script for Windows
REM Builds both the engine (WASM) and web frontend

echo === Glint3D Web Build Script ===

REM Check if we're in the right directory
if not exist "CMakeLists.txt" (
    echo Error: Must be run from the glint3d root directory
    exit /b 1
)
if not exist "engine" (
    echo Error: Must be run from the glint3d root directory
    exit /b 1
)

REM Default build type
set BUILD_TYPE=%1
if "%BUILD_TYPE%"=="" set BUILD_TYPE=Release

echo Building Glint3D Web (%BUILD_TYPE%)

REM Step 1: Build engine for web (WASM)
echo.
echo ^>^>^> Step 1: Building C++ engine for WASM

REM Check if emscripten is available
emcc --version >nul 2>&1
if errorlevel 1 (
    echo Error: Emscripten not found. Please install and activate the Emscripten SDK:
    echo   git clone https://github.com/emscripten-core/emsdk.git
    echo   cd emsdk
    echo   emsdk install latest
    echo   emsdk activate latest
    echo   emsdk_env.bat
    exit /b 1
)

REM Create build directory
if not exist "builds\web\emscripten" mkdir builds\web\emscripten

REM Configure with emscripten
echo Configuring CMake with Emscripten...
emcmake cmake -S . -B builds\web\emscripten -DCMAKE_BUILD_TYPE=%BUILD_TYPE%
if errorlevel 1 (
    echo Error: CMake configuration failed
    exit /b 1
)

REM Build the engine
echo Compiling C++ engine to WASM...
cmake --build builds\web\emscripten -j
if errorlevel 1 (
    echo Error: Engine build failed
    exit /b 1
)

echo ✓ Engine WASM build completed

REM Step 2: Copy engine artifacts to web frontend
echo.
echo ^>^>^> Step 2: Copying WASM artifacts to web frontend

REM Create engine directory in web public folder
if not exist "web\public\engine" mkdir web\public\engine

REM Copy engine files (handle both possible output names)
if exist "builds\web\emscripten\glint3d.js" (
    copy builds\web\emscripten\glint3d.* web\public\engine\ >nul
    echo ✓ Copied glint3d.* files to web\public\engine\
) else if exist "builds\web\emscripten\objviewer.js" (
    copy builds\web\emscripten\objviewer.* web\public\engine\ >nul
    echo ✓ Copied objviewer.* files to web\public\engine\
) else (
    echo Error: No engine WASM artifacts found. Expected glint3d.* or objviewer.* files.
    exit /b 1
)

REM Step 3: Build packages if needed
echo.
echo ^>^>^> Step 3: Building TypeScript packages
npm run build:packages
if errorlevel 1 (
    echo Error: Package build failed
    exit /b 1
)

echo ✓ Packages built successfully

REM Step 4: Build web frontend
echo.
echo ^>^>^> Step 4: Building React web frontend
npm run build:web
if errorlevel 1 (
    echo Error: Web frontend build failed
    exit /b 1
)

echo ✓ Web frontend built successfully

REM Step 5: Show build summary
echo.
echo ^>^>^> Build Summary

echo Engine WASM artifacts:
dir /b builds\web\emscripten\*.js builds\web\emscripten\*.wasm builds\web\emscripten\*.data builds\web\emscripten\*.html 2>nul

echo.
echo Web frontend build:
dir /b web\dist\

echo.
echo Web engine assets:
dir /b web\public\engine\

echo.
echo ✓ Web build completed successfully!

echo.
echo ^>^>^> To serve the web build:
echo   npm run dev             # Development server
echo   npm run preview         # Preview production build
echo   emrun --port 8080 builds\web\emscripten\glint3d.html  # Serve WASM directly

exit /b 0