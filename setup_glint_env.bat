@echo off
REM Setup script to add Glint3D to PATH and create environment variables
REM Run this script as Administrator for system-wide installation

set "GLINT_ROOT=%~dp0"
set "GLINT_RELEASE_EXE=%GLINT_ROOT%builds\desktop\cmake\Release\glint.exe"
set "GLINT_DEBUG_EXE=%GLINT_ROOT%builds\desktop\cmake\Debug\glint.exe"

echo Setting up Glint3D environment...
echo.
echo Glint Root: %GLINT_ROOT%
echo Release Exe: %GLINT_RELEASE_EXE%
echo Debug Exe: %GLINT_DEBUG_EXE%
echo.

REM Check if executables exist
if not exist "%GLINT_RELEASE_EXE%" (
    echo WARNING: Release executable not found at %GLINT_RELEASE_EXE%
    echo Please build the project first with: cmake --build builds/desktop/cmake -j
    echo.
)

if not exist "%GLINT_DEBUG_EXE%" (
    echo WARNING: Debug executable not found at %GLINT_DEBUG_EXE%
    echo.
)

REM Create glint.bat wrapper script in the root directory
echo @echo off > "%GLINT_ROOT%glint.bat"
echo REM Glint3D wrapper script >> "%GLINT_ROOT%glint.bat"
echo set "GLINT_ROOT=%GLINT_ROOT%" >> "%GLINT_ROOT%glint.bat"
echo. >> "%GLINT_ROOT%glint.bat"
echo REM Change to Glint root directory so asset paths work >> "%GLINT_ROOT%glint.bat"
echo cd /d "%GLINT_ROOT%" >> "%GLINT_ROOT%glint.bat"
echo. >> "%GLINT_ROOT%glint.bat"
echo REM Try release build first, fallback to debug >> "%GLINT_ROOT%glint.bat"
echo if exist "%GLINT_RELEASE_EXE%" ( >> "%GLINT_ROOT%glint.bat"
echo     "%GLINT_RELEASE_EXE%" %%* >> "%GLINT_ROOT%glint.bat"
echo ^) else if exist "%GLINT_DEBUG_EXE%" ( >> "%GLINT_ROOT%glint.bat"
echo     "%GLINT_DEBUG_EXE%" %%* >> "%GLINT_ROOT%glint.bat"
echo ^) else ( >> "%GLINT_ROOT%glint.bat"
echo     echo ERROR: No Glint executable found. Please build the project first. >> "%GLINT_ROOT%glint.bat"
echo     exit /b 1 >> "%GLINT_ROOT%glint.bat"
echo ^) >> "%GLINT_ROOT%glint.bat"

echo Created wrapper script: %GLINT_ROOT%glint.bat
echo.

REM Add to PATH for current session
set "PATH=%GLINT_ROOT%;%PATH%"
echo Added %GLINT_ROOT% to PATH for current session.
echo.

echo To make this permanent, you have two options:
echo.
echo 1. Manual setup (recommended):
echo    - Add "%GLINT_ROOT%" to your system PATH environment variable
echo    - Or add "%GLINT_ROOT%" to your user PATH environment variable
echo.
echo 2. Automatic setup (requires Administrator):
echo    - Run this script as Administrator
echo    - Uncomment the setx commands below
echo.

REM Uncomment these lines to automatically add to system PATH (requires Administrator)
REM setx /M PATH "%GLINT_ROOT%;%PATH%"
REM echo Added to system PATH permanently.

echo.
echo Setup complete! You can now use 'glint' from any directory.
echo Example: glint --ops examples/json-ops/sphere_basic.json --render output.png --w 512 --h 512
echo.
pause