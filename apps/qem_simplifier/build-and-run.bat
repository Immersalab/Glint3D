REM Machine Summary Block
REM {"file":"apps/qem_simplifier/build-and-run.bat","purpose":"Builds and runs the standalone QEM scaffold targets with fast local iteration modes.","exports":[],"depends_on":["cmake","apps/qem_simplifier/CMakeLists.txt"],"notes":["modes_configure_build_run_smoke_clean","standalone_qem_loop"]}
REM Human Summary
REM Windows helper for the QEM tool scaffold supporting configure/build/run/smoke/clean without rebuilding the full Glint app.
@echo off
setlocal enabledelayedexpansion

for %%I in ("%~dp0..\..") do set "REPO_ROOT=%%~fI"
pushd "%REPO_ROOT%" >nul

set "CONFIG=Debug"
set "MODE=run"
set "RUN_ARGS="

:parse_args
if "%~1"=="" goto args_done
if /i "%~1"=="debug" (set "CONFIG=Debug" & shift & goto parse_args)
if /i "%~1"=="release" (set "CONFIG=Release" & shift & goto parse_args)
if /i "%~1"=="configure" (set "MODE=configure" & shift & goto parse_args)
if /i "%~1"=="build" (set "MODE=build" & shift & goto parse_args)
if /i "%~1"=="run" (set "MODE=run" & shift & goto parse_args)
if /i "%~1"=="smoke" (set "MODE=smoke" & shift & goto parse_args)
if /i "%~1"=="clean" (set "MODE=clean" & shift & goto parse_args)
set "RUN_ARGS=!RUN_ARGS! %1"
shift
goto parse_args

:args_done
set "BUILD_DIR=builds\qem_simplifier\cmake"
set "EXE="

echo.
echo ========================================
echo  QEM Simplifier Build and Run Script
echo ========================================
echo Mode: %MODE%
echo Config: %CONFIG%
echo Repo: %REPO_ROOT%
echo.

if /i "%MODE%"=="clean" goto do_clean
if /i "%MODE%"=="configure" goto do_configure
if /i "%MODE%"=="build" goto do_build
if /i "%MODE%"=="run" goto do_run
if /i "%MODE%"=="smoke" goto do_smoke

echo ERROR: Unknown mode %MODE%
exit /b 1

:do_clean
if exist "%BUILD_DIR%" (
  echo Removing %BUILD_DIR% ...
  rmdir /S /Q "%BUILD_DIR%"
) else (
  echo Build directory does not exist: %BUILD_DIR%
)
exit /b 0

:do_configure
echo [1/1] Configuring standalone QEM scaffold...
cmake -S apps/qem_simplifier -B "%BUILD_DIR%"
if errorlevel 1 exit /b 1
exit /b 0

:do_build
call :configure_if_needed
if errorlevel 1 exit /b 1
echo [1/1] Building %CONFIG%...
cmake --build "%BUILD_DIR%" --config %CONFIG% -j
if errorlevel 1 exit /b 1
exit /b 0

:do_run
call :build_all
if errorlevel 1 exit /b 1
call :resolve_exe %CONFIG%
if errorlevel 1 exit /b 1
echo Launching: %EXE%
"%EXE%" --startup-clear-help %RUN_ARGS%
exit /b %ERRORLEVEL%

:do_smoke
call :build_all
if errorlevel 1 exit /b 1
call :resolve_exe %CONFIG%
if errorlevel 1 exit /b 1
echo Running smoke test (help/status/quit)...
(
  echo help
  echo status
  echo quit
) | "%EXE%"
exit /b %ERRORLEVEL%

:configure_if_needed
if not exist "%BUILD_DIR%\CMakeCache.txt" (
  echo [1/2] Configuring standalone QEM scaffold...
  cmake -S apps/qem_simplifier -B "%BUILD_DIR%"
  if errorlevel 1 exit /b 1
)
exit /b 0

:build_all
call :configure_if_needed
if errorlevel 1 exit /b 1
echo [2/2] Building %CONFIG%...
cmake --build "%BUILD_DIR%" --config %CONFIG% -j
if errorlevel 1 exit /b 1
exit /b 0

:resolve_exe
set "RESOLVE_CONFIG=%~1"
for %%P in (
  "%BUILD_DIR%\%RESOLVE_CONFIG%\glint_qem_studio_shell.exe"
  "%BUILD_DIR%\Release\glint_qem_studio_shell.exe"
  "%BUILD_DIR%\RelWithDebInfo\glint_qem_studio_shell.exe"
  "%BUILD_DIR%\MinSizeRel\glint_qem_studio_shell.exe"
  "%BUILD_DIR%\Debug\glint_qem_studio_shell.exe"
  "%BUILD_DIR%\glint_qem_studio_shell.exe"
) do (
  if exist "%%~P" (
    set "EXE=%%~fP"
    exit /b 0
  )
)
echo ERROR: glint_qem_studio_shell.exe not found under %BUILD_DIR%
exit /b 1
