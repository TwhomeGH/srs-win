<#
.SYNOPSIS
    Build the local API reference (Doxygen) into _site/api.

.DESCRIPTION
    Uses a local doxygen if available; otherwise runs Doxygen in a disposable
    Ubuntu container (same version family as .github/workflows/pages.yml).
    The output is also what the Pages workflow publishes under /api/.

    Examples:
      pwsh dev/build-docs.ps1
      pwsh dev/build-docs.ps1 -Open
      pwsh dev/build-docs.ps1 -ForceDocker
#>
[CmdletBinding()]
param(
    [switch]$Open,
    [switch]$ForceDocker
)

$ErrorActionPreference = 'Stop'
$repoRoot = (Resolve-Path (Join-Path $PSScriptRoot '..')).Path
$doxyfile = Join-Path $repoRoot 'Doxyfile'
$index = Join-Path $repoRoot '_site/api/index.html'

function Step([string]$message) {
    Write-Host "==> $message" -ForegroundColor Cyan
}

if (-not (Test-Path $doxyfile)) {
    throw "Doxyfile not found: $doxyfile"
}

$doxygen = if (-not $ForceDocker) { Get-Command doxygen -ErrorAction SilentlyContinue } else { $null }
$stopwatch = [Diagnostics.Stopwatch]::StartNew()

if ($doxygen) {
    Step "doxygen $($doxygen.Version)"
    Push-Location $repoRoot
    try {
        & $doxygen.Source Doxyfile
        if ($LASTEXITCODE -ne 0) { throw "doxygen failed with exit $LASTEXITCODE" }
    } finally {
        Pop-Location
    }
} else {
    $docker = Get-Command docker -ErrorAction SilentlyContinue
    if (-not $docker) {
        throw 'doxygen is not installed and docker is unavailable. Install Doxygen or Docker Desktop.'
    }

    Step 'doxygen via ubuntu:24.04 container'
    & docker run --rm `
        -v "${repoRoot}:/src" `
        -w /src `
        ubuntu:24.04 `
        bash -lc 'apt-get update -qq >/dev/null && apt-get install -y -qq doxygen >/dev/null && doxygen Doxyfile'
    if ($LASTEXITCODE -ne 0) { throw "dockerized doxygen failed with exit $LASTEXITCODE" }
}

$stopwatch.Stop()

if (-not (Test-Path $index)) {
    throw "Doxygen finished but $index is missing"
}

$size = (Get-Item $index).Length
Write-Host ("API reference ready: {0} ({1:N0} bytes, {2:N1}s)" -f $index, $size, $stopwatch.Elapsed.TotalSeconds) -ForegroundColor Green
Write-Host "warnings: $repoRoot\doxygen-warnings.log (if any)"

if ($Open) {
    Start-Process $index
}
