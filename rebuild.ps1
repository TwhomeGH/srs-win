# rebuild.ps1 — Build + Run SRS on Windows
# SRS now auto-kills old instances on startup (via named mutex), so no manual kill needed.
param(
    [switch]$Run,
    [switch]$NoBuild
)

$BuildDir  = "F:\srs\trunk\ide\srs_msvc\build"
$OutDir    = "$BuildDir\Debug"
$SrsExe    = "$OutDir\srs.exe"
$ConfSrc   = "$OutDir\live.conf"

Write-Host "[1/3] Running CMake..." -ForegroundColor Cyan
cmake -B $BuildDir -S "F:\srs\trunk\ide\srs_msvc" `
      -G "Visual Studio 17 2022" -A x64 `
      -DCMAKE_TOOLCHAIN_FILE="C:\vcpkg\scripts\buildsystems\vcpkg.cmake"
if ($LASTEXITCODE -ne 0) { Write-Host "CMake failed!" -ForegroundColor Red; exit 1 }

if (-not $NoBuild) {
    Write-Host "[2/3] Building..." -ForegroundColor Cyan
    & "C:\Program Files\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin\MSBuild.exe" `
        "$BuildDir\srs.sln" /p:Configuration=Debug /m
    if ($LASTEXITCODE -ne 0) { Write-Host "Build failed!" -ForegroundColor Red; exit 1 }
}

Write-Host "[3/3] Ready!" -ForegroundColor Cyan
$env:SRS_PID = "$OutDir\objs\srs.pid"

if ($Run) {
    Write-Host "Starting SRS..." -ForegroundColor Yellow
    $proc = Start-Process -FilePath $SrsExe -ArgumentList "-c `"$ConfSrc`"" -NoNewWindow -PassThru
    Start-Sleep -Seconds 4
    if ($proc.HasExited) {
        Write-Host "SRS exited with code $($proc.ExitCode)" -ForegroundColor Red
        exit $proc.ExitCode
    } else {
        Write-Host "SRS is running (PID=$($proc.Id))" -ForegroundColor Green
    }
} else {
    Write-Host "To run:    .\rebuild.ps1 -Run" -ForegroundColor Yellow
    Write-Host "To stop:   .\stop.ps1" -ForegroundColor Yellow
    Write-Host "To config: $ConfSrc" -ForegroundColor Yellow
}
