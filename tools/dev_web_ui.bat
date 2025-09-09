@echo off
REM Wrapper to run the PowerShell web refresh script when 'pwsh' is unavailable
REM Usage: tools\dev_web_ui.bat [args...]
REM Examples:
REM   tools\dev_web_ui.bat -EmsdkRoot "D:\Graphics\emsdk"
REM   tools\dev_web_ui.bat -Config Debug -Clean

setlocal
set SCRIPT=tools\dev_web_ui.ps1

if not exist "%SCRIPT%" (
  echo ERROR: Cannot find %SCRIPT%
  exit /b 1
)

REM Use Windows PowerShell (powershell.exe) with relaxed policy for this process
powershell -NoProfile -ExecutionPolicy Bypass -File "%SCRIPT%" %*

endlocal
