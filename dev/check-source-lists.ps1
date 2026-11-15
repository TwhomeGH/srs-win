<#
.SYNOPSIS
    Check that every src/app source is in both build manifests.

.DESCRIPTION
    SRS has two build paths: trunk/configure (Autotools) and
    trunk/ide/srs_msvc/CMakeLists.txt (MSVC). A new app source that is added
    to only one path links on one platform and fails on the other; the console
    auth file once missed trunk/configure and broke the Linux Docker lane.

    This check compares:
      - actual trunk/src/app/*.cpp files
      - srs_app_* entries in trunk/configure
      - srs_app_*.cpp entries in the MSVC CMakeLists.txt

.EXAMPLE
    pwsh dev/check-source-lists.ps1
#>
[CmdletBinding()]
param()

$ErrorActionPreference = 'Stop'
$repoRoot = (Resolve-Path (Join-Path $PSScriptRoot '..')).Path
$appDir = Join-Path $repoRoot 'trunk/src/app'
$configurePath = Join-Path $repoRoot 'trunk/configure'
$cmakePath = Join-Path $repoRoot 'trunk/ide/srs_msvc/CMakeLists.txt'

$actual = Get-ChildItem -LiteralPath $appDir -Filter '*.cpp' -File |
    ForEach-Object { $_.BaseName } |
    Sort-Object -Unique

$configureText = Get-Content -LiteralPath $configurePath -Raw
$configureNames = [regex]::Matches($configureText, '"srs_app_[a-z0-9_]+"') |
    ForEach-Object { $_.Value.Trim('"') } |
    Sort-Object -Unique

$cmakeText = Get-Content -LiteralPath $cmakePath -Raw
$cmakeNames = [regex]::Matches($cmakeText, 'srs_app_[a-z0-9_]+\.cpp') |
    ForEach-Object { $_.Value -replace '\.cpp$', '' } |
    Sort-Object -Unique

function Show-Diff([string]$label, [string[]]$expected, [string[]]$actualNames) {
    $missing = $expected | Where-Object { $_ -notin $actualNames }
    $extra = $actualNames | Where-Object { $_ -notin $expected }
    if ($missing.Count -gt 0) {
        Write-Host "missing from ${label}:" -ForegroundColor Red
        $missing | ForEach-Object { Write-Host "  $_" }
    }
    if ($extra.Count -gt 0) {
        Write-Host "stale in ${label} (file does not exist):" -ForegroundColor Yellow
        $extra | ForEach-Object { Write-Host "  $_" }
    }
    return ($missing.Count -eq 0 -and $extra.Count -eq 0)
}

# Learn 1 uses this file to prove the source-list guard fails. It must remain
# visible to git status (not .gitignore) so a forgotten probe cannot hide.
$tutorialProbe = Join-Path $appDir 'srs_app_tutorial_probe.cpp'
$probePresent = Test-Path -LiteralPath $tutorialProbe
if ($probePresent) {
    Write-Host 'tutorial probe detected: delete trunk/src/app/srs_app_tutorial_probe.cpp' -ForegroundColor Red
}

$okConfigure = Show-Diff 'trunk/configure' $actual $configureNames
$okCmake = Show-Diff 'CMakeLists.txt' $actual $cmakeNames

if ($probePresent -or -not ($okConfigure -and $okCmake)) {
    Write-Host 'source list check: FAIL' -ForegroundColor Red
    exit 1
}

Write-Host ("source list check: OK ({0} app sources in configure and CMake)" -f $actual.Count) -ForegroundColor Green
exit 0
