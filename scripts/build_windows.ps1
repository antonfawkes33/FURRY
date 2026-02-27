param(
    [ValidateSet("Debug", "Release")]
    [string]$Configuration = "Release"
)

$ErrorActionPreference = "Stop"

cmake -S . -B build -G "Visual Studio 17 2022" -A x64
cmake --build build --config $Configuration
ctest --test-dir build -C $Configuration --output-on-failure

Write-Host "[OK] Build and tests completed for $Configuration."
