# stop.ps1 — Stop all SRS processes
Get-Process -Name "srs" -ErrorAction SilentlyContinue | Stop-Process -Force
Write-Host "SRS stopped" -ForegroundColor Green
