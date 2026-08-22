[CmdletBinding()]
param(
    [Parameter(Position = 0)]
    [ValidateSet("All", "Engine", "Editor", "Tests", "App")]
    [string]$Target = "All",

    [Parameter(Position = 1)]
    [ValidateSet("Debug", "Release", "RelWithDebInfo")]
    [string]$Config = "Debug"
)

$ErrorActionPreference = "Stop"
$ScriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path
$RootDir = Split-Path -Parent $ScriptDir

Write-Host "==========================================================" -ForegroundColor Cyan
Write-Host "  Obscura Engine & Qt Editor CMake Build Orchestration" -ForegroundColor Cyan
Write-Host "  Target: $Target | Config: $Config" -ForegroundColor Cyan
Write-Host "  QT_SDK: $ENV:QT_SDK" -ForegroundColor Cyan
Write-Host "  VULKAN_SDK: $ENV:VULKAN_SDK" -ForegroundColor Cyan
Write-Host "==========================================================" -ForegroundColor Cyan

Push-Location $RootDir
try {
    Write-Host "`n[1/2] Configuring project with CMake..." -ForegroundColor Yellow
    & cmake -B Build
    if ($LASTEXITCODE -ne 0) {
        throw "CMake configuration failed with exit code $LASTEXITCODE"
    }

    Write-Host "`n[2/2] Building targets ($Config)..." -ForegroundColor Yellow
    if ($Target -eq "All") {
        & cmake --build Build --config $Config
    } elseif ($Target -eq "Engine") {
        & cmake --build Build --config $Config --target Obscura.Engine Obscura.Renderer Obscura.Scripting Obscura.TestPlugin
    } elseif ($Target -eq "Editor") {
        & cmake --build Build --config $Config --target ObscuraQtEditor
    } elseif ($Target -eq "Tests") {
        & cmake --build Build --config $Config --target Obscura.Tests
    } elseif ($Target -eq "App") {
        & cmake --build Build --config $Config --target Obscura.App
    }

    if ($LASTEXITCODE -ne 0) {
        throw "CMake build failed with exit code $LASTEXITCODE"
    }
}
finally {
    Pop-Location
}

Write-Host "`n==========================================================" -ForegroundColor Green
Write-Host "  Build Completed Successfully!" -ForegroundColor Green
Write-Host "  Binaries directory: Build\bin\$Config\" -ForegroundColor Green
Write-Host "==========================================================" -ForegroundColor Green
