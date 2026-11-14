<#
.SYNOPSIS
    Local deploy for the fixed SRS dev container: build the image, report which
    revision it contains, and optionally recreate the container.

.DESCRIPTION
    Mirrors the Oryx local-deploy workflow:

      * The image carries git revision and SRS version labels (srs.revision,
        srs.version), so "which build is the container running?" is answerable.
      * -Check reports source vs image vs running container without changing
        anything.
      * -Deploy rebuilds the compose container with the new image.
      * Settings live in dev/local-deploy.ini (gitignored); defaults are
        repository-relative so a fresh checkout works without configuration.

    Runtime state stays in this repository: dev/docker (compose), dev/runtime
    (Windows dev lane), and the Docker volumes. Nothing is written to AppData.

    Examples:
      pwsh dev/local-deploy.ps1                 # build + report
      pwsh dev/local-deploy.ps1 -Check          # report only
      pwsh dev/local-deploy.ps1 -Deploy         # build + recreate container
      pwsh dev/local-deploy.ps1 -Test           # deploy + HTTP/RTMP smoke
      pwsh dev/local-deploy.ps1 -Docs           # build the local API reference
      pwsh dev/local-deploy.ps1 -ComposeDir F:\srs\dev\docker -Save

.PARAMETER Image
    Image tag. Default: srs-win-dev-linux:local.

.PARAMETER Project
    docker compose project name. Default: srs-dev.

.PARAMETER Service
    compose service name. Default: srs-dev.

.PARAMETER Container
    Optional full container name; otherwise resolved through docker compose.

.PARAMETER ComposeDir
    Directory containing docker-compose.yml. Default: dev/docker.

.PARAMETER Check
    Only report; do not build or recreate.

.PARAMETER Deploy
    Recreate the container with the built image.

.PARAMETER Test
    After deploy, check HTTP API and RTMP handshake on the host ports.

.PARAMETER Docs
    Build _site/api via dev/build-docs.ps1 (Doxygen or Docker fallback).

.PARAMETER Save
    Persist ComposeDir/Project/Service/Image/Container to local-deploy.ini.

.PARAMETER DryRun
    Print actions without running docker build/compose.
#>
[CmdletBinding()]
param(
    [string]$Image,
    [string]$Project,
    [string]$Service,
    [string]$Container,
    [string]$ComposeDir,
    [switch]$Check,
    [switch]$Deploy,
    [switch]$Test,
    [switch]$Docs,
    [switch]$Save,
    [switch]$DryRun
)

$ErrorActionPreference = 'Stop'

$repoRoot = (Resolve-Path (Join-Path $PSScriptRoot '..')).Path
$iniPath = Join-Path $PSScriptRoot 'local-deploy.ini'
$defaultComposeDir = Join-Path $PSScriptRoot 'docker'

function Step([string]$message) {
    Write-Host "==> $message" -ForegroundColor Cyan
}

function Read-Ini([string]$path) {
    $values = @{}
    if (-not (Test-Path $path)) { return $values }

    foreach ($line in Get-Content $path) {
        if ($line -match '^\s*([^#=]+?)\s*=\s*(.*?)\s*$') {
            $values[$Matches[1]] = $Matches[2]
        }
    }
    return $values
}

$saved = Read-Ini $iniPath
if (-not $Image) { $Image = if ($saved['image']) { $saved['image'] } else { 'srs-win-dev-linux:local' } }
if (-not $Project) { $Project = if ($saved['project']) { $saved['project'] } else { 'srs-dev' } }
if (-not $Service) { $Service = if ($saved['service']) { $saved['service'] } else { 'srs-dev' } }
if (-not $ComposeDir) { $ComposeDir = if ($saved['composeDir']) { $saved['composeDir'] } else { $defaultComposeDir } }
if (-not $Container) { $Container = $saved['container'] }

# Keep relative paths anchored to the repository, not the caller's current directory.
if (-not [IO.Path]::IsPathRooted($ComposeDir)) {
    $ComposeDir = Join-Path $repoRoot $ComposeDir
}

