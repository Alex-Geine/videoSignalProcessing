param()

# build_win.ps1 — сборка с w64devkit без изменения системного PATH

$ScriptDir = Split-Path $MyInvocation.MyCommand.Path -Parent
. "$ScriptDir\setup-env.ps1"

$ProjectRoot = "D:\uni\videoSignalProcessing"
$BuildDir    = "$ProjectRoot\build"

Write-Host "Configuring CMake..." -ForegroundColor Cyan .
cmake -S "$ProjectRoot" -B "$BuildDir" -G "Unix Makefiles" -DCMAKE_BUILD_TYPE=Debug -DBUILD_WORKER_REAL=ON -DBUILD_POSTPROCESSOR_REAL=ON


Write-Host "Building progect..." -ForegroundColor Cyan
cmake --build "$BuildDir" -j 15 

if ($LASTEXITCODE -ne 0) {
    Write-Host "Error of building" -ForegroundColor Red
    exit 1
}

Write-Host "Building complete!" -ForegroundColor Green