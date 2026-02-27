@echo off
setlocal

if "%~1"=="" (
  set BUILD_TYPE=Release
) else (
  set BUILD_TYPE=%~1
)

if "%~2"=="" (
  set BUILD_DIR=build
) else (
  set BUILD_DIR=%~2
)

where cmake >nul 2>&1
if errorlevel 1 (
  echo [ERROR] cmake not found in PATH.
  exit /b 1
)

for %%P in (furry_app.exe test_furry.exe) do (
  taskkill /IM %%P /F >nul 2>&1
)

set CMAKE_GEN=
set CMAKE_ARCH=-A x64

if exist "%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe" (
  for /f "usebackq delims=" %%I in (`"%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe" -latest -products * -requires Microsoft.Component.MSBuild -property installationVersion`) do set VSVER=%%I
)

if defined VSVER (
  echo %VSVER% | findstr /b "17." >nul && set CMAKE_GEN=Visual Studio 17 2022
  if not defined CMAKE_GEN (
    echo %VSVER% | findstr /b "16." >nul && set CMAKE_GEN=Visual Studio 16 2019
  )
)

if not defined CMAKE_GEN (
  set CMAKE_GEN=Ninja
  set CMAKE_ARCH=
)

cmake -S . -B %BUILD_DIR% -G "%CMAKE_GEN%" %CMAKE_ARCH%
if errorlevel 1 exit /b 1

cmake --build %BUILD_DIR% --config %BUILD_TYPE%
if errorlevel 1 exit /b 1

ctest --test-dir %BUILD_DIR% -C %BUILD_TYPE% --output-on-failure
if errorlevel 1 exit /b 1

if defined FURRY_DLLS (
  for %%D in (%FURRY_DLLS%) do (
    copy /Y "%%~D" "%BUILD_DIR%\bin\" >nul
  )
)

echo [OK] Build and tests completed for %BUILD_TYPE% using generator "%CMAKE_GEN%".
