@echo off
setlocal

rem Normalize repo root without trailing backslash
for %%i in ("%~dp0.") do set "SRC_DIR=%%~fi"
set "BUILD_DIR=%SRC_DIR%\build"
set "GENERATOR=Visual Studio 16 2019"

if not exist "%BUILD_DIR%" mkdir "%BUILD_DIR%"

cmake -S "%SRC_DIR%" -B "%BUILD_DIR%" -G "%GENERATOR%" -A x64 || goto :err
cmake --build "%BUILD_DIR%" --config Release || goto :err

echo Build complete. Exe is at %BUILD_DIR%\Release\zspace_web.exe
exit /b 0

:err
echo Build failed.
exit /b 1
