<#
.SYNOPSIS
    Quick local build/check loop for SRS-Win.

.DESCRIPTION
    Rebuilds only the executable and the event-loop test (no clean), then runs
    the test. Use -Full for the vcpkg RTC/SRT configuration, or -Docs to
    refresh the API reference too.

    Examples:
      pwsh dev/check.ps1
      pwsh dev/check.ps1 -Full
      pwsh dev/check.ps1 -Docs
      pwsh dev/check.ps1 -NoTests
#>
[CmdletBinding()]
param(
    [switch]$Full,
    [switch]$Docs,
    [switch]$NoTests
)

$ErrorActionPreference = 'Stop'
$repoRoot = (Resolve-Path (Join-Path $PSScriptRoot '..')).Path
$sourceDir = Join-Path $repoRoot 'trunk/ide/srs_msvc'
$buildDir = if ($Full) { Join-Path $sourceDir 'build-full' } else { Join-Path $sourceDir 'build' }
$stopwatch = [Diagnostics.Stopwatch]::StartNew()

function Step([string]$message) {
    Write-Host "==> $message" -ForegroundColor Cyan
}

function Invoke-Checked([scriptblock]$action, [string]$what) {
    & $action
    if ($LASTEXITCODE -ne 0) {
        throw "$what failed with exit $LASTEXITCODE"
    }
}

if ($Full -and -not $env:VCPKG_ROOT) {
    throw 'VCPKG_ROOT is not set; set it to the vcpkg installation (for example H:\vcpkg) for -Full.'
}

if (-not (Test-Path (Join-Path $buildDir 'CMakeCache.txt'))) {
    Step "configure $buildDir"
    if ($Full) {
        Push-Location $sourceDir
        try {
            Invoke-Checked { cmake --preset windows-full } 'cmake --preset windows-full'
        } finally {
            Pop-Location
        }
    } else {
        Invoke-Checked { cmake -B $buildDir -S $sourceDir -G 'Visual Studio 17 2022' -A x64 } 'cmake configure'
    }
}

if ($Full) {
    Step 'build srs + st-event-loop-test (windows-full)'
    Push-Location $sourceDir
    try {
        Invoke-Checked { cmake --build --preset windows-full-debug --target srs st-event-loop-test -- /m:2 } 'cmake build'
    } finally {
        Pop-Location
    }
} else {
    Step 'build srs + st-event-loop-test (windows-default)'
    Invoke-Checked {
        cmake --build $buildDir --config Debug --target srs st-event-loop-test -- /m:2
    } 'cmake build'
}

if (-not $NoTests) {
    Step 'run st-event-loop-test'
    if ($Full) {
        Push-Location $sourceDir
        try {
            Invoke-Checked { ctest --preset windows-full-test } 'ctest'
        } finally {
            Pop-Location
        }
    } else {
        Invoke-Checked { ctest --test-dir $buildDir -C Debug -R st-event-loop-test --output-on-failure } 'ctest'
    }
}

if ($Docs) {
    & (Join-Path $PSScriptRoot 'build-docs.ps1')
}

$stopwatch.Stop()
Write-Host ("check OK: {0:N1}s ({1})" -f $stopwatch.Elapsed.TotalSeconds, $(if ($Full) { 'full' } else { 'default' })) -ForegroundColor Green