if ($Save) {
    $ini = @(
        '# local-deploy settings (generated, gitignored).',
        "composeDir=$ComposeDir",
        "project=$Project",
        "service=$Service",
        "image=$Image",
        "container=$Container"
    ) -join "`n"
    [IO.File]::WriteAllText($iniPath, $ini + "`n", (New-Object System.Text.UTF8Encoding $false))
    Write-Host "saved settings to $iniPath" -ForegroundColor Green
    exit 0
}

$composeFile = Join-Path $ComposeDir 'docker-compose.yml'
if (-not (Test-Path $composeFile)) {
    Write-Host "compose file not found: $composeFile" -ForegroundColor Red
    exit 1
}

# --- Source revision + version -------------------------------------------------
$version = '7.0.0'
$versionFile = Join-Path $repoRoot 'trunk/src/core/srs_core_version7.hpp'
if (Test-Path $versionFile) {
    $text = Get-Content $versionFile -Raw
    $major = if ($text -match 'VERSION_MAJOR\s+(\d+)') { $Matches[1] } else { '7' }
    $minor = if ($text -match 'VERSION_MINOR\s+(\d+)') { $Matches[1] } else { '0' }
    $rev = if ($text -match 'VERSION_REVISION\s+(\d+)') { $Matches[1] } else { '0' }
    $version = "$major.$minor.$rev"
}

$revision = 'unknown'
$gitRevision = git -C $repoRoot rev-parse HEAD 2>$null
if ($gitRevision) { $revision = $gitRevision.Trim() }
$shortRevision = $revision.Substring(0, [Math]::Min(12, $revision.Length))
Write-Host "source: version=$version revision=$shortRevision"

# --- Docker helpers ------------------------------------------------------------
function Get-ImageInfo([string]$reference) {
    if (-not $reference) { return $null }
    $id = (docker image inspect $reference --format '{{.Id}}' 2>$null)
    if (-not $id) { return $null }

    $labels = docker image inspect $id --format '{{json .Config.Labels}}' 2>$null
    $rev = $null
    $ver = $null
    if ($labels) {
        try {
            $parsed = $labels | ConvertFrom-Json
            $rev = $parsed.'srs.revision'
            $ver = $parsed.'srs.version'
        } catch { }
    }

    return [pscustomobject]@{
        Id      = $id.Trim()
        Revision = $rev
        Version = $ver
    }
}

function Get-RunningContainerId {
    if ($Container) {
        $id = docker inspect $Container --format '{{.Id}}' 2>$null
        if ($id) { return $id.Trim() }
        return $null
    }

    $id = docker compose -f $composeFile -p $Project ps -q $Service 2>$null
    if ($id) { return ($id | Select-Object -First 1).Trim() }
    return $null
}

function Get-ContainerImageId([string]$containerId) {
    if (-not $containerId) { return $null }
    $id = docker inspect $containerId --format '{{.Image}}' 2>$null
    if ($id) { return $id.Trim() }
    return $null
}

function Show-Status {
    $local = Get-ImageInfo $Image
    $containerId = Get-RunningContainerId
    $runningId = Get-ContainerImageId $containerId

    Step 'status'
    if ($local) {
        $stale = $local.Revision -and ($local.Revision -ne $revision)
        $suffix = if ($stale) { 'stale (source changed)' } else { "matches source $shortRevision" }
        Write-Host ("  image {0}: {1} rev={2} version={3} -> {4}" -f
            $Image, $local.Id.Substring(7, 12), ($local.Revision ?? 'n/a'), ($local.Version ?? 'n/a'), $suffix) `
            -ForegroundColor $(if ($stale) { 'Yellow' } else { 'Green' })
    } else {
        Write-Host "  image ${Image}: not built" -ForegroundColor Yellow
    }

    if ($containerId) {
        $running = Get-ImageInfo $runningId
        if ($running) {
            $stale = $local -and ($running.Id -ne $local.Id)
            $suffix = if ($stale) { 'stale (deploy to apply image)' } else { 'up to date' }
            Write-Host ("  container {0}: {1} rev={2} version={3} -> {4}" -f
                $containerId.Substring(0, [Math]::Min(12, $containerId.Length)),
                $running.Id.Substring(7, 12), ($running.Revision ?? 'n/a'), ($running.Version ?? 'n/a'), $suffix) `
                -ForegroundColor $(if ($stale) { 'Yellow' } else { 'Green' })
        }
    } else {
        Write-Host "  container: not running ($Project/$Service)" -ForegroundColor DarkYellow
    }

    return [pscustomobject]@{ Local = $local; ContainerId = $containerId; Running = $runningId }
}

