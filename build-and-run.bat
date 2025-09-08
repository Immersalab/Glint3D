@echo off
REM Glint3D Build and Launch Script for Windows
REM Usage: build-and-run.bat [debug|release] [args...]

setlocal enabledelayedexpansion

REM Default to Debug if no config specified
set CONFIG=Debug
if not "%1"=="" (
    if /i "%1"=="release" set CONFIG=Release
    if /i "%1"=="debug" set CONFIG=Debug
)

REM Shift arguments if config was specified
set ARGS=
set FIRST_ARG=%1
if /i "!FIRST_ARG!"=="debug" shift
if /i "!FIRST_ARG!"=="release" shift

REM Collect remaining arguments
:loop
if not "%1"=="" (
    set ARGS=!ARGS! %1
    shift
    goto loop
)

echo.
echo ========================================
echo  Glint3D Build and Launch Script
echo ========================================
echo Configuration: %CONFIG%
echo Arguments: %ARGS%
echo.

REM Step 1: Generate CMake project files
echo [1/3] Generating CMake project files...
cmake -S . -B builds/desktop/cmake
if errorlevel 1 (
    echo ERROR: CMake generation failed
    exit /b 1
)

REM Step 2: Build the project
echo.
echo [2/3] Building %CONFIG% configuration...
cmake --build builds/desktop/cmake --config %CONFIG% -j
if errorlevel 1 (
    echo ERROR: Build failed
    exit /b 1
)

REM Step 3: Launch the application
echo.
echo [3/3] Launching Glint3D...
echo Command: builds\desktop\cmake\%CONFIG%\glint.exe %ARGS%
echo.
echo ========================================
echo  Glint3D is starting...
echo ========================================
echo.

builds\desktop\cmake\%CONFIG%\glint.exe %ARGS%

echo.
echo ========================================
echo  Glint3D has exited
echo ========================================