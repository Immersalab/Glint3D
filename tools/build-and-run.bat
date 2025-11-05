REM Machine Summary Block
REM {"file":"tools/build-and-run.bat","purpose":"Automates dependency checks and desktop builds for Glint3D on Windows.","exports":[],"depends_on":["cmake","curl","powershell"],"notes":["glfw_managed_dropin_supported","assimp_oidn_support"]}
REM Human Summary
REM Windows helper that verifies GLFW/Doxygen, configures CMake, builds Glint3D, and supports managed GLFW drop-ins.
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
echo [1/5] Checking dependencies...
set MANAGED_GLFW_DIR=third_party\managed\glfw
set MANAGED_ASSIMP_DIR=third_party\managed\assimp
set MANAGED_OIDN_DIR=third_party\managed\openimagedenoise
set LIB_DIR=%MANAGED_GLFW_DIR%\lib
set INCLUDE_DIR=%MANAGED_GLFW_DIR%\include
set BIN_DIR=%MANAGED_GLFW_DIR%\bin

if not exist "%LIB_DIR%\glfw3*.lib" (
    echo GLFW3 not found. Downloading and installing...
    echo.
    call :install_glfw
    if errorlevel 1 (
        echo ERROR: Failed to install dependencies
        exit /b 1
    )
) else (
    echo GLFW3 found in %LIB_DIR%
)

REM Check for Doxygen (optional)
where doxygen >nul 2>nul
if %ERRORLEVEL% NEQ 0 (
    echo.
    echo [INFO] Doxygen not found - documentation generation will be unavailable
    echo Install Doxygen from: https://www.doxygen.nl/download.html
    echo Then run: tools\generate-docs.bat
) else (
    echo Doxygen found: ready for documentation generation
)

call :check_assimp
call :check_oidn

set "CMAKE_EXTRA_FLAGS=-DCMAKE_BUILD_TYPE=%CONFIG%"
if defined VCPKG_ROOT (
    if exist "%VCPKG_ROOT%\scripts\buildsystems\vcpkg.cmake" (
        set "CMAKE_EXTRA_FLAGS=%CMAKE_EXTRA_FLAGS% -DCMAKE_TOOLCHAIN_FILE=%VCPKG_ROOT%\scripts\buildsystems\vcpkg.cmake"
    )
)

echo.
echo [2/5] Configuring CMake...
echo Using optional managed libraries from: %MANAGED_GLFW_DIR%
cmake -S . -B builds/desktop/cmake ^
    -DCMAKE_PREFIX_PATH="%CD%\%MANAGED_GLFW_DIR%" ^
    -DGLFW3_INCLUDE_DIR="%CD%\%MANAGED_GLFW_DIR%\include" ^
    %CMAKE_EXTRA_FLAGS%
if errorlevel 1 (
    echo ERROR: CMake configuration failed
    exit /b 1
)

echo.
echo [3/5] Building %CONFIG%...
cmake --build builds/desktop/cmake --config %CONFIG% -j
if errorlevel 1 (
    echo ERROR: Build failed
    exit /b 1
)

echo.
echo [4/5] Verifying executable...
set EXE=builds\desktop\cmake\%CONFIG%\glint.exe
if not exist "%EXE%" (
    echo ERROR: Executable not found at %EXE%
    exit /b 1
)

echo.
echo [5/5] Launching Glint3D...
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
set TEMP_DIR=%MANAGED_GLFW_DIR%\glfw-temp

