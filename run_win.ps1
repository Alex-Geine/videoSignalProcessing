# run.ps1
param(
    [Parameter(Mandatory=$true)]
    [string]$TargetName
)

$SourceConfig = "config.yaml"
$DestDir = "build_msvc\bin\Debug"  # или "build\bin\Release", если используете Release
$DestConfig = Join-Path $DestDir "config.yaml"
$Executable = Join-Path $DestDir "$TargetName.exe"

# Создаём папку назначения, если её нет
if (!(Test-Path $DestDir)) {
    New-Item -ItemType Directory -Path $DestDir | Out-Null
}

# Копируем config.yaml
Copy-Item -Path $SourceConfig -Destination $DestConfig -Force

# Запускаем исполняемый файл
if (Test-Path $Executable) {
    & $Executable
} else {
    Write-Error "Executable not found: $Executable"
    exit 1
}