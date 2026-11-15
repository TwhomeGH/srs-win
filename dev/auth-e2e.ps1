<#
.SYNOPSIS
    Run the console login/token E2E against an SRS build with Basic auth enabled.

.DESCRIPTION
    Uses dev/dev-auth.ps1 to provide a stable development password, starts SRS
    with dev/windows/auth.conf, then verifies:
      - the login form obtains a bearer token
      - reload reuses the token
      - /api/v1/console/refresh rotates it
      - logout returns to the login form

.EXAMPLE
    pwsh dev/auth-e2e.ps1 `
      -SrsExe trunk/ide/srs_msvc/build-full/Debug/srs.exe `
      -WorkDir trunk/ide/srs_msvc/build-full/Debug
#>
[CmdletBinding()]
param(
    [Parameter(Mandatory = $true)]
    [string]$SrsExe,

    [Parameter(Mandatory = $true)]
    [string]$WorkDir,

    [string]$ConfFile = 'dev/windows/auth.conf',
    [int]$ApiPort = 11985
)

$ErrorActionPreference = 'Stop'

$SrsExe = (Resolve-Path $SrsExe).Path
$ConfFile = (Resolve-Path $ConfFile).Path
$WorkDir = (Resolve-Path $WorkDir).Path

$consoleIndex = Join-Path $WorkDir 'objs/nginx/html/console/index.html'
if (-not (Test-Path $consoleIndex)) {
    throw "console build not found at $consoleIndex; run dev/build-ui.ps1 -Target full first"
}

. (Join-Path $PSScriptRoot 'dev-auth.ps1')
$env:SRS_AUTH_USERNAME = $env:SRS_HTTP_API_AUTH_USERNAME
$env:SRS_AUTH_PASSWORD = $env:SRS_HTTP_API_AUTH_PASSWORD
$env:SRS_BASE = "http://127.0.0.1:$ApiPort"

$objs = Join-Path $WorkDir 'objs'
New-Item -ItemType Directory -Path $objs -Force | Out-Null
Remove-Item -Path (Join-Path $objs 'srs.pid') -ErrorAction SilentlyContinue

Write-Host "auth-e2e: exe=$SrsExe conf=$ConfFile cwd=$WorkDir"
$proc = Start-Process -FilePath $SrsExe `
    -ArgumentList @('-c', $ConfFile) `
    -WorkingDirectory $WorkDir `
    -NoNewWindow -PassThru

try {
    $ready = $false
    $headers = @{ Authorization = 'Basic ' + [Convert]::ToBase64String([Text.Encoding]::UTF8.GetBytes("$env:SRS_AUTH_USERNAME`:$env:SRS_AUTH_PASSWORD")) }
    for ($i = 0; $i -lt 40; $i++) {
        Start-Sleep -Milliseconds 500
        $proc.Refresh()
        if ($proc.HasExited) { throw "SRS exited before API was ready, code=$($proc.ExitCode)" }
        try {
            $null = Invoke-RestMethod -Uri "http://127.0.0.1:$ApiPort/api/v1/versions" -Headers $headers -TimeoutSec 2
            $ready = $true
            break
        } catch {
            continue
        }
    }
    if (-not $ready) { throw "SRS API did not become ready with configured credentials" }

    Push-Location (Join-Path $PSScriptRoot '../ui')
    try {
        npm run test:e2e:auth
        if ($LASTEXITCODE -ne 0) { throw 'console auth E2E failed' }
    } finally {
        Pop-Location
    }
} finally {
    if ($proc -and -not $proc.HasExited) {
        Stop-Process -Id $proc.Id -Force -ErrorAction SilentlyContinue
    }
    Remove-Item -Path (Join-Path $objs 'srs.pid') -ErrorAction SilentlyContinue
    Remove-Item Env:SRS_AUTH_USERNAME -ErrorAction SilentlyContinue
    Remove-Item Env:SRS_AUTH_PASSWORD -ErrorAction SilentlyContinue
    Remove-Item Env:SRS_BASE -ErrorAction SilentlyContinue
}

Write-Host 'auth-e2e: PASS' -ForegroundColor Green