function Invoke-Build {
    Step "build $Image (revision=$shortRevision version=$version)"
    $buildArgs = @(
        'build',
        '-f', (Join-Path $ComposeDir 'Dockerfile'),
        '-t', $Image,
        '--label', "srs.revision=$revision",
        '--label', "srs.version=$version",
        $repoRoot
    )
    if ($DryRun) {
        Write-Host "[DryRun] docker $($buildArgs -join ' ')"
        return
    }
    docker @buildArgs
    if ($LASTEXITCODE -ne 0) { throw 'docker build failed' }
}

function Invoke-Deploy {
    Step "docker compose up -d --force-recreate ($Project/$Service)"
    if ($DryRun) {
        Write-Host "[DryRun] SRS_DEV_IMAGE=$Image docker compose -f $composeFile -p $Project up -d --force-recreate"
        return
    }
    $env:SRS_DEV_IMAGE = $Image
    docker compose -f $composeFile -p $Project up -d --force-recreate
    if ($LASTEXITCODE -ne 0) { throw 'docker compose up failed' }
}

function Test-DevEndpoints {
    Step 'smoke test (HTTP API + RTMP handshake)'
    try {
        $response = Invoke-WebRequest -Uri 'http://127.0.0.1:11985/api/v1/versions' -UseBasicParsing -TimeoutSec 10
        Write-Host "  HTTP API: $($response.StatusCode)" -ForegroundColor Green
    } catch {
        Write-Host "  HTTP API failed: $($_.Exception.Message)" -ForegroundColor Red
        throw
    }

    $tcp = New-Object System.Net.Sockets.TcpClient
    try {
        $tcp.Connect('127.0.0.1', 11935)
        $tcp.ReceiveTimeout = 10000
        $stream = $tcp.GetStream()
        $handshake = New-Object byte[] 1537
        (New-Object System.Random).NextBytes($handshake)
        $handshake[0] = 3
        $stream.Write($handshake, 0, $handshake.Length)
        $reply = New-Object byte[] 16
        $read = $stream.Read($reply, 0, $reply.Length)
        if ($read -ne 16) { throw "RTMP handshake returned $read bytes" }
        Write-Host "  RTMP handshake: $read bytes" -ForegroundColor Green
    } finally {
        $tcp.Close()
    }
}

# --- Main ----------------------------------------------------------------------
if (-not $Check) {
    Invoke-Build
}

$status = Show-Status

if ($Deploy) {
    Invoke-Deploy
    $status = Show-Status
}

if ($Test) {
    Test-DevEndpoints
}

if ($Docs) {
    & (Join-Path $PSScriptRoot 'build-docs.ps1') -Open
}

if (-not $Check -and -not $Deploy -and $status.Local -and $status.Local.Revision -and ($status.Local.Revision -ne $revision)) {
    Write-Host 'hint: image is older than the working tree; run this script again or use -Deploy.' -ForegroundColor Yellow
}

if ($status.ContainerId -and $status.Local -and $status.Running -and ($status.Running -ne $status.Local.Id)) {
    Write-Host 'hint: container is not running the latest image; add -Deploy.' -ForegroundColor Yellow
}
