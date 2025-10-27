@echo off
REM Glint3D Build and Run Script (Pre-RHI Release v1.0)
REM Automatically checks/installs dependencies, builds, and launches

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
echo  Glint3D Build and Run Script
echo ========================================
echo Configuration: %CONFIG%
echo.

REM Navigate to repository root (parent of tools folder)
cd /d "%~dp0.."
echo Working directory: %CD%
echo.

REM Step 1: Check for GLFW3 dependencies
echo [1/4] Checking dependencies...
set LIB_DIR=engine\Libraries\lib
if not exist "%LIB_DIR%\glfw3.lib" (
    echo GLFW3 not found. Downloading and installing...
    echo.
    call :install_glfw
    if errorlevel 1 (
        echo ERROR: Failed to install dependencies
        exit /b 1
    )
) else (
    echo GLFW3 found: %LIB_DIR%\glfw3.lib
)

echo.
echo [2/4] Configuring CMake...
echo Using libraries from: engine/Libraries
cmake -S . -B builds/desktop/cmake ^
    -DCMAKE_PREFIX_PATH="%CD%\engine\Libraries" ^
    -DGLFW3_INCLUDE_DIR="%CD%\engine\Libraries\include" ^
    -DCMAKE_BUILD_TYPE=%CONFIG%
if errorlevel 1 (
    echo ERROR: CMake configuration failed
    exit /b 1
)

echo.
echo [3/4] Building %CONFIG%...
cmake --build builds/desktop/cmake --config %CONFIG% -j
if errorlevel 1 (
    echo ERROR: Build failed
    exit /b 1
)

echo.
echo [4/4] Launching Glint3D...
set EXE=builds\desktop\cmake\%CONFIG%\glint.exe
if not exist "%EXE%" (
    echo ERROR: Executable not found at %EXE%
    exit /b 1
)

echo.
echo ========================================
echo  Build Complete - Launching
echo ========================================
echo.

start "" "%EXE%" %ARGS%
exit /b 0

REM ==========================================
REM GLFW Installation Subroutine
REM ==========================================
:install_glfw
set TEMP_DIR=engine\Libraries\glfw-temp

REM Create lib directory if needed
if not exist "%LIB_DIR%" mkdir "%LIB_DIR%"

echo Downloading GLFW 3.3.8 for Windows (VC2022)...
curl -L -o "%TEMP_DIR%.zip" https://github.com/glfw/glfw/releases/download/3.3.8/glfw-3.3.8.bin.WIN64.zip
if errorlevel 1 (
    echo ERROR: Failed to download GLFW
    goto :cleanup_glfw
)

echo Extracting...
powershell -Command "Expand-Archive -Path '%TEMP_DIR%.zip' -DestinationPath '%TEMP_DIR%' -Force"
if errorlevel 1 (
    echo ERROR: Failed to extract GLFW
    goto :cleanup_glfw
)

echo Installing libraries...
copy /Y "%TEMP_DIR%\glfw-3.3.8.bin.WIN64\lib-vc2022\*.lib" "%LIB_DIR%\" >nul
copy /Y "%TEMP_DIR%\glfw-3.3.8.bin.WIN64\lib-vc2022\*.dll" "%LIB_DIR%\" >nul

echo GLFW3 installed successfully to: %LIB_DIR%
echo.

:cleanup_glfw
if exist "%TEMP_DIR%.zip" del /Q "%TEMP_DIR%.zip"
if exist "%TEMP_DIR%" rmdir /S /Q "%TEMP_DIR%"
exit /b 0
