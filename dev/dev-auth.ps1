<#
.SYNOPSIS
    Create or reuse a stable development-only console password.

.DESCRIPTION
    Production SRS generates a random initial http_api.auth password and writes
    it to objs/console-auth.txt. Development needs a stable value across
    restarts, so this script stores one under dev/runtime/dev-password.txt
    (gitignored) and exports SRS_HTTP_API_AUTH_PASSWORD for the current shell.

    SRS_HTTP_API_AUTH_PASSWORD always wins, matching VideoServer's dev-password
    pattern. The file is not committed and the password is not printed by SRS
    itself.

.EXAMPLE
    . .\dev\dev-auth.ps1
    pwsh dev/srs-dev.ps1 -Build -Run

    # Optional non-default username
    $env:SRS_HTTP_API_AUTH_USERNAME = 'operator'
    . .\dev\dev-auth.ps1
#>
[CmdletBinding()]
param()

$ErrorActionPreference = 'Stop'

if ($env:SRS_HTTP_API_AUTH_PASSWORD) {
    Write-Host '[dev-auth] using SRS_HTTP_API_AUTH_PASSWORD from environment' -ForegroundColor Cyan
    return
}

$dir = Join-Path $PSScriptRoot 'runtime'
$file = Join-Path $dir 'dev-password.txt'
New-Item -ItemType Directory -Path $dir -Force | Out-Null

$password = if (Test-Path $file) { (Get-Content $file -Raw).Trim() } else { '' }
if ($password.Length -lt 12) {
    $bytes = [System.Security.Cryptography.RandomNumberGenerator]::GetBytes(18)
    $password = [Convert]::ToBase64String($bytes).TrimEnd('=').Replace('+', '-').Replace('/', '_')
    Set-Content -Path $file -Value $password -NoNewline -Encoding ascii
}

$env:SRS_HTTP_API_AUTH_PASSWORD = $password
if (-not $env:SRS_HTTP_API_AUTH_USERNAME) {
    $env:SRS_HTTP_API_AUTH_USERNAME = 'admin'
}

Write-Host "[dev-auth] console password: $password" -ForegroundColor Yellow
Write-Host "[dev-auth] stored in $file (gitignored)" -ForegroundColor DarkGray
