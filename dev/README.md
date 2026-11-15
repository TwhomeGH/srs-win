# Dev 測試環境（Windows 原生＋Linux Docker）

開發用的 SRS 測試入口。所有執行時產物都在 `dev/runtime/`（已 gitignore），不寫入 `%APPDATA%` 或 `%TEMP%`。

| 車道 | 用途 | 產物 |
| ------ | ------ | ------ |
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
| ------ | -------------- | ------------------------ |
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

## Markdown 檢查

```powershell
pwsh dev/lint-docs.ps1          # 只檢查專案維護的 Docs/、README、TODO、dev、ui
pwsh dev/lint-docs.ps1 -Fix     # 自動修正可修的格式問題
```

`MD024` 使用 `siblings_only`：`ChangeHistory` 在不同日期小節下重複「問題／修改／驗證」是刻意的。`MD024`、`MD040`、`MD045` 不是 autofix 能安全處理的規則，需人工判斷。

## 本機部署到固定 Docker 容器

```powershell
pwsh dev/local-deploy.ps1            # 建置 image + 回報 revision／version
pwsh dev/local-deploy.ps1 -Check     # 只看狀態，不建置
pwsh dev/local-deploy.ps1 -Deploy    # 建置並 force-recreate 容器
pwsh dev/local-deploy.ps1 -Deploy -Test  # 加上 HTTP／RTMP smoke

## 共用 smoke test

Nightly 與 Release 都呼叫 `dev/smoke-test.ps1`，不再各自維護一份 PowerShell：

```powershell
pwsh dev/smoke-test.ps1 `
  -SrsExe trunk/ide/srs_msvc/build/Release/srs.exe `
  -ConfFile trunk/ide/srs_msvc/build/Release/test.conf `
  -WorkDir trunk/ide/srs_msvc/build/Release `
  -CheckConsole
```

它會啟動 SRS、等待 API、檢查 RTMP handshake、`/console/`（選用）與 `/api/v1/config`，最後停止 process。預設 API／RTMP 埠是 `1985`／`19350`，本機埠被占用時用 `-ApiPort`／`-RtmpPort` 覆寫。

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

## RTC 媒體 E2E

需要 full（RTC／SRT）建置與已建置的 console：

```powershell
pwsh dev/build-ui.ps1 -Target full
pwsh dev/rtc-e2e.ps1 `
  -SrsExe trunk/ide/srs_msvc/build-full/Debug/srs.exe `
  -ConfFile dev/windows/dev.conf `
  -WorkDir trunk/ide/srs_msvc/build-full/Debug
```

腳本會啟動 SRS、用 `ui/e2e/rtc-whip.html` 發佈 canvas H.264 串流，再從 console 的 WHEP 播放器播放，確認 `videoWidth` > 0 且 API 仍可回應。需要 Chrome 或 Edge（`playwright-core` 會依序嘗試）。

## Console 認證（可選）

一般開發的 `dev/windows/dev.conf` 不啟用 auth。要測認證時：

```powershell
. .\dev\dev-auth.ps1          # 產生／沿用 dev/runtime/dev-password.txt，並設定 SRS_HTTP_API_AUTH_PASSWORD
pwsh dev/auth-e2e.ps1 `
  -SrsExe trunk/ide/srs_msvc/build-full/Debug/srs.exe `
  -WorkDir trunk/ide/srs_msvc/build-full/Debug
```

`dev/windows/auth.conf` 只啟用 `http_api.auth`，不寫死帳密；第一次啟動會由 SRS 產生 `objs/console-auth.txt`，或由 `SRS_HTTP_API_AUTH_PASSWORD` 覆寫。正式部署請自行保存並限制該檔案讀取權限。

## Docker 建置快取

Docker build context 只包含 Linux 建置需要的 `trunk/` 原始碼：`.dockerignore` 排除 `trunk/ide`（Windows CMake 與本機 build 產物）、`trunk/build`、`trunk/objs`、`ui/node_modules`、`dev/runtime` 等。

Dockerfile 另外使用 BuildKit cache mount：

- `/var/cache/apt`、`/var/lib/apt/lists`：避免 apt 套件重複下載。
- `/root/.ccache`：原始碼變更時只重編受影響的 translation unit，未變更的對象由 ccache 重用。

因此：

- 沒有原始碼變更時，`local-deploy.ps1 -Deploy` 幾乎只重建容器，image layers 全部 cache hit。
- 只有 `trunk/` 變更時，`COPY trunk` 會失效，但 configure／make 會利用 ccache；不需要每次重新下載 apt 套件。
- 若 BuildKit cache 累積過大，可定期執行 `docker builder prune`；不要為了一次建置速度而關閉 cache。

## 執行時狀態與持久化

