# Phase 1 結案：Windows 平台語意硬化

**結論**：Phase 1 的關鍵平台語意已達到「可以開始做功能」的最小門檻；尚未完成的是測試深度與非阻擋性的平台重構，不應再無限期阻擋功能開發。

## 1. 出口條件與證據

| 條件 | 狀態 | 證據 |
|------|------|------|
| Debug 建置與 4 個 native tests | ✅ | `dev/check.ps1` 4/4 |
| Full（RTC／SRT）建置與 ctest | ✅ | `dev/check.ps1 -Full` 4/4（145.6s） |
| Release Nightly | ✅ | run `43654341813` |
| Windows Full CI＋vcpkg cache | ✅ | run `43724683576`（12m10s） |
| 共用 smoke test | ✅ | run `43656170128`；`dev/smoke-test.ps1` |
| st context switch ABI | ✅ | `st_abi_probe.asm` mask=0（Debug／Release） |
| HLS 檔案 rename／share | ✅ | `path-rename-test`、`file-share-test` |
| PID 單實例鎖 | ✅ | `LockFileEx`；重複啟動被拒 |
| POSIX stub fail-loud | ✅ | `win-compat-test`；`asprocess`／`inotify_auto_reload` 設定拒絕 |
| 建置資訊與功能宣稱 | ✅ | `/api/v1/config` 實際 build date；README 分預設／full |
| 上游脈絡與不支援清單 | ✅ | `Docs/Windows移植技術文件.md` |

## 2. 已修正的關鍵問題

- st context switch 漏存 RDI／RSI、XMM15 與 MXCSR 重疊；補 ABI probe。
- HLS 在 Windows 上 rename 到既有檔失敗；改用 `MoveFileEx`／兩段式替換。
- HTTP 讀取中的 m3u8 無法被替換；open 加上 `FILE_SHARE_DELETE`。
- `fcntl(F_SETLK)` 在 Windows 是 no-op；PID 鎖改 `LockFileEx`。
- `fork`／`kill`／`inotify`／`epoll`／`fcntl` 等 stub 不再假成功。
- `SRS_BUILD_DATE` 不再固定為 `2026-06-05`。
- Nightly／Release 不再各自維護 smoke。

## 3. 明確延後，不阻擋功能

> Phase 2 第一項「RTC 媒體 E2E（WHIP／WHEP）＋ console 播放器」已完成，見 `dev/rtc-e2e.ps1` 與 Windows Full CI `43729141759`。

| 項目 | 延後理由 | 何時處理 |
|------|----------|----------|
| 選擇性接入 `trunk/src/utest` | app／SRT／RTC 耦合深，gtest harness 大 | 有具體 protocol／kernel 回歸需求時 |
| `CreateProcess` 行程管理 | 與 exec／ingest 功能綁定 | 做 process／ingest 功能時 |
| IOCP 取代 `select()` | 目前沒有量測到瓶頸 | 有延遲／併發量測證據時 |
| ARM64 Windows | 需求未出現；需另一份 asm | 有 ARM64 使用者時 |
| FFmpeg 7.x／真轉碼 | 目前是 stub；功能未在範圍 | 做轉碼／內部 ingest 時 |
| 移除 dead 第三方元件 | 不影響 runtime | 獨立清理批次 |
| `srs_win32_compat.h` 重複定義 | 未被 include，屬整理債 | 下一輪結構清理 |
| 連線關閉流程細節 | 尚無實際事故；需專門測試 | 有關閉／fd 洩漏證據時 |

## 4. 功能開發護欄

1. 一次一個 feature branch，不順手重寫平台層。
2. 每個功能至少一個回歸測試：
   - C++：`dev/tests/`
   - Web：`ui/tests/`
   - 端到端：擴充 `dev/smoke-test.ps1`
3. 碰到平台缺口才回頭修 foundation，並補測試。
4. RTC／SRT 功能前先補對應媒體 E2E。
5. 新 stub 不得回 `0` 假裝成功；平台限制要有 `@note`。
6. 不改 `compat/` 的失敗語意，除非同時更新測試與 `Docs/Windows移植技術文件.md`。

## 5. Phase 2 建議順序

```text
1. RTC 媒體 E2E（WHIP／WHEP）＋ console 播放器（已完成）
2. HLS／DASH／DVR 功能與設定改進
3. Web 控制台：Basic Auth、Playwright E2E
4. 行程管理／ingest（CreateProcess）
5. 平台重構（IOCP／ARM64／FFmpeg）依需求排入
```

## 6. 版本標記

目前**沒有**建立 tag。Release workflow 會對 `v1*` tag 觸發正式發佈，因此 tag 應在有明確版本內容與 release notes 時再打；Phase 1 結案不代表已經發佈。
