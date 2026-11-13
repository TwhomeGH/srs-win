# TODO — SRS-Win 未完成事項

主待辦清單（changelogkit 會解析本檔的 `- [ ]`／`- [x]`）。完成後勾選，不要另開副本。

## 設計檢查（進行中）

- [ ] 逐模組檢查 Windows 相容層的設計缺口（`force_include.h`、`srs_win32_compat.h`、`compat/`）
- [ ] 確認 POSIX 相容層的錯誤碼轉換一致（`errno` ↔ `GetLastError()`）
- [x] 檢查 socket／`select()` 事件層在 `FD_SETSIZE` 邊界（已修正 ST 閒置執行緒堆疊不足與 context 堆疊指標）
- [ ] 檢查連線關閉流程（fd 回收、half-close、`select()` 例外集）
- [ ] 檢查 pid 檔與服務生命週期的設計（避免殘留 pid 造成啟動失敗）

## CI／發佈

- [x] Nightly 改為僅 push 觸發（移除固定排程）
- [x] Smoke test 設定檔與埠號一致（`test.conf`／1985）
- [ ] Smoke test 增加 RTMP 推拉流驗證（目前只驗 HTTP API）
- [ ] Nightly 與 Release 共用同一份 smoke test 腳本（避免兩份漂移）

## 工具鏈

- [x] 接入 changelogkit（`.changelog.toml` ＋ `Docs/ChangeHistory.md`）
- [x] 建立主 `TODO.md`
- [x] 新增 Pages workflow（`changelogkit export` → GitHub Pages）
- [ ] 首次部署後確認 Pages 網址可瀏覽、文件分組正確

## 已知限制改善

- [ ] 以 `CreateProcess()` 實作行程管理（讓 exec／ingest 可用）
- [x] 以 `SetConsoleCtrlHandler()` 取代 POSIX 訊號 stub（優雅關閉）
- [ ] 評估以 IOCP 取代 `select()` 的可行性
- [ ] 支援 ARM64 Windows（需 `md_win64_arm64.asm` 與 vcpkg 驗證）
- [ ] 減少 `force_include.h` 與 `srs_win32_compat.h` 的重複定義
- [ ] `rebuild.ps1` 改用相對路徑

## 文件

- [ ] `Docs/` 四份文件與現況對齊（埠號、路徑、CI 行為）
- [ ] `README.md` 補上建置／執行的最短步驟與 Pages 連結
