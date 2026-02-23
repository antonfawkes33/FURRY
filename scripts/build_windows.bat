@echo off
setlocal

if "%~1"=="" (
  set BUILD_TYPE=Release
) else (
  set BUILD_TYPE=%~1
)

where cmake >nul 2>&1
if errorlevel 1 (
  echo [ERROR] cmake not found in PATH.
  exit /b 1
)

cmake -S . -B build -G "Visual Studio 17 2022" -A x64
if errorlevel 1 exit /b 1

cmake --build build --config %BUILD_TYPE%
if errorlevel 1 exit /b 1

ctest --test-dir build -C %BUILD_TYPE% --output-on-failure
if errorlevel 1 exit /b 1

echo [OK] Build and tests completed for %BUILD_TYPE%.
