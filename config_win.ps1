# setup-env.ps1
# Настройка временного PATH для сборки проекта

# --- Укажите здесь свои абсолютные пути ---
$ProjectRoot = "D:\uni\videoSignalProcessing"

$W64DevkitBin = Join-Path $ProjectRoot "ext\w64devkit-1.16.1\w64devkit\bin"
$CMakeBin     = "D:\CMake\bin"
$GitBin       = "D:\Git\bin"  # или где у вас git

# --- Проверка существования путей ---
if (!(Test-Path $W64DevkitBin)) { throw "w64devkit bin not found: $W64DevkitBin" }
if (!(Test-Path $CMakeBin))     { throw "CMake bin not found: $CMakeBin" }
if (!(Test-Path $GitBin))       { Write-Warning "Git bin not found: $GitBin (optional)" }

# --- Формируем новый PATH (w64devkit должен быть первым!) ---
$env:PATH = "$W64DevkitBin;$CMakeBin;$GitBin;$env:PATH"

Write-Host "✅ Временный PATH настроен:" -ForegroundColor Green
Write-Host "   w64devkit: $W64DevkitBin"
Write-Host "   CMake:     $CMakeBin"
Write-Host "   Git:       $GitBin"