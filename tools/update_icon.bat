@echo off
echo Updating Glint3D application icon...
echo.

echo 1. Converting PNG to ICO format...
python create_basic_ico.py
if %errorlevel% neq 0 (
    echo Failed to convert PNG to ICO
    pause
    exit /b 1
)

echo 2. Rebuilding application with new icon...
cd ..
cmake --build builds/desktop/cmake --config Release
if %errorlevel% neq 0 (
    echo Build failed
    pause
    exit /b 1
)

echo.
echo Success! The application icon has been updated.
echo Check builds/desktop/cmake/Release/glint.exe in Windows Explorer to verify.
echo.
pause