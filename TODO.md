# TODO — SRS-Win 未完成事項

主待辦清單（changelogkit 會解析本檔的 `- [ ]`／`- [x]`）。完成後勾選，不要另開副本。

## 設計檢查（進行中）

### P0 平台契約與跨平台建置

- [x] Source list 一致性檢查：比對 `trunk/src/app/*.cpp`、`trunk/configure` 與 CMake
- [x] Linux build CI（workflow 已加入；run `43743234425` 全綠）
- [x] Add source-list consistency check to `dev/check.ps1` and CI
- [x] Feature flags 單一來源：共用 `srs_feature_flags.hpp`，Windows／Linux 輸入正規化，並在 `/api/v1/config` 提供 capabilities 自檢
- [ ] 撰寫 `Docs/平台移植契約.md`（Build／OS／ABI contract、capability matrix）
- [ ] Platform capability tests（file replace、share-delete、poll、pid lock、spawn、time）
- [ ] 合併 `force_include.h`／`compat/`／`srs_win32_compat.h` 重疊（後者為死碼）
- [ ] macOS 建置原型（POSIX adapter + Darwin override）
- [ ] Linux／Windows ARM64 建置原型與 st context-switch probe

- [ ] 逐模組檢查 Windows 相容層的設計缺口（`force_include.h`、`srs_win32_compat.h`、`compat/`）
- [ ] 每次修改區塊同步補齊公開 API 的 Doxygen 註解（change-scoped，不做全域大改）
- [ ] 確認 POSIX 相容層的錯誤碼轉換一致（`errno` ↔ `GetLastError()`）
- [x] 已使用的 POSIX stub fail-loud（`fcntl`／`epoll`／`inotify`／`asprocess`／`inotify_auto_reload`）
- [x] 檢查 socket／`select()` 事件層在 `FD_SETSIZE` 邊界（已修正 ST 閒置執行緒堆疊不足與 context 堆疊指標）
- [ ] 檢查連線關閉流程（fd 回收、half-close、`select()` 例外集）
- [x] 檢查 pid 檔與服務生命週期的設計（Windows 改用 `LockFileEx`，stale pid 可重啟）

## 開發教學

- [x] Learn 1 教材：建置、啟動、Docker、VSCode 入口（`Docs/development/tutorial/`）
- [x] Learn 1 在乾淨 worktree 從頭實跑，修正文件與指令落差
- [x] Worktree 安全清理指引與 `dev/remove-worktree.ps1`（回收站優先）
- [ ] Learn 2：新增唯讀 API 欄位（含 Doxygen／console 驗證）
- [ ] Learn 3：VSCode 除錯工作流（launch／tasks／attach／Docker 邊界）
- [ ] Learn 4：新增 console 頁面（React／Vite）

## CI／發佈

- [x] Nightly 改為每晚排程＋手動觸發；有 commit 變更才建置，push 不再觸發
- [x] Smoke test 設定檔與埠號一致（`test.conf`／1985）
- [x] 建置輸出含 OpenSSL DLL、`test.conf`、`objs/`（artifact 可直接執行）
- [x] Smoke test 增加 RTMP handshake（HTTP API 之外）
- [x] Nightly 執行 `st-event-loop-test`（`ctest -R st-event-loop-test`）
- [x] Windows RTC／SRT 依賴（vcpkg `libsrtp`／`libsrt`／`opus`）納入 `windows-full` preset
- [x] SRT 推流 Windows 端到端驗證（ffmpeg → SRS，H264/320x240）
- [x] RTC 媒體（WHIP／WHEP）Windows 端到端驗證（`dev/rtc-e2e.ps1`）
- [x] 全功能（RTC／SRT）CI job 與 vcpkg 快取（run `43724683576` 通過）
- [x] Nightly 產出 Release artifact（workflow 已改 Release；手動 run 43654341813 驗證通過）
- [x] Nightly 與 Release 共用同一份 smoke test 腳本（`dev/smoke-test.ps1`）

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
- [x] 唯讀 `/api/v1/config` 與設定頁（listeners／limits／features，不再依賴 raw API）
- [x] 推流診斷（HTTP-FLV 標籤串流、位元率、封包、關鍵影格／GOP）
- [x] Web 控制台離線時保留最後資料，只顯示警告卡＋重試
- [x] UI 單元測試（Vitest：FLV 解析、API 錯誤分類）
- [x] Web 控制台：RTC（WHEP）／HTTP-FLV／HLS 播放器
- [x] 可選 HTTP API Basic Auth＋console token 登入（7 天、自動續期、初次產生密碼；dev 專用密碼）
- [ ] Web 控制台：SRT 播放器、多使用者／權限管理、Playwright 全頁 E2E
- [ ] 選擇性接入 `trunk/src/utest` 的 protocol／kernel 測試
- [x] 首次部署後確認 Pages 網址可瀏覽、文件分組正確

## 已知限制改善

- [ ] 認證升級：HttpOnly／SameSite cookie session（含 CSRF 防護、HTTPS 時 Secure、伺服器端撤銷）

- [ ] 以 `CreateProcess()` 實作行程管理（讓 exec／ingest 可用）
- [x] 以 `SetConsoleCtrlHandler()` 取代 POSIX 訊號 stub（優雅關閉）
- [ ] 評估以 IOCP 取代 `select()` 的可行性
- [ ] 支援 ARM64 Windows（需 `md_win64_arm64.asm` 與 vcpkg 驗證）
- [ ] 減少 `force_include.h` 與 `srs_win32_compat.h` 的重複定義（後者目前未被 include）
- [x] `rebuild.ps1` 改用相對路徑
- [ ] 移除 dead 第三方元件（`httpx-static`、`srs-docs`、`gprof`、`gperftools`、`signaling`）
- [ ] OpenSSL：移除 vendored 1.1.1l，統一使用系統 3.x
- [ ] FFmpeg：Windows 改接真實 FFmpeg 7.x 或明確改為外部 `ffmpeg.exe`；Linux 使用系統 6.x
- [x] CMakePresets 補 CI 用 `windows-full` job（`.github/workflows/windows-full.yml`）

## 文件

- [ ] `Docs/` 四份文件與現況對齊（埠號、路徑、CI 行為）
- [ ] `README.md` 補上建置／執行的最短步驟與 Pages 連結
- [x] VSCode 除錯手冊（含生產版 `.exe`、`launch.json`／`tasks.json`）
- [x] Phase 1 結案文件（`Docs/Phase1結案.md`）
