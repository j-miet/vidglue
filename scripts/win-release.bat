@echo off
REM One-click MSYS2 build for Windows

REM Path to MSYS2 bash
SET MSYS2_BASH=C:\msys64\usr\bin\bash.exe

REM Get folder of this batch file
SET PROJECT_DIR=%~dp0
REM Remove trailing backslash
IF "%PROJECT_DIR:~-1%"=="\" SET PROJECT_DIR=%PROJECT_DIR:~0,-1%

REM Convert Windows path C:\path\to\project -> /c/path/to/project
SET MSYS_PROJECT_DIR=%PROJECT_DIR%
SET MSYS_PROJECT_DIR=%MSYS_PROJECT_DIR::=%
SET MSYS_PROJECT_DIR=/%MSYS_PROJECT_DIR:\=/%

REM Run MSYS2 bash with proper PATH and project folder
"%MSYS2_BASH%" -lc "export PATH=/mingw64/bin:$PATH && cd $MSYS_PROJECT_DIR && ./win-release.sh"

pause