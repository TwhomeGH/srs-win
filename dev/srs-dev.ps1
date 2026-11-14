<#
.SYNOPSIS
    Repo-local dev harness for SRS: Windows native srs-dev.exe and the Linux Docker lane.

.DESCRIPTION
    All dev runtime state stays under dev/runtime (gitignored), never in
    %APPDATA% or %TEMP%.

    Windows lane:
      - builds trunk/ide/srs_msvc/build (Debug)
      - copies srs.exe -> dev/runtime/windows/srs-dev.exe
      - copies the matching OpenSSL DLLs and dev/windows/dev.conf
      - creates a writable objs/ directory

    Docker lane:
      - builds the upstream Linux SRS from the local trunk/ sources
      - runs it in a container with the same host-facing dev ports

    Default action (no switches) is -Status.

.EXAMPLE
    .\dev\srs-dev.ps1 -Build -Run
    .\dev\srs-dev.ps1 -Status
    .\dev\srs-dev.ps1 -Stop

.EXAMPLE
    .\dev\srs-dev.ps1 -Platform docker -Build -Run
    .\dev\srs-dev.ps1 -Platform docker -Stop
#>
[CmdletBinding()]
param(
    [ValidateSet('windows', 'docker')]
    [string]$Platform = 'windows',

    [switch]$Build,
    [switch]$Run,
    [switch]$Stop,
    [switch]$Status,
    [switch]$Clean
)

$ErrorActionPreference = 'Stop'

$RepoRoot = Split-Path -Parent $PSScriptRoot
$WinRuntime = Join-Path $PSScriptRoot 'runtime\windows'
$WinConf = Join-Path $PSScriptRoot 'windows\dev.conf'
$BuildDir = Join-Path $RepoRoot 'trunk\ide\srs_msvc\build'
$WinExe = Join-Path $WinRuntime 'srs-dev.exe'
$PidFile = Join-Path $WinRuntime 'srs-dev.pid'
$ApiUrl = 'http://127.0.0.1:11985/api/v1/versions'
$ComposeFile = Join-Path $PSScriptRoot 'docker\docker-compose.yml'
$ComposeArgs = @('compose', '-f', $ComposeFile, '-p', 'srs-dev')

function Get-WindowsStatus {
    if (-not (Test-Path $PidFile)) {
        Write-Host 'windows: not running (no pid file)'
        return $false
    }

    $processId = [int](Get-Content $PidFile -Raw)
    $process = Get-Process -Id $processId -ErrorAction SilentlyContinue
    if (-not $process) {
        Write-Host "windows: stale pid $processId"
        return $false
    }

    Write-Host "windows: running (pid $processId)"
    try {
        $response = Invoke-WebRequest -Uri $ApiUrl -UseBasicParsing -TimeoutSec 5
        Write-Host "windows: api $($response.StatusCode)"
        return $true
    } catch {
        Write-Host "windows: api error - $($_.Exception.Message)"
        return $false
    }
}

function Stop-Windows {
    if (-not (Test-Path $PidFile)) {
        Write-Output 'windows: not running'
        return
    }

    $processId = [int](Get-Content $PidFile -Raw)
    $process = Get-Process -Id $processId -ErrorAction SilentlyContinue
    if ($process) {
        Stop-Process -Id $processId -Force
        Write-Output "windows: stopped pid $processId"
    } else {
        Write-Output "windows: pid $processId was not running"
    }
    Remove-Item $PidFile -Force -ErrorAction SilentlyContinue
}

function Build-Windows {
    if (-not (Test-Path (Join-Path $BuildDir 'CMakeCache.txt'))) {
        Write-Output 'windows: configuring CMake'
        cmake -B $BuildDir -S (Join-Path $RepoRoot 'trunk\ide\srs_msvc') -G 'Visual Studio 17 2022' -A x64
        if ($LASTEXITCODE -ne 0) { throw 'CMake configure failed' }
    }

    Write-Output 'windows: building Debug'
    cmake --build $BuildDir --config Debug -- /m:2
    if ($LASTEXITCODE -ne 0) { throw 'CMake build failed' }

    New-Item -ItemType Directory $WinRuntime -Force | Out-Null
    Copy-Item (Join-Path $BuildDir 'Debug\srs.exe') $WinExe -Force
    Copy-Item (Join-Path $BuildDir 'Debug\srs.pdb') (Join-Path $WinRuntime 'srs.pdb') -Force -ErrorAction SilentlyContinue
    Get-ChildItem (Join-Path $BuildDir 'Debug') -Filter 'libcrypto-*.dll' | Copy-Item -Destination $WinRuntime -Force
    Get-ChildItem (Join-Path $BuildDir 'Debug') -Filter 'libssl-*.dll' | Copy-Item -Destination $WinRuntime -Force
    New-Item -ItemType Directory (Join-Path $WinRuntime 'objs') -Force | Out-Null
    Write-Output "windows: bundle ready at $WinRuntime"
}

function Start-Windows {
    Stop-Windows

    if (-not (Test-Path $WinExe)) {
        throw 'srs-dev.exe is missing; run with -Build first'
    }

    $stdout = Join-Path $WinRuntime 'srs-dev.out.log'
    $stderr = Join-Path $WinRuntime 'srs-dev.err.log'
    $process = Start-Process -FilePath $WinExe `
        -ArgumentList '-c', $WinConf `
        -WorkingDirectory $WinRuntime `
        -RedirectStandardOutput $stdout `
        -RedirectStandardError $stderr `
        -PassThru

    $process.Id | Set-Content $PidFile
    Start-Sleep -Seconds 3

    if ($process.HasExited) {
        Write-Output "windows: exited with code $($process.ExitCode)"
        Get-Content $stderr -ErrorAction SilentlyContinue | Select-Object -Last 10
        throw 'srs-dev.exe failed to start'
    }

    Write-Output "windows: started pid $($process.Id)"
    Write-Output "windows: log $stdout"
    Get-WindowsStatus | Out-Null
}

function Invoke-Docker([string[]]$Arguments) {
    & docker @ComposeArgs @Arguments
    if ($LASTEXITCODE -ne 0) {
        throw "docker compose $($Arguments -join ' ') failed"
    }
}

function Get-DockerStatus {
    try {
        $response = Invoke-WebRequest -Uri $ApiUrl -UseBasicParsing -TimeoutSec 5
        Write-Host "docker: api $($response.StatusCode)"
        return $true
    } catch {
        Write-Host "docker: api error - $($_.Exception.Message)"
        return $false
    }
}

$didSomething = $false

if ($Clean) {
    $didSomething = $true
    if ($Platform -eq 'windows') {
        Stop-Windows
    } else {
        Invoke-Docker @('down', '--remove-orphans')
    }
    Remove-Item (Join-Path $PSScriptRoot 'runtime') -Recurse -Force -ErrorAction SilentlyContinue
    Write-Output 'runtime removed'
}

if ($Build) {
    $didSomething = $true
    if ($Platform -eq 'windows') { Build-Windows } else { Invoke-Docker @('build') }
}

if ($Run) {
    $didSomething = $true
    if ($Platform -eq 'windows') { Start-Windows } else { Invoke-Docker @('up', '-d') }
}

if ($Stop) {
    $didSomething = $true
    if ($Platform -eq 'windows') { Stop-Windows } else { Invoke-Docker @('down', '--remove-orphans') }
}

if ($Status -or -not $didSomething) {
    if ($Platform -eq 'windows') {
        Get-WindowsStatus | Out-Null
    } else {
        Invoke-Docker @('ps')
        Get-DockerStatus | Out-Null
    }
}