不要把「建置產物」和「需要保留的狀態」混在同一個目錄：

| 路徑 | 類型 | 是否保留 |
|------|------|----------|
| `trunk/ide/srs_msvc/build*/`、`ui/build/` | 建置產物 | ❌ 可重建 |
| `objs/srs.pid` | 程序狀態 | ❌ 重啟即過期 |
| `objs/nginx/html/console/` | 靜態控制台 | ❌ 由建置產生 |
| `objs/nginx/html/hls/` | HLS 輸出 | ✅ 需要保留（Docker 已掛載到 `dev/runtime/docker/hls`） |
| `objs/console-auth.txt` | 初始認證 | ✅ 需要保留；建議用 `SRS_CONSOLE_AUTH_FILE` 指到固定路徑 |
| `srs_log_file`（若啟用） | 日誌 | 依需求保留；Docker 預設用 console |
| `dvr_path`（若啟用） | 錄影 | ✅ 需要保留 |
| `dev/runtime/` | 本機執行時狀態 | ✅ 不進版控 |

Docker lane 已把 HLS 掛到 `dev/runtime/docker/hls`，並把可選的認證狀態放在 `dev/runtime/docker/state/console-auth.txt`。`dev.conf` 預設不啟用 auth，所以一般 dev 容器不會建立這個檔案。

## Windows 車道

`-Build` 會：

1. 若沒有 `CMakeCache.txt` 就 configure（Visual Studio 17 2022 / x64）。
2. 建置 `trunk/ide/srs_msvc/build` 的 Debug。
3. 複製 `srs.exe` → `dev/runtime/windows/srs-dev.exe`，連同 `srs.pdb`、OpenSSL DLL。
4. 建立可寫的 `objs/`，使用 `dev/windows/dev.conf`。

日誌：`dev/runtime/windows/srs-dev.out.log`、`srs-dev.err.log`；pid：`srs-dev.pid`。

HLS 輸出：`dev/runtime/windows/objs/nginx/html/hls/<app>/`。`hls_path` 是相對於
`dev/runtime/windows` 工作目錄，不是 `%TEMP%`；直接用 `srs-dev.ps1` 啟動就不會
把 HLS 散落到系統暫存。

## Docker 車道

HLS 輸出會 bind mount 到 `dev/runtime/docker/hls/<app>/`（容器內是
`/srs/trunk/objs/nginx/html/hls`），可直接從 repo 找到；不會留在 container
內部或 Windows 暫存目錄。

`-Build` 會用 `dev/docker/Dockerfile` 從本地 `trunk/` 建置**上游 Linux 版**（Ubuntu 24.04），不是 Windows 移植版。映像刻意使用發行版套件而非 vendored 舊版：OpenSSL 3、libsrtp 2.5+、SRT 1.5+、FFmpeg 6.1+，configure 帶 `--use-sys-srtp=on --use-sys-srt=on --use-sys-ffmpeg=on`。

`-Run` 以 `dev/docker/dev.conf` 啟動，並把主機的 11935／11985／18088（TCP）與 11980／18000（UDP）對到容器的 1935／1985／8088／10080／8000。啟動日誌應看到：

```text
RTMP listen at tcp://0.0.0.0:1935
HTTP-API listen at tcp://0.0.0.0:1985
srt listen at udp://0.0.0.0:10080
WebRTC listen at udp://0.0.0.0:8000
```

原始碼變更後要重新 `-Build`；只改 `dev/docker/dev.conf` 時，`-Stop` 後 `-Run` 即可（設定檔以唯讀 volume 掛入）。

> Windows 車道的 RTC／SRT 預設關閉；要測試全功能，使用 vcpkg 與 `windows-full` preset（見下方）。Docker 車道已預設啟用兩者。

## Windows 全功能（RTC／SRT）

```powershell
# 建立 .vscode/vcpkg junction；自動找 vcpkg，必要時加 -VcpkgRoot
pwsh dev/setup-vscode.ps1
& .vscode\vcpkg\vcpkg.exe install libsrtp libsrt opus --triplet x64-windows

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
| ------ | ------ |
| `srs-dev.exe is missing` | 先跑 `-Build` |
| `code=1034 PidFileAcquire` | `dev/runtime/windows/objs` 不存在或殘留 `srs.pid`；重跑 `-Build` 或刪除該 pid |
| 11985 已被占用 | 改 `dev/windows/dev.conf` 與 `dev/srs-dev.ps1` 的 `ApiUrl` |
| Docker 建置失敗 | 確認 Docker Desktop 已啟動；`docker compose -f dev/docker/docker-compose.yml build` 看完整日誌 |
| 兩個車道同時跑 | 不行，主機埠相同；先 `-Stop` 再切換 |
