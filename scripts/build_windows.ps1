param(
    [ValidateSet("Debug", "Release")]
    [string]$Configuration = "Release",
    [string]$BuildDir = "build",
    [string[]]$RuntimeDlls = @()
)

$ErrorActionPreference = "Stop"

Get-Process furry_app,test_furry -ErrorAction SilentlyContinue | Stop-Process -Force -ErrorAction SilentlyContinue

$vsWhere = Join-Path ${env:ProgramFiles(x86)} "Microsoft Visual Studio\Installer\vswhere.exe"
$generator = $null
$archArgs = @("-A", "x64")

if (Test-Path $vsWhere) {
    $version = & $vsWhere -latest -products * -requires Microsoft.Component.MSBuild -property installationVersion
    if ($LASTEXITCODE -eq 0 -and $version) {
        if ($version.StartsWith("17.")) {
            $generator = "Visual Studio 17 2022"
        } elseif ($version.StartsWith("16.")) {
            $generator = "Visual Studio 16 2019"
        }
    }
}

if (-not $generator) {
    $generator = "Ninja"
    $archArgs = @()
}

cmake -S . -B $BuildDir -G $generator @archArgs
cmake --build $BuildDir --config $Configuration
ctest --test-dir $BuildDir -C $Configuration --output-on-failure

$outDir = Join-Path $BuildDir "bin"
if (-not (Test-Path $outDir)) {
    New-Item -ItemType Directory -Path $outDir | Out-Null
}

$allDlls = @()
if ($env:FURRY_DLLS) {
    $allDlls += ($env:FURRY_DLLS -split ';')
}
$allDlls += $RuntimeDlls

foreach ($dll in $allDlls | Where-Object { $_ -and $_.Trim() }) {
    if (Test-Path $dll) {
        Copy-Item -Path $dll -Destination $outDir -Force
    } else {
        Write-Warning "DLL not found: $dll"
    }
}

Write-Host "[OK] Build/tests completed for $Configuration using generator '$generator'."
