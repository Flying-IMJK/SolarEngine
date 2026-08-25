@echo off
setlocal EnableExtensions

set "ROOT_DIR=%~dp0"
if "%ROOT_DIR:~-1%"=="\" set "ROOT_DIR=%ROOT_DIR:~0,-1%"

set "CONFIG=%~1"
if "%CONFIG%"=="" set "CONFIG=Debug"

if /I "%CONFIG%"=="Debug" goto config_ok
if /I "%CONFIG%"=="Release" goto config_ok

echo Usage: %~nx0 [Debug^|Release]
exit /b 1

:config_ok
set "BUILD_DIR=%ROOT_DIR%\Sln"
set "BUILD_SLN=%BUILD_DIR%\SolarEngine.sln"

where cmake >nul 2>nul
if errorlevel 1 (
    echo Error: cmake was not found in PATH.
    exit /b 1
)

echo Generating SolarEngine %CONFIG% solution...
cmake -S "%ROOT_DIR%" -B "%BUILD_DIR%" ^
    -G "Visual Studio 17 2022" ^
    -A x64 ^
    -T host=x64 ^
    -DCMAKE_BUILD_TYPE=%CONFIG% ^
    -DCMAKE_CONFIGURATION_TYPES=%CONFIG%
if errorlevel 1 (
    echo Error: CMake generation failed.
    exit /b 1
)

if not exist "%BUILD_SLN%" (
    echo Error: generated solution was not found:
    echo   "%BUILD_SLN%"
    exit /b 1
)

echo Generated:
echo   %BUILD_SLN%
echo Build files:
echo   %BUILD_DIR%
echo.
echo Open "%BUILD_SLN%" in Visual Studio.
exit /b 0
