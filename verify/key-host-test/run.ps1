# Builds and runs the host-side key behavior test with the CLion MinGW GCC.
param(
    [string]$Gcc = 'D:\Clion\Clionnn\CLion 2026.2.2\bin\mingw\bin\gcc.exe'
)

$ErrorActionPreference = 'Stop'

$testDir = Split-Path -Parent $MyInvocation.MyCommand.Path
$repoDir = (Resolve-Path (Join-Path $testDir '..\..')).Path
$buildDir = Join-Path $testDir 'build'

if (-not (Test-Path $Gcc)) {
    $Gcc = (Get-Command gcc -ErrorAction SilentlyContinue).Source
}
if (-not $Gcc) {
    throw 'No gcc found. Pass -Gcc <path to a host gcc.exe>.'
}

New-Item -ItemType Directory -Force -Path $buildDir | Out-Null

$sources = @(
    (Join-Path $repoDir 'Core\Src\key.c'),
    (Join-Path $repoDir 'Core\Src\control.c'),
    (Join-Path $repoDir 'Core\Src\pid.c'),
    (Join-Path $testDir 'hal_stub.c'),
    (Join-Path $testDir 'test_key_angle.c')
)

$exe = Join-Path $buildDir 'key-angle-test.exe'

& $Gcc -std=c11 -Wall -Wextra -O1 -o $exe `
    "-I" (Join-Path $repoDir 'Core\Inc') `
    "-I" (Join-Path $testDir 'stubs') `
    $sources

if ($LASTEXITCODE -ne 0) {
    throw "compile failed with exit code $LASTEXITCODE"
}

& $exe
exit $LASTEXITCODE
