@echo off
REM Run build and exe in MSYS2 MinGW64 bash with proper PATH. Input videos must be placed in 'vidglue' root folder.

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

REM Run MSYS2 bash with proper PATH and project folder, run bash script then cd into root folder.
"%MSYS2_BASH%" -lc "export PATH=/mingw64/bin:$PATH && cd $MSYS_PROJECT_DIR && ./build_dev.sh && cd ./.. && ./bin/vidglue.exe"

pause