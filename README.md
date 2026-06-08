# SRS-Win (Simple Realtime Server — Windows Native Port)

[![](https://img.shields.io/badge/SRS%20Official-ossrs/srs-blue)](https://github.com/ossrs/srs)
[![](https://img.shields.io/badge/Platform-Windows%20x64-success)]()

**SRS-Win** 是 [SRS](https://github.com/ossrs/srs) 即時影音串流伺服器的 **Windows 原生移植版**。
使用 **MSVC (Visual Studio 2022)** + **CMake** 建置，支援 **Windows 10/11 x64**。

> ⚠️ **官方 SRS 已在 v7.0.60+ 終止 Windows 支援**（原因：維護成本過高）。
> 此 Fork 為獨立維護的 **Windows 原生版本**，持續支援 Windows 平台。

## 支援的協議

| 協議 | 狀態 | 說明 |
|------|------|------|
| **RTMP** | ✅ | 發佈／播放，SRT↔RTMP 橋接 |
| **HTTP-FLV / HTTP-TS** | ✅ | HTTP 串流 |
| **HLS** | ✅ | HTTP Live Streaming |
| **WebRTC (WHIP/WHEP)** | ✅ | RTC over UDP/TCP |
| **SRT** | ✅ | 可靠 UDP 傳輸 |
| **GB28181** | ✅ | 中國安防監控 |
| **MPEG-DASH** | ✅ | HTTP 自適應串流 |
| **DVR** | ✅ | 錄影至 FLV/MP4 |

## 快速開始

### 前置需求

- Windows 10/11 x64
- Visual Studio 2022 (含 MSVC v143)
- CMake 3.15+
- [vcpkg](https://github.com/Microsoft/vcpkg)（建議安裝於 `C:\vcpkg`）

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
curl http://localhost:8080/api/v1/versions
```

推流測試（需另裝 FFmpeg）：

```powershell
ffmpeg -re -i video.mp4 -c copy -f flv rtmp://localhost/live/stream
```

## 建置方式

詳細建置說明請參考 [Docs/建置與執行說明.md](Docs/建置與執行說明.md)。

```powershell
# CMake Configure
cmake -B trunk/ide/srs_msvc/build -S trunk/ide/srs_msvc `
      -G "Visual Studio 17 2022" -A x64 `
      -DCMAKE_TOOLCHAIN_FILE="C:\vcpkg\scripts\buildsystems\vcpkg.cmake"

# Build
cmake --build trunk/ide/srs_msvc/build --config Debug
```

## 專案文件

| 文件 | 說明 |
|------|------|
| [Docs/項目概覽說明.md](Docs/項目概覽說明.md) | 專案簡介、目錄結構、移植範圍 |
| [Docs/Windows移植技術文件.md](Docs/Windows移植技術文件.md) | CMake 系統、POSIX 相容層、st-srs 移植細節 |
| [Docs/建置與執行說明.md](Docs/建置與執行說明.md) | 環境需求、建置步驟、設定、驗證 |
| [Docs/已知限制與待辦事項.md](Docs/已知限制與待辦事項.md) | 已知限制與改善建議 |

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
