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
set "BUILD_DIR=%ROOT_DIR%\build\sln"
set "BUILD_SLN=%BUILD_DIR%\SolarEngine.sln"
set "ROOT_SLN=%ROOT_DIR%\SolarEngine.sln"

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

copy /Y "%BUILD_SLN%" "%ROOT_SLN%" >nul
if errorlevel 1 (
    echo Error: failed to copy solution to project root.
    exit /b 1
)

set "SE_ROOT_SLN=%ROOT_SLN%"
powershell -NoProfile -ExecutionPolicy Bypass -Command ^
"$ErrorActionPreference='Stop'; $s=$env:SE_ROOT_SLN; $dq=[char]34; $c=Get-Content -LiteralPath $s -Raw; $p='('+$dq+'\s*,\s*'+$dq+')(?!build\\sln\\|[A-Za-z]:\\|\\\\|\.\.\\)([^'+$dq+']+\.vcxproj)('+$dq+'[,])'; $c=[regex]::Replace($c,$p,'$1build\sln\$2$3'); $p='('+$dq+'\s*,\s*'+$dq+')\.\.\\\.\.\\([^'+$dq+']+\.csproj)('+$dq+'[,])'; $c=[regex]::Replace($c,$p,'$1$2$3'); Set-Content -LiteralPath $s -Value $c -Encoding UTF8"
if errorlevel 1 (
    echo Error: failed to rewrite root solution project paths.
    exit /b 1
)

echo Generated:
echo   %ROOT_SLN%
echo Build files:
echo   %BUILD_DIR%
echo.
echo Open "%ROOT_SLN%" in Visual Studio.
exit /b 0
