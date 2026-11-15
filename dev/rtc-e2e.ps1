#>
# .SYNOPSIS
#     Run the RTC media E2E test against a full SRS build.
#
# .DESCRIPTION
#     Starts SRS with RTC enabled, opens the WHIP publisher fixture, plays the
#     stream through the console WHEP player, and verifies that the browser
#     decoded video. Requires a full (windows-full) build with RTC and a built
#     console (dev/build-ui.ps1 -Target full).
#
# .EXAMPLE
#     pwsh dev/rtc-e2e.ps1 `
#       -SrsExe trunk/ide/srs_msvc/build-full/Debug/srs.exe `
#       -ConfFile dev/windows/dev.conf `
#       -WorkDir trunk/ide/srs_msvc/build-full/Debug
#>
[CmdletBinding()]
param(
    [Parameter(Mandatory = $true)]
    [string]$SrsExe,

    [Parameter(Mandatory = $true)]
    [string]$ConfFile,

    [Parameter(Mandatory = $true)]
    [string]$WorkDir,

    [int]$ApiPort = 11985,
    [int]$RtmpPort = 11935,
    [string]$BaseUrl = ''
)

$ErrorActionPreference = 'Stop'

$SrsExe = (Resolve-Path $SrsExe).Path
$ConfFile = (Resolve-Path $ConfFile).Path
$WorkDir = (Resolve-Path $WorkDir).Path
if (-not $BaseUrl) {
    $BaseUrl = "http://127.0.0.1:$ApiPort"
}

$consoleDir = Join-Path $WorkDir 'objs/nginx/html/console'
if (-not (Test-Path (Join-Path $consoleDir 'index.html'))) {
    throw "console build not found at $consoleDir; run dev/build-ui.ps1 -Target full first"
}

$fixtureSrc = Join-Path $PSScriptRoot '../ui/e2e/rtc-whip.html'
$fixtureDstDir = Join-Path $consoleDir 'e2e'
New-Item -ItemType Directory -Path $fixtureDstDir -Force | Out-Null
Copy-Item $fixtureSrc (Join-Path $fixtureDstDir 'rtc-whip.html') -Force

$objs = Join-Path $WorkDir 'objs'
New-Item -ItemType Directory -Path $objs -Force | Out-Null
Remove-Item -Path (Join-Path $objs 'srs.pid') -ErrorAction SilentlyContinue

Write-Host "rtc-e2e: exe=$SrsExe conf=$ConfFile cwd=$WorkDir base=$BaseUrl"
$proc = Start-Process -FilePath $SrsExe `
    -ArgumentList @('-c', $ConfFile) `
    -WorkingDirectory $WorkDir `
    -NoNewWindow -PassThru

try {
    $ready = $false
    for ($i = 0; $i -lt 40; $i++) {
        Start-Sleep -Milliseconds 500
        $proc.Refresh()
        if ($proc.HasExited) { throw "SRS exited before API was ready, code=$($proc.ExitCode)" }
        try {
            $null = Invoke-RestMethod -Uri "$BaseUrl/api/v1/versions" -TimeoutSec 2
            $ready = $true
            break
        } catch {
            continue
        }
    }
    if (-not $ready) { throw "SRS API did not become ready at $BaseUrl" }

    Push-Location (Join-Path $PSScriptRoot '../ui')
    try {
        $env:SRS_BASE = $BaseUrl
        npm run test:e2e:rtc
        if ($LASTEXITCODE -ne 0) { throw 'RTC media E2E failed' }
    } finally {
        Pop-Location
        Remove-Item Env:SRS_BASE -ErrorAction SilentlyContinue
    }
} finally {
    if ($proc -and -not $proc.HasExited) {
        Stop-Process -Id $proc.Id -Force -ErrorAction SilentlyContinue
    }
    Remove-Item -Path (Join-Path $objs 'srs.pid') -ErrorAction SilentlyContinue
}

Write-Host 'rtc-e2e: PASS' -ForegroundColor Green
