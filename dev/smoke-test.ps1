#>
# .SYNOPSIS
#     Shared SRS Windows smoke test.
#
# .DESCRIPTION
#     Starts srs.exe with the given config, checks the HTTP API and RTMP
#     handshake, and optionally checks the web console and read-only config API.
#     Nightly and Release both call this script so their smoke coverage cannot
#     drift apart.
#
# .EXAMPLE
#     pwsh dev/smoke-test.ps1 -SrsExe build/Release/srs.exe -ConfFile build/Release/test.conf -CheckConsole
#>
[CmdletBinding()]
param(
    [Parameter(Mandatory = $true)]
    [string]$SrsExe,

    [Parameter(Mandatory = $true)]
    [string]$ConfFile,

    [string]$WorkDir,
    [int]$ApiPort = 1985,
    [int]$RtmpPort = 19350,
    [switch]$CheckConsole
)

$ErrorActionPreference = 'Stop'

$SrsExe = (Resolve-Path $SrsExe).Path
$ConfFile = (Resolve-Path $ConfFile).Path
if (-not $WorkDir) {
    $WorkDir = Split-Path $SrsExe -Parent
}
$WorkDir = (Resolve-Path $WorkDir).Path

$objs = Join-Path $WorkDir 'objs'
New-Item -ItemType Directory -Path $objs -Force | Out-Null
Remove-Item -Path (Join-Path $objs 'srs.pid') -ErrorAction SilentlyContinue

Write-Host "smoke: exe=$SrsExe conf=$ConfFile cwd=$WorkDir"

$proc = Start-Process -FilePath $SrsExe `
    -ArgumentList @('-c', $ConfFile) `
    -WorkingDirectory $WorkDir `
    -NoNewWindow -PassThru

try {
    # Wait until the HTTP API accepts connections; a fixed sleep is racy on CI.
    $api = $null
    for ($i = 0; $i -lt 30; $i++) {
        Start-Sleep -Milliseconds 500
        $proc.Refresh()
        if ($proc.HasExited) {
            throw "SRS exited before API was ready, code=$($proc.ExitCode)"
        }
        try {
            $api = Invoke-RestMethod -Uri "http://127.0.0.1:$ApiPort/api/v1/versions" -TimeoutSec 2
            break
        } catch {
            continue
        }
    }
    if (-not $api) {
        throw "HTTP API did not become ready on port $ApiPort"
    }
    if ($api.code -ne 0 -or -not $api.data.version) {
        throw "HTTP API returned invalid data"
    }
    Write-Host "API OK: $($api.data.version)" -ForegroundColor Green

    # RTMP handshake: 1537-byte C0+C1, expect 16-byte S0+S1 in reply.
    $tcp = New-Object System.Net.Sockets.TcpClient
    try {
        $tcp.Connect('127.0.0.1', $RtmpPort)
        $tcp.ReceiveTimeout = 15000
        $stream = $tcp.GetStream()
        $handshake = New-Object byte[] 1537
        (New-Object System.Random).NextBytes($handshake)
        $handshake[0] = 3
        $stream.Write($handshake, 0, $handshake.Length)
        $reply = New-Object byte[] 16
        $read = $stream.Read($reply, 0, $reply.Length)
        if ($read -ne 16) {
            throw "RTMP handshake returned $read bytes, expected 16"
        }
    } finally {
        $tcp.Close()
    }
    Write-Host 'RTMP handshake OK: 16 bytes' -ForegroundColor Green

    if ($CheckConsole) {
        $console = Invoke-WebRequest -Uri "http://127.0.0.1:$ApiPort/console/" -UseBasicParsing -TimeoutSec 15
        if ($console.StatusCode -ne 200) {
            throw "console returned $($console.StatusCode)"
        }
        Write-Host 'Console OK: /console/' -ForegroundColor Green
    }

    $config = Invoke-RestMethod -Uri "http://127.0.0.1:$ApiPort/api/v1/config" -TimeoutSec 15
    if ($config.code -ne 0 -or -not $config.data.version) {
        throw 'config API returned invalid data'
    }
    Write-Host "Config API OK: version=$($config.data.version) build=$($config.data.build)" -ForegroundColor Green
} finally {
    if ($proc -and -not $proc.HasExited) {
        Stop-Process -Id $proc.Id -Force -ErrorAction SilentlyContinue
    }
    Remove-Item -Path (Join-Path $objs 'srs.pid') -ErrorAction SilentlyContinue
}

Write-Host 'smoke: PASS' -ForegroundColor Green
