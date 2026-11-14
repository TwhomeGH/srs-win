# SRS-Win API 導讀 {#mainpage}

**用途**：這個頁面是 Doxygen 的預設首頁，提供中文的模組地圖與閱讀順序；詳細的類別、參數與回傳值仍由原始碼註解產生。

## 怎麼看這份 API

| 想做什麼 | 從哪裡開始 |
|----------|------------|
| 找某個類別／函式 | 右上角搜尋，或左側 `Classes`／`Files` |
| 了解啟動流程 | `SrsServer` → `SrsServer::initialize()` → `run()` → `SrsTcpListener`／`SrsUdpMuxListener` |
| 找 RTMP 連線處理 | `SrsRtmpConn`、`SrsRtmpServer`、`SrsLiveSource` |
| 找 HTTP API／靜態站 | `SrsHttpApi`、`SrsHttpServer`、`SrsHttpStreamServer` |
| 找 RTC／WebRTC | `SrsRtcServer`、`SrsRtcConnection`、`SrsRtcSource` |
| 找 SRT | `SrsSrtServer`、`SrsSrtListener`、`SrsSrtSocket` |
| 找設定解析 | `SrsConfig`、`SrsConfDirective` |
| 找 console／API 的資料格式 | `SrsStatistic`、`SrsHttpApi`（`/api/v1/*`） |

## 模組地圖

| 目錄 | 角色 | 常見入口 |
|------|------|----------|
| `src/core` | 平台、錯誤、時間、記憶體、版本 | `srs_core.hpp`、`srs_core_platform.hpp` |
| `src/kernel` | RTMP／FLV／TS／MP4／PS、buffer、codec、統計 | `SrsLiveSource`、`SrsStatistic`、`SrsKbps` |
| `src/protocol` | RTMP、HTTP、ST、AMF0、RTP／RTCP、SRT、SDP | `SrsRtmpServer`、`SrsHttpParser`、`SrsSrtSocket` |
| `src/app` | server、listener、HLS／DVR、edge、RTC、GB28181、console API | `SrsServer`、`SrsTcpListener`、`SrsRtcServer` |
| `src/main` | 啟動、daemon、pid、訊號 | `srs_main_server.cpp` |
| `src/utest` | 單元測試（目前未接入預設建置） | — |

## 生命週期（簡化）

```text
main()
  └─ SrsServer::initialize()      設定、pid、DTLS、DVR async
  └─ SrsServer::listen()          RTMP／HTTP／SRT／RTC listeners
  └─ SrsServer::run()             ST 協程主迴圈
       ├─ SrsTcpListener::cycle()       accept → handler
       ├─ SrsRtmpConn::cycle()          handshake → publish／play
       ├─ SrsLiveSource                 串流來源與 consumers
       ├─ SrsHttpApi                   /api/v1、/console
       └─ SrsRtcServer／SrsSrtServer    WebRTC／SRT
```

- **協程**：`SrsFastCoroutine` 包住 ST 執行緒；任何阻塞操作都應走 ST 的 I/O，不要直接 sleep。
- **錯誤**：回傳 `srs_error_t`；用 `srs_error_wrap` 加脈絡，不要吞掉。
- **關閉**：Windows 用 `SetConsoleCtrlHandler`，不是 POSIX signal pipe；詳見 `SrsSignalManager`。

## 註解語言策略

| 位置 | 語言 | 理由 |
|------|------|------|
| 類別／函式名稱、參數 | 原文（英文） | 可執行識別字，不能翻譯 |
| 公開 API 的 `@brief`／`@param`／`@return` | **中英雙語**（新寫或改動的程式） | 讓 Doxygen 直接可讀，同時保留上游合併能力 |
| 既有英文註解 | 逐步補中文 `@details` | 不一次大批翻譯，避免 diff 失控 |
| 操作文件、模組導讀 | 繁體中文 | `Docs/` 是主要入口 |

> 不建議只靠 Edge／瀏覽器翻譯：技術名詞、代碼片段與參數名容易被誤譯，且翻譯結果不會進版控。正確做法是把中文放在原始碼註解（雙語）與本導讀頁。

## 相關文件

- `Docs/Web控制台.md`：`/api/v1` 的實際使用者。
- `Docs/Windows移植技術文件.md`：相容層與 st-srs。
- `Docs/組件盤點與技術債.md`：目前依賴與重構方向。
- `Docs/文件索引.md`：所有入口。
