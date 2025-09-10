@echo off 
REM Glint3D wrapper script 
set "GLINT_ROOT=D:\class\Glint3D\" 
 
REM Change to Glint root directory so asset paths work 
cd /d "D:\class\Glint3D\" 
 
REM Try release build first, fallback to debug 
if exist "D:\class\Glint3D\builds\desktop\cmake\Release\glint.exe" ( 
    "D:\class\Glint3D\builds\desktop\cmake\Release\glint.exe" %* 
) else if exist "D:\class\Glint3D\builds\desktop\cmake\Debug\glint.exe" ( 
    "D:\class\Glint3D\builds\desktop\cmake\Debug\glint.exe" %* 
) else ( 
    echo ERROR: No Glint executable found. Please build the project first. 
    exit /b 1 
) 
