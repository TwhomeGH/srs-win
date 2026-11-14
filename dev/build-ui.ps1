<#
.SYNOPSIS
    Build the SRS web console and install it into the SRS HTTP API console path.

.DESCRIPTION
    The console is a Vite/React app under ui/. It talks to /api/v1 on the same
    origin, so the built files must live under objs/nginx/html/console, which
    is what SrsHttpApi mounts at /console.

    Examples:
      pwsh dev/build-ui.ps1
      pwsh dev/build-ui.ps1 -Target full
      pwsh dev/build-ui.ps1 -Target both -NoInstall
#>
[CmdletBinding()]
param(
    [ValidateSet('default', 'full', 'both')]
    [string]$Target = 'both',
    [switch]$NoInstall
)

$ErrorActionPreference = 'Stop'
$repoRoot = (Resolve-Path (Join-Path $PSScriptRoot '..')).Path
$uiDir = Join-Path $repoRoot 'ui'
$stopwatch = [Diagnostics.Stopwatch]::StartNew()

function Step([string]$message) {
    Write-Host "==> $message" -ForegroundColor Cyan
}

if (-not (Get-Command npm -ErrorAction SilentlyContinue)) {
    throw 'npm is required to build the web console.'
}

Push-Location $uiDir
try {
    if (-not $NoInstall -and -not (Test-Path (Join-Path $uiDir 'node_modules'))) {
        Step 'npm install'
        & npm install --no-audit --no-fund
        if ($LASTEXITCODE -ne 0) { throw 'npm install failed' }
    }

    Step 'npm run build'
    & npm run build
    if ($LASTEXITCODE -ne 0) { throw 'npm run build failed' }
} finally {
    Pop-Location
}

$buildRoots = @()
if ($Target -eq 'default' -or $Target -eq 'both') {
    $buildRoots += Join-Path $repoRoot 'trunk/ide/srs_msvc/build/Debug'
}
if ($Target -eq 'full' -or $Target -eq 'both') {
    $buildRoots += Join-Path $repoRoot 'trunk/ide/srs_msvc/build-full/Debug'
}

foreach ($root in $buildRoots) {
    $dst = Join-Path $root 'objs/nginx/html/console'
    New-Item -ItemType Directory $dst -Force | Out-Null
    Get-ChildItem $dst -Force | Remove-Item -Recurse -Force -ErrorAction SilentlyContinue
    Copy-Item (Join-Path $uiDir 'build/*') $dst -Recurse -Force
    Write-Host "installed console: $dst" -ForegroundColor Green
}

$stopwatch.Stop()
Write-Host ("web console build OK: {0:N1}s" -f $stopwatch.Elapsed.TotalSeconds) -ForegroundColor Green
Write-Host 'Open http://127.0.0.1:11985/console/ (default build) or 11985 for the full build.'
