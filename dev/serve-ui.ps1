<#
.SYNOPSIS
    Start the SRS web console in Vite development mode.

.DESCRIPTION
    The dev server proxies /api to the SRS HTTP API (default 127.0.0.1:11985),
    so the console can be edited with hot reload while SRS keeps running.

    Examples:
      pwsh dev/serve-ui.ps1
      pwsh dev/serve-ui.ps1 -Api http://127.0.0.1:1985
#>
[CmdletBinding()]
param(
    [string]$Api = 'http://127.0.0.1:11985'
)

$ErrorActionPreference = 'Stop'
$repoRoot = (Resolve-Path (Join-Path $PSScriptRoot '..')).Path
$uiDir = Join-Path $repoRoot 'ui'

if (-not (Get-Command npm -ErrorAction SilentlyContinue)) {
    throw 'npm is required to run the web console.'
}

if (-not (Test-Path (Join-Path $uiDir 'node_modules'))) {
    Write-Host '==> npm install' -ForegroundColor Cyan
    Push-Location $uiDir
    try {
        npm install --no-audit --no-fund
        if ($LASTEXITCODE -ne 0) { throw 'npm install failed' }
    } finally {
        Pop-Location
    }
}

$env:SRS_API = $Api
Write-Host "==> SRS API proxy: $Api" -ForegroundColor Cyan
Write-Host '==> open http://127.0.0.1:5173/console/' -ForegroundColor Green

Push-Location $uiDir
try {
    npm run dev -- --host 127.0.0.1
} finally {
    Pop-Location
}
