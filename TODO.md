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
- [x] 建置輸出含 OpenSSL DLL、`test.conf`、`objs/`（artifact 可直接執行）
- [x] Smoke test 增加 RTMP handshake（HTTP API 之外）
- [x] Nightly 執行 `st-event-loop-test`（`ctest -R st-event-loop-test`）
- [x] Windows RTC／SRT 依賴（vcpkg `libsrtp`／`libsrt`／`opus`）納入 `windows-full` preset
- [x] SRT 推流 Windows 端到端驗證（ffmpeg → SRS，H264/320x240）
- [ ] RTC 媒體（WHIP／WHEP）Windows 端到端驗證
- [ ] 全功能（RTC／SRT）CI job 與 vcpkg 快取
- [ ] Nightly 產出 Release artifact（避免 Debug CRT 相依）
- [ ] Nightly 與 Release 共用同一份 smoke test 腳本（避免兩份漂移）

## 工具鏈

- [x] 接入 changelogkit（`.changelog.toml` ＋ `Docs/ChangeHistory.md`）
- [x] 建立主 `TODO.md`
- [x] 新增 Pages workflow（`changelogkit export` → GitHub Pages）
- [x] 專屬 dev 測試環境（Windows `srs-dev.exe` ＋ Docker Linux lane，執行時產物在 `dev/runtime/`）
- [x] Docker lane 改用系統 libsrtp／SRT／FFmpeg 6／OpenSSL 3（Ubuntu 24.04）
- [x] st-srs 事件迴圈回歸測試（context switch＋select round-trip＋協程內例外）
- [x] Windows 全功能建置（vcpkg＋`windows-full` preset）
- [x] Doxygen API 參考（`Doxyfile` → Pages `/api/`）
- [x] 文件單一入口（`Docs/文件索引.md`）
- [x] 固定 dev 容器部署腳本（`dev/local-deploy.ps1` ＋ `local-deploy.ini`）
- [x] 快速增量檢查（`dev/check.ps1`）與本機 API 產生（`dev/build-docs.ps1`）
- [x] Windows VSCode launch／tasks／IntelliSense 設定
- [x] Web 控制台 MVP（React/Vite，Overview／Vhosts／Streams／Clients／Configs）
- [ ] Web 控制台：RTC／SRT 播放器、Basic Auth 登入、Configs raw API 降級提示
- [x] 首次部署後確認 Pages 網址可瀏覽、文件分組正確

## 已知限制改善

- [ ] 以 `CreateProcess()` 實作行程管理（讓 exec／ingest 可用）
- [x] 以 `SetConsoleCtrlHandler()` 取代 POSIX 訊號 stub（優雅關閉）
- [ ] 評估以 IOCP 取代 `select()` 的可行性
- [ ] 支援 ARM64 Windows（需 `md_win64_arm64.asm` 與 vcpkg 驗證）
- [ ] 減少 `force_include.h` 與 `srs_win32_compat.h` 的重複定義（後者目前未被 include）
- [ ] `rebuild.ps1` 改用相對路徑
- [ ] 移除 dead 第三方元件（`httpx-static`、`srs-docs`、`gprof`、`gperftools`、`signaling`）
- [ ] OpenSSL：移除 vendored 1.1.1l，統一使用系統 3.x
- [ ] FFmpeg：Windows 改接真實 FFmpeg 7.x 或明確改為外部 `ffmpeg.exe`；Linux 使用系統 6.x
- [ ] CMakePresets 補 CI 用 `windows-full` job（不要讓 nightly 依賴本機 `VCPKG_ROOT`）

## 文件

- [ ] `Docs/` 四份文件與現況對齊（埠號、路徑、CI 行為）
- [ ] `README.md` 補上建置／執行的最短步驟與 Pages 連結
- [x] VSCode 除錯手冊（含生產版 `.exe`、`launch.json`／`tasks.json`）
