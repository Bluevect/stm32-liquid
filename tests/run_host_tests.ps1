# Host-side test / preview runner (Windows PowerShell).
#
# Usage (from the project root):
#   powershell -ExecutionPolicy Bypass -File tests/run_host_tests.ps1
#
# Builds the firmware-shared sources (App/Src/liquid.c + Drivers/BSP/Src/bsp_oled.c)
# for the PC, runs the behaviour tests and renders the liquid preview frames.

$ErrorActionPreference = 'Stop'
$root = Split-Path -Parent $PSScriptRoot
Set-Location $root

$common = @(
    '-std=c11', '-O1', '-Wall', '-Wextra',
    '-IApp/Inc', '-IDrivers/BSP/Inc', '-IDrivers/Platform/Inc', '-Itests/host_hal'
)

function Build-SharedSources {
    param([string]$Output, [string[]]$Sources)
    & gcc @common @Sources '-lm' '-o' $Output
    if ($LASTEXITCODE -ne 0) { throw "gcc failed for $Output" }
}

Write-Output '=== build host tools ==='
Build-SharedSources -Output 'build/liquid_test.exe' -Sources @(
    'tests/liquid_test.c', 'tests/host_hal_stub.c', 'App/Src/liquid.c', 'Drivers/BSP/Src/bsp_oled.c')
Build-SharedSources -Output 'build/preview.exe' -Sources @(
    'tests/preview.c', 'tests/host_hal_stub.c', 'App/Src/liquid.c', 'Drivers/BSP/Src/bsp_oled.c')
Build-SharedSources -Output 'build/liquid_probe.exe' -Sources @(
    'tests/liquid_probe.c', 'tests/host_hal_stub.c', 'App/Src/liquid.c', 'Drivers/BSP/Src/bsp_oled.c')

Write-Output '=== run liquid behaviour tests ==='
& ./build/liquid_test.exe
if ($LASTEXITCODE -ne 0) { throw 'liquid_test failed' }

Write-Output '=== render preview frames ==='
New-Item -ItemType Directory -Force -Path 'build/preview/anim' | Out-Null
& ./build/preview.exe contact
& ./build/preview.exe anim

Write-Output '=== liquid probe (30 deg tilt, 2500 steps) ==='
& ./build/liquid_probe.exe 0.5 0.87 2500

function Resolve-Python {
    $candidates = @()
    if ($env:PYTHON) { $candidates += $env:PYTHON }
    $candidates += 'python', 'python3', 'py'

    foreach ($cmd in $candidates) {
        $exe = $null
        if (Test-Path -LiteralPath $cmd) {
            $exe = $cmd
        } else {
            $found = Get-Command $cmd -ErrorAction SilentlyContinue
            if ($found) { $exe = $found.Source }
        }
        if (-not $exe) { continue }

        & $exe -c 'import PIL' 2>$null
        if ($LASTEXITCODE -eq 0) { return $exe }
    }

    throw 'Python with Pillow not found. Set $env:PYTHON, or run: pip install pillow'
}

$python = Resolve-Python
Write-Output "=== using Python: $python ==="

Write-Output '=== preview images ==='
& $python tests/make_preview.py anim
& $python tests/make_preview.py contact

Write-Output '=== host tests done ==='
