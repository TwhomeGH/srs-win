<#
.SYNOPSIS
    Lint or auto-fix the project's maintained Markdown files.

.DESCRIPTION
    Vendored sources and upstream reference docs are ignored by
    .markdownlint-cli2.jsonc. MD024 uses siblings_only because ChangeHistory
    intentionally repeats "問題/修改/驗證" under different dated sections.

    Markdownlint can auto-fix formatting rules such as MD022/MD031/MD032/MD047.
    MD024 (duplicate headings) is not auto-fixable: either rename the heading or
    use siblings_only when the duplication is intentional under different parents.

.EXAMPLE
    pwsh dev/lint-docs.ps1
    pwsh dev/lint-docs.ps1 -Fix
#>
[CmdletBinding()]
param(
    [switch]$Fix
)

$ErrorActionPreference = 'Stop'
$repoRoot = (Resolve-Path (Join-Path $PSScriptRoot '..')).Path
$globs = @(
    'Docs/**/*.md',
    'README.md',
    'TODO.md',
    'dev/*.md',
    'ui/*.md'
)

$lintArgs = @()
if ($Fix) {
    $lintArgs += '--fix'
}
$lintArgs += $globs

Push-Location $repoRoot
try {
    & npx --yes markdownlint-cli2@0.18.1 @lintArgs
    $code = $LASTEXITCODE
} finally {
    Pop-Location
}

if ($code -ne 0) {
    if ($Fix) {
        Write-Host 'Some Markdown issues are not auto-fixable (for example MD024, MD040, MD045); review the output above.' -ForegroundColor Yellow
    }
    exit $code
}

Write-Host 'markdownlint: OK' -ForegroundColor Green

# ChangeHistory headings are part of the changelogkit contract. Keep the
# timestamp mandatory so entries remain sortable and do not silently fall
# back to date-only ordering.
$history = Join-Path $repoRoot 'Docs/ChangeHistory.md'
if (Test-Path $history) {
    $missing = Select-String -Path $history -Pattern '^## ' |
        Where-Object { $_.Line -notmatch '^## \d{4}\.\d{2}\.\d{2} \d{2}:\d{2} ' }
    if ($missing) {
        Write-Host 'ChangeHistory entries missing HH:MM:' -ForegroundColor Red
        $missing | ForEach-Object { Write-Host ("  " + $_.LineNumber + ': ' + $_.Line) }
        exit 1
    }
    Write-Host 'ChangeHistory timestamps: OK' -ForegroundColor Green
}