REM Create directories if needed
if not exist "%MANAGED_GLFW_DIR%" mkdir "%MANAGED_GLFW_DIR%"
if not exist "%LIB_DIR%" mkdir "%LIB_DIR%"
if not exist "%INCLUDE_DIR%" mkdir "%INCLUDE_DIR%"
if not exist "%BIN_DIR%" mkdir "%BIN_DIR%"

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
 del /Q "%LIB_DIR%\glfw3*.lib" >nul 2>nul
 copy /Y "%TEMP_DIR%\glfw-3.3.8.bin.WIN64\lib-vc2022\*.lib" "%LIB_DIR%\" >nul
 del /Q "%BIN_DIR%\glfw3*.dll" >nul 2>nul
 copy /Y "%TEMP_DIR%\glfw-3.3.8.bin.WIN64\lib-vc2022\*.dll" "%BIN_DIR%\" >nul
 if exist "%INCLUDE_DIR%\GLFW" rmdir /S /Q "%INCLUDE_DIR%\GLFW"
 xcopy /E /I /Y "%TEMP_DIR%\glfw-3.3.8.bin.WIN64\include\GLFW" "%INCLUDE_DIR%\GLFW" >nul

echo GLFW3 installed successfully to: %MANAGED_GLFW_DIR%
echo.

:cleanup_glfw
if exist "%TEMP_DIR%.zip" del /Q "%TEMP_DIR%.zip"
if exist "%TEMP_DIR%" rmdir /S /Q "%TEMP_DIR%"
exit /b 0

REM ==========================================
REM Assimp / OpenImageDenoise helpers
REM ==========================================
:check_assimp
if defined VCPKG_ROOT (
    if exist "%VCPKG_ROOT%\installed\x64-windows\lib\assimp*.lib" (
        echo Assimp found via vcpkg (%VCPKG_ROOT%)
        goto :eof
    )
    echo Assimp not detected in vcpkg. Attempting install...
    call :install_assimp_vcpkg
    goto :eof
)
if exist "%MANAGED_ASSIMP_DIR%\lib\assimp*.lib" (
    echo Assimp found in %MANAGED_ASSIMP_DIR%
) else (
    echo [INFO] Assimp not detected. Install via vcpkg (set VCPKG_ROOT) or populate %MANAGED_ASSIMP_DIR%.
)
goto :eof

:check_oidn
if defined VCPKG_ROOT (
    if exist "%VCPKG_ROOT%\installed\x64-windows\lib\OpenImageDenoise*.lib" (
        echo OpenImageDenoise found via vcpkg (%VCPKG_ROOT%)
        goto :eof
    )
    echo OpenImageDenoise not detected in vcpkg. Attempting install...
    call :install_oidn_vcpkg
    goto :eof
)
if exist "%MANAGED_OIDN_DIR%\lib\OpenImageDenoise*.lib" (
    echo OpenImageDenoise found in %MANAGED_OIDN_DIR%
) else (
    echo [INFO] OpenImageDenoise not detected. Install via vcpkg (set VCPKG_ROOT) or populate %MANAGED_OIDN_DIR%.
)
goto :eof

:install_assimp_vcpkg
if not defined VCPKG_ROOT (
    echo [INFO] VCPKG_ROOT not set. Skipping automatic Assimp install.
    goto :eof
)
if not exist "%VCPKG_ROOT%\vcpkg.exe" (
    echo [INFO] vcpkg.exe not found at %VCPKG_ROOT%. Skipping automatic Assimp install.
    goto :eof
)
"%VCPKG_ROOT%\vcpkg.exe" install assimp:x64-windows
if errorlevel 1 (
    echo [WARNING] Failed to install Assimp via vcpkg. Please install manually.
) else (
    echo Assimp installed via vcpkg.
)
goto :eof

:install_oidn_vcpkg
if not defined VCPKG_ROOT (
    echo [INFO] VCPKG_ROOT not set. Skipping automatic OpenImageDenoise install.
    goto :eof
)
if not exist "%VCPKG_ROOT%\vcpkg.exe" (
    echo [INFO] vcpkg.exe not found at %VCPKG_ROOT%. Skipping automatic OpenImageDenoise install.
    goto :eof
)
"%VCPKG_ROOT%\vcpkg.exe" install openimagedenoise:x64-windows
if errorlevel 1 (
    echo [WARNING] Failed to install OpenImageDenoise via vcpkg. Please install manually.
) else (
    echo OpenImageDenoise installed via vcpkg.
)
goto :eof
