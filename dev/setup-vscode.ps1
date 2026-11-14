<#
# .SYNOPSIS
#     Link the local vcpkg installation into the workspace.
#
# .DESCRIPTION
#     VSCode IntelliSense and CMakePresets reference .vscode/vcpkg instead of
#     a hard-coded C:/H: path. This script discovers vcpkg (or accepts an
#     explicit -VcpkgRoot) and creates a directory junction on Windows, or a
#     symlink elsewhere.
#
#     Discovery order:
#       1. -VcpkgRoot
#       2. VCPKG_ROOT
#       3. vcpkg.exe on PATH
#       4. <drive>:\vcpkg and <home>/vcpkg
#
# .EXAMPLE
#     pwsh dev/setup-vscode.ps1
#     pwsh dev/setup-vscode.ps1 -VcpkgRoot D:\tools\vcpkg
#>
[CmdletBinding()]
param(
    [string]$VcpkgRoot
)

$ErrorActionPreference = 'Stop'
$repoRoot = (Resolve-Path (Join-Path $PSScriptRoot '..')).Path
$link = Join-Path $repoRoot '.vscode\vcpkg'

function Find-VcpkgRoot {
    if ($VcpkgRoot) { return $VcpkgRoot }
    if ($env:VCPKG_ROOT) { return $env:VCPKG_ROOT }

    $command = Get-Command vcpkg.exe -ErrorAction SilentlyContinue
    if ($command) { return (Split-Path $command.Source -Parent) }

    $candidates = @()
    foreach ($drive in (Get-PSDrive -PSProvider FileSystem)) {
        if ($drive.Root) {
            $candidates += (Join-Path $drive.Root 'vcpkg')
        }
    }
    $candidates += (Join-Path $HOME 'vcpkg')

    foreach ($candidate in $candidates) {
        if (Test-Path (Join-Path $candidate 'scripts\buildsystems\vcpkg.cmake')) {
            return $candidate
        }
    }

    return $null
}

$root = Find-VcpkgRoot
if (-not $root) {
    throw 'vcpkg not found. Pass -VcpkgRoot <path> or set VCPKG_ROOT.'
}

$root = (Resolve-Path $root).Path
$toolchain = Join-Path $root 'scripts\buildsystems\vcpkg.cmake'
if (-not (Test-Path $toolchain)) {
    throw "Not a vcpkg root (missing scripts/buildsystems/vcpkg.cmake): $root"
}

New-Item -ItemType Directory -Path (Split-Path $link -Parent) -Force | Out-Null

if (Test-Path $link) {
    $item = Get-Item $link -Force
    $target = @($item.Target) | Select-Object -First 1
    if ($item.LinkType -in @('Junction', 'SymbolicLink') -and $target -eq $root) {
        Write-Output ".vscode/vcpkg already points to $root"
        return
    }
    if ($item.LinkType -in @('Junction', 'SymbolicLink')) {
        Remove-Item $link -Force
    } else {
        throw "Refusing to replace a real directory: $link"
    }
}

if ($IsWindows) {
    New-Item -ItemType Junction -Path $link -Target $root | Out-Null
} else {
    New-Item -ItemType SymbolicLink -Path $link -Target $root | Out-Null
}

Write-Output ".vscode/vcpkg -> $root"
Write-Output 'Reopen VSCode or reload the C/C++ configuration if IntelliSense still shows the old path.'
