# SRS-Win (Simple Realtime Server — Windows Native Port)

[![SRS Official](https://img.shields.io/badge/SRS%20Official-ossrs/srs-blue)](https://github.com/ossrs/srs)
[![Platform Windows x64](https://img.shields.io/badge/Platform-Windows%20x64-success)](https://github.com/TwhomeGH/srs-win)

**SRS-Win** 是 [SRS](https://github.com/ossrs/srs) 即時影音串流伺服器的 **Windows 原生移植版**。
使用 **MSVC (Visual Studio 2022)** + **CMake** 建置，支援 **Windows 10/11 x64**。

> ⚠️ **官方 SRS 的 Windows 支援是 Cygwin64（自 v5.0.89）；官方在 v7.0.60（2025-08-21，#4447）正式停止支援，並實際移除 Cygwin64／Windows 實作、建置設定與 CI**；不是只停止維護、也不是 v7.0.60 之後才移除。
> 此 Fork 以 **SRS v7.0.89（2025-09-15）** 為基礎，重新建立原生 MSVC/CMake Windows port；官方 SRS 7 本身不提供 Windows 原生支援。
>
> HTTP API 認證是**可選**的（預設關閉）。啟用但未設定帳密時，SRS 會產生隨機初始密碼並寫入 `objs/console-auth.txt`；請勿使用 `admin/admin`。

## 支援的協議

| 協議 | 預設建置 | 全功能建置 | 說明 |
| --- | --- | --- | --- |
| **RTMP** | ✅ | ✅ | 發佈／播放 |
| **HTTP-FLV / HTTP-TS** | ✅ | ✅ | HTTP 串流 |
| **HLS** | ✅ | ✅ | HTTP Live Streaming |
| **WebRTC (WHIP/WHEP)** | — | ✅ | 需 `windows-full`＋vcpkg `libsrtp`／`opus`；WHIP 發佈／WHEP 播放已納入 RTC E2E |
| **SRT** | — | ✅ | 需 `windows-full`＋vcpkg `libsrt` |
| **GB28181** | △ | △ | 巨集已啟用；UDP／媒體路徑尚未端到端驗證 |
| **MPEG-DASH** | ✅ | ✅ | HTTP 自適應串流 |
| **DVR** | ✅ | ✅ | 錄影至 FLV/MP4 |

> 「預設建置」是沒有 vcpkg 的 `windows-default`（RTC／SRT 關閉）；「全功能建置」是 `windows-full`。README 不把未啟用的功能標成預設可用。

## 快速開始

### 前置需求

- Windows 10/11 x64
- Visual Studio 2022 (含 MSVC v143)
- CMake 3.15+
- [vcpkg](https://github.com/Microsoft/vcpkg)（位置不限；先執行 `pwsh dev/setup-vscode.ps1` 建立 `.vscode/vcpkg`）

### 一鍵建置

```powershell
# 建置
.\rebuild.ps1

# 建置並執行
.\rebuild.ps1 -Run

# 停止
.\stop.ps1
```

### 執行

```powershell
.\trunk\ide\srs_msvc\build\Debug\srs.exe -c .\trunk\conf\test.conf
```

驗證服務是否正常：

```powershell
curl http://localhost:1985/api/v1/versions
```

推流測試（需另裝 FFmpeg）：

```powershell
ffmpeg -re -i video.mp4 -c copy -f flv rtmp://localhost/live/stream
```

## 建置方式

詳細建置說明請參考 [Docs/建置與執行說明.md](Docs/建置與執行說明.md)。

```powershell
# CMake Configure（先執行 pwsh dev/setup-vscode.ps1）
cmake -B trunk/ide/srs_msvc/build -S trunk/ide/srs_msvc `
      -G "Visual Studio 17 2022" -A x64 `
      -DCMAKE_TOOLCHAIN_FILE=".vscode/vcpkg/scripts/buildsystems/vcpkg.cmake"

# Build
cmake --build trunk/ide/srs_msvc/build --config Debug
```

## 專案文件

| 文件 | 說明 |
| ------ | ------ |
| [Docs/文件索引.md](Docs/文件索引.md) | **文件單一入口**：最短路徑、文件地圖、維護規則 |
| [Docs/項目概覽說明.md](Docs/項目概覽說明.md) | 專案簡介、目錄結構、移植範圍 |
| [Docs/Windows移植技術文件.md](Docs/Windows移植技術文件.md) | CMake 系統、POSIX 相容層、st-srs 移植細節 |
| [Docs/組件盤點與技術債.md](Docs/組件盤點與技術債.md) | 目前使用的組件、版本、功能現況與需重新設計的項目 |
| [ui/README.md](ui/README.md) | Web 控制台快速入口；完整說明見 [Docs/Web控制台.md](Docs/Web控制台.md) |
| [Docs/建置與執行說明.md](Docs/建置與執行說明.md) | 環境需求、建置步驟、設定、驗證 |
| [Docs/VSCode除錯手冊.md](Docs/VSCode除錯手冊.md) | VSCode 中斷點、`launch.json`／`tasks.json`、生產版 `.exe` 偵錯 |
| [Docs/已知限制與待辦事項.md](Docs/已知限制與待辦事項.md) | 已知限制與改善建議 |

API 參考由 Doxygen 從原始碼註解產生，Pages 上的路徑是 `<Pages 網址>/api/`；本機請執行 `doxygen Doxyfile`。

## 已知限制

- **無 daemon 模式** — 只能 foreground 執行
- **無 `fork()`** — 無法 spawn 轉碼/ingest 子行程
- **無 inotify** — 設定檔 hot-reload 不可用
- **I/O 使用 `select()`** — 非 `epoll`，高併連線效能受限
- **FFmpeg 內部 API stub** — 無法內部轉碼，但可外部 spawn `ffmpeg.exe`
- **僅 x64** — 無 x86 或 ARM64 支援

詳細說明請見 [Docs/已知限制與待辦事項.md](Docs/已知限制與待辦事項.md)。

## 授權

SRS is licenced under [MIT](https://github.com/ossrs/srs/blob/develop/LICENSE).

---

Base on [SRS](https://github.com/ossrs/srs) (Simple Realtime Server) by Winlin and contributors.
Windows port maintained separately.
