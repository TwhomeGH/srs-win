# Dev 測試環境（Windows 原生＋Linux Docker）

開發用的 SRS 測試入口。所有執行時產物都在 `dev/runtime/`（已 gitignore），不寫入 `%APPDATA%` 或 `%TEMP%`。

| 車道 | 用途 | 產物 |
|------|------|------|
| Windows 原生 | 本機除錯、Windows 移植驗證 | `dev/runtime/windows/srs-dev.exe` |
| Docker | 上游 Linux 版對照、協議行為比對 | `srs-win-dev-linux` 映像＋容器 `srs-dev` |

> `srs-dev.exe` 是 **開發用副本**，不是正式產物；正式測試請用 `trunk/ide/srs_msvc/build/Debug/srs.exe` 或 CI artifact。

## 快速開始

```powershell
# Windows：建置、啟動、看狀態、停止
.\dev\srs-dev.ps1 -Build -Run
.\dev\srs-dev.ps1 -Status
.\dev\srs-dev.ps1 -Stop

# Docker（上游 Linux 版）
.\dev\srs-dev.ps1 -Platform docker -Build -Run
.\dev\srs-dev.ps1 -Platform docker -Status
.\dev\srs-dev.ps1 -Platform docker -Stop

# 清掉 dev/runtime（會先停止對應車道）
.\dev\srs-dev.ps1 -Clean
```

## 連接埠

兩個車道刻意錯開，且都避開本機可能已被占用的 1985。

| 用途 | Windows 原生 | Docker（主機 → 容器） |
|------|--------------|------------------------|
| RTMP | `11935` | `11935 → 1935` |
| HTTP API | `11985` | `11985 → 1985` |
| HTTP 靜態／HLS | `18088` | `18088 → 8088` |
| SRT（UDP） | `19980` | `11980 → 10080` |
| WebRTC（UDP） | `19000` | `18000 → 8000` |

驗證：

```powershell
curl http://127.0.0.1:11985/api/v1/versions
```

## 快速檢查

```powershell
pwsh dev/check.ps1          # 增量建置 srs + st-event-loop-test，再跑 ctest
pwsh dev/check.ps1 -Full    # windows-full（RTC/SRT）
pwsh dev/check.ps1 -Docs    # 同時更新 _site/api
```

`check.ps1` 不 clean、只重建變更的 target；適合每次改碼後跑。

## 本機部署到固定 Docker 容器

```powershell
pwsh dev/local-deploy.ps1            # 建置 image + 回報 revision／version
pwsh dev/local-deploy.ps1 -Check     # 只看狀態，不建置
pwsh dev/local-deploy.ps1 -Deploy    # 建置並 force-recreate 容器
pwsh dev/local-deploy.ps1 -Deploy -Test  # 加上 HTTP／RTMP smoke
```

設定存在 `dev/local-deploy.ini`（已 gitignore）；沒有 ini 時使用 repo 相對預設值。範例見 `dev/local-deploy.ini.example`。

## 本機 API 參考

```powershell
pwsh dev/build-docs.ps1 -Open
```

有本機 `doxygen` 就直接用；否則用 `ubuntu:24.04` 容器安裝 Doxygen 後產生 `_site/api/index.html`。

## Web 控制台

```powershell
pwsh dev/serve-ui.ps1      # Vite HMR，http://127.0.0.1:5173/console/
pwsh dev/build-ui.ps1      # 建置並安裝到 objs/nginx/html/console/
```

建置後可直接開 `http://127.0.0.1:11985/console/`；API 同源，不需要 CORS。細節見 [`ui/README.md`](../ui/README.md)。

## Windows 車道

`-Build` 會：

1. 若沒有 `CMakeCache.txt` 就 configure（Visual Studio 17 2022 / x64）。
2. 建置 `trunk/ide/srs_msvc/build` 的 Debug。
3. 複製 `srs.exe` → `dev/runtime/windows/srs-dev.exe`，連同 `srs.pdb`、OpenSSL DLL。
4. 建立可寫的 `objs/`，使用 `dev/windows/dev.conf`。

日誌：`dev/runtime/windows/srs-dev.out.log`、`srs-dev.err.log`；pid：`srs-dev.pid`。

## Docker 車道

`-Build` 會用 `dev/docker/Dockerfile` 從本地 `trunk/` 建置**上游 Linux 版**（Ubuntu 24.04），不是 Windows 移植版。映像刻意使用發行版套件而非 vendored 舊版：OpenSSL 3、libsrtp 2.5+、SRT 1.5+、FFmpeg 6.1+，configure 帶 `--use-sys-srtp=on --use-sys-srt=on --use-sys-ffmpeg=on`。

`-Run` 以 `dev/docker/dev.conf` 啟動，並把主機的 11935／11985／18088（TCP）與 11980／18000（UDP）對到容器的 1935／1985／8088／10080／8000。啟動日誌應看到：

```
RTMP listen at tcp://0.0.0.0:1935
HTTP-API listen at tcp://0.0.0.0:1985
srt listen at udp://0.0.0.0:10080
WebRTC listen at udp://0.0.0.0:8000
```

原始碼變更後要重新 `-Build`；只改 `dev/docker/dev.conf` 時，`-Stop` 後 `-Run` 即可（設定檔以唯讀 volume 掛入）。

> Windows 車道的 RTC／SRT 預設關閉；要測試全功能，使用 vcpkg 與 `windows-full` preset（見下方）。Docker 車道已預設啟用兩者。

## Windows 全功能（RTC／SRT）

```powershell
$env:VCPKG_ROOT = "H:\vcpkg"
& "$env:VCPKG_ROOT\vcpkg.exe" install libsrtp libsrt opus --triplet x64-windows

cd trunk/ide/srs_msvc
cmake --preset windows-full
cmake --build --preset windows-full-debug
ctest --preset windows-full-test
```

輸出在 `trunk/ide/srs_msvc/build-full/Debug`；RTC／SRT 監聽埠沿用 `dev/windows/dev.conf` 的 `19980`／`19000`（UDP）。

SRT 推流驗證（需 FFmpeg）：

```powershell
ffmpeg -re -f lavfi -i testsrc=size=320x240:rate=10 -t 5 `
  -c:v libx264 -preset ultrafast -f mpegts `
  "srt://127.0.0.1:19980?streamid=#!::r=live/srt-test,m=publish"
```

## 為什麼放在 repo 內

- 測試設定與產物跟著專案版本走，換機器不用重建。
- 不污染 `%APPDATA%`／`%TEMP%`；`-Clean` 就能全部移除。
- `.dockerignore` 已排除 `dev/runtime`、建置目錄與 `Docs/`，Docker context 不會帶進產物。
- `.changelog.toml` 的 `exclude` 也排除 `dev/runtime/**`，靜態站不會索引執行時資料。

## 常見問題

| 症狀 | 處理 |
|------|------|
| `srs-dev.exe is missing` | 先跑 `-Build` |
| `code=1034 PidFileAcquire` | `dev/runtime/windows/objs` 不存在或殘留 `srs.pid`；重跑 `-Build` 或刪除該 pid |
| 11985 已被占用 | 改 `dev/windows/dev.conf` 與 `dev/srs-dev.ps1` 的 `ApiUrl` |
| Docker 建置失敗 | 確認 Docker Desktop 已啟動；`docker compose -f dev/docker/docker-compose.yml build` 看完整日誌 |
| 兩個車道同時跑 | 不行，主機埠相同；先 `-Stop` 再切換 |
