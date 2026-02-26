@echo off
REM Glint3D Documentation Generation Script (Windows)
REM Generates API documentation using Doxygen

echo ============================================
echo Glint3D Documentation Generator
echo ============================================
echo.

REM Check if Doxygen is installed
where doxygen >nul 2>nul
if %ERRORLEVEL% NEQ 0 (
    echo [ERROR] Doxygen not found in PATH
    echo Please install Doxygen from https://www.doxygen.nl/download.html
    exit /b 1
)

REM Change to repository root (parent directory)
cd /d "%~dp0.."

REM Check if Doxyfile exists
if not exist "Doxyfile" (
    echo [ERROR] Doxyfile not found in repository root
    echo Please ensure Doxyfile exists in the repository root
    exit /b 1
)

REM Generate task module documentation (optional - requires Python)
echo [INFO] Generating task module documentation...
where python >nul 2>nul
if %ERRORLEVEL% EQU 0 (
    python tools\generate-task-docs.py
    if %ERRORLEVEL% NEQ 0 (
        echo [WARNING] Task documentation generation failed, continuing with Doxygen...
    )
) else (
    echo [INFO] Python not found, skipping task documentation generation
    echo [INFO] Install Python to enable automatic task docs: https://www.python.org/
)
echo.

REM Generate documentation
echo [INFO] Generating API documentation...
echo.
doxygen Doxyfile

if %ERRORLEVEL% EQU 0 (
    echo.
    echo ============================================
    echo [SUCCESS] Documentation generated successfully!
    echo Output: docs\api\html\index.html
    echo ============================================
    echo.
    echo Open docs\api\html\index.html in your browser to view the documentation
) else (
    echo.
    echo [ERROR] Documentation generation failed
    exit /b 1
)