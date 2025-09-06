@echo off
echo Testing Path Security Build Fix
echo ===============================

echo.
echo Step 1: Testing standalone compilation
echo --------------------------------------
g++ -std=c++17 -I. test_path_security_build.cpp engine/src/path_security.cpp -o test_path_security_build.exe
if %ERRORLEVEL% neq 0 (
    echo ❌ Standalone compilation failed
    exit /b 1
)

echo ✅ Standalone compilation successful

echo.
echo Step 2: Running path security tests
echo -----------------------------------
test_path_security_build.exe
if %ERRORLEVEL% neq 0 (
    echo ❌ Path security tests failed
    exit /b 1
)

echo.
echo Step 3: Testing CMake build
echo ---------------------------
if not exist "builds\desktop\cmake" mkdir builds\desktop\cmake
cd builds\desktop\cmake
cmake -S ..\..\.. -DCMAKE_BUILD_TYPE=Release
if %ERRORLEVEL% neq 0 (
    echo ❌ CMake configure failed
    cd ..\..\..
    exit /b 1
)

cmake --build . --config Release -j
if %ERRORLEVEL% neq 0 (
    echo ❌ CMake build failed
    cd ..\..\..
    exit /b 1
)

cd ..\..\..
echo ✅ CMake build successful

echo.
echo Step 4: Testing basic engine functionality
echo ------------------------------------------
if exist "builds\desktop\cmake\Release\glint.exe" (
    echo ✅ Engine binary created: builds\desktop\cmake\Release\glint.exe
) else if exist "builds\desktop\cmake\glint.exe" (
    echo ✅ Engine binary created: builds\desktop\cmake\glint.exe
) else (
    echo ❌ Engine binary not found
    exit /b 1
)

echo.
echo 🎉 All tests passed! Build fix is successful.
echo The --asset-root implementation should now work in CI.

pause