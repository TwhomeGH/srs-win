<#
.SYNOPSIS
    Remove a registered Git worktree by sending it to the Windows Recycle Bin.

.DESCRIPTION
    `git worktree remove` deletes files directly and has no undo. This helper
    keeps the Recycle Bin as the default: it validates the target, refuses a
    dirty worktree unless -DiscardChanges is explicit, sends the directory to
    the Recycle Bin, and only then prunes Git's stale worktree metadata.

    The branch is not deleted. Delete it separately after it has been merged:
        git branch -d <branch>

.PARAMETER Path
    Path of a worktree registered in this repository. The main worktree is
    rejected.

.PARAMETER DiscardChanges
    Allow removal when tracked files are modified or untracked files exist.
    The directory still goes to the Recycle Bin, not a permanent delete.

.PARAMETER DryRun
    Print the checks and target without moving anything.

.EXAMPLE
    pwsh dev/remove-worktree.ps1 -Path F:\srs-tutorial-learn1 -DryRun
    pwsh dev/remove-worktree.ps1 -Path F:\srs-tutorial-learn1
#>
[CmdletBinding()]
param(
    [Parameter(Mandatory = $true)]
    [string]$Path,

    [switch]$DiscardChanges,

    [switch]$DryRun
)

$ErrorActionPreference = 'Stop'
$repoRoot = (Resolve-Path (Join-Path $PSScriptRoot '..')).Path

if (-not $IsWindows) {
    throw 'This helper uses the Windows Recycle Bin. On macOS use Finder Trash; on Linux use gio trash, then run git worktree prune.'
}

if (-not (Test-Path -LiteralPath $Path)) {
    throw "worktree path does not exist: $Path"
}

$resolved = (Resolve-Path -LiteralPath $Path).Path.TrimEnd('\', '/')
$repoRoot = $repoRoot.TrimEnd('\', '/')

function ConvertTo-NormalizedPath([string]$value) {
    return [IO.Path]::GetFullPath($value).TrimEnd('\', '/')
}

$resolved = ConvertTo-NormalizedPath $resolved
$repoRoot = ConvertTo-NormalizedPath $repoRoot

if ($resolved -eq $repoRoot) {
    throw 'refusing to remove the main worktree'
}

$worktreePaths = @()
foreach ($line in (& git -C $repoRoot worktree list --porcelain)) {
    if ($line -match '^worktree (.+)$') {
        $worktreePaths += ConvertTo-NormalizedPath $Matches[1]
    }
}

if ($worktreePaths -notcontains $resolved) {
    throw "path is not a registered worktree: $resolved"
}

$branch = (& git -C $resolved rev-parse --abbrev-ref HEAD 2>$null)
$status = @(& git -C $resolved status --porcelain)

Write-Host "worktree: $resolved"
Write-Host "branch:   $branch"

if ($status.Count -gt 0) {
    Write-Host 'uncommitted or untracked changes:' -ForegroundColor Yellow
    $status | ForEach-Object { Write-Host "  $_" }
    if (-not $DiscardChanges) {
        throw 'refusing to remove a dirty worktree; commit/patch the changes or pass -DiscardChanges (still goes to the Recycle Bin)'
    }
    Write-Host 'continuing because -DiscardChanges was provided; files will still be sent to the Recycle Bin.' -ForegroundColor Yellow
}

# A running SRS/dev server can hold build files and make the move fail. Report
# it instead of silently falling back to a permanent delete.
$locked = Get-Process -ErrorAction SilentlyContinue |
    Where-Object {
        $processPath = $null
        try { $processPath = $_.Path } catch { $processPath = $null }
        $processPath -and (ConvertTo-NormalizedPath $processPath).StartsWith($resolved + [IO.Path]::DirectorySeparatorChar, [StringComparison]::OrdinalIgnoreCase)
    }
if ($locked) {
    $locked | Select-Object Id, ProcessName, Path | Format-Table -AutoSize
    throw 'processes are running from this worktree; stop them before removing it'
}

if ($branch -ne 'HEAD') {
    $unmerged = (& git -C $repoRoot rev-list --count "develop..$branch" 2>$null)
    if ($unmerged -and [int]$unmerged -gt 0) {
        Write-Host "branch has $unmerged commit(s) not in develop; the branch is kept." -ForegroundColor Yellow
    }
}

if ($DryRun) {
    Write-Host 'dry run: no files moved' -ForegroundColor Cyan
    exit 0
}

Add-Type -AssemblyName Microsoft.VisualBasic
[Microsoft.VisualBasic.FileIO.FileSystem]::DeleteDirectory(
    $resolved,
    [Microsoft.VisualBasic.FileIO.UIOption]::OnlyErrorDialogs,
    [Microsoft.VisualBasic.FileIO.RecycleOption]::SendToRecycleBin,
    [Microsoft.VisualBasic.FileIO.UICancelOption]::ThrowException)

if (Test-Path -LiteralPath $resolved) {
    throw 'Recycle Bin operation returned but the directory still exists; no success reported'
}

& git -C $repoRoot worktree prune
if ($LASTEXITCODE -ne 0) {
    throw 'worktree directory was recycled, but git worktree prune failed; run it manually'
}

$remaining = @()
foreach ($line in (& git -C $repoRoot worktree list --porcelain)) {
    if ($line -match '^worktree (.+)$') {
        $remaining += ConvertTo-NormalizedPath $Matches[1]
    }
}
if ($remaining -contains $resolved) {
    throw 'directory was recycled but the worktree is still registered; run git worktree prune manually'
}

Write-Host "moved to Recycle Bin: $resolved" -ForegroundColor Green
Write-Host 'worktree metadata pruned.' -ForegroundColor Green
if ($branch -ne 'HEAD') {
    Write-Host "branch kept: $branch"
    Write-Host "after merging, delete it with: git branch -d $branch"
}

