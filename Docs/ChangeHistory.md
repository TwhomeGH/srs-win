# SRS-Win 變更歷史

最新在最上。格式：`## YYYY.MM.DD HH:MM 一句話標題`，內文分「問題／根因／修改」。

## 2026.11.14 03:30 建置輸出改為可直接執行的組合（OpenSSL DLL、設定、objs）

**類型**: 修復 · **檔案**: `trunk/ide/srs_msvc/CMakeLists.txt`, `Docs/VSCode除錯手冊.md`, `Docs/建置與執行說明.md`, `README.md`

### 問題 - 從 CI artifact 執行 `srs.exe` 時找不到 `libcrypto-3-x64.dll`
下載的 `srs-win-x64-debug` 只有 `srs.exe` 與 `srs.pdb`；啟動即跳出缺少 OpenSSL DLL。輸出目錄也沒有 `test.conf` 與可寫的 `objs/`，無法直接執行。

### 根因 - CMake 只連結 import library，沒有把執行期 DLL 複製到輸出；artifact glob 的 `Debug/*.dll` 因此永遠是空的
OpenSSL 以動態連結方式使用，DLL 留在 OpenSSL 安裝目錄的 `bin`；CI 的 artifact 只上傳 `Debug/*.dll`，沒有東西可收。設定檔只在 configure 階段複製到 build 根目錄，不在 `Debug`。

### 修改 - POST_BUILD 複製對應版本的 OpenSSL DLL 與設定，建立 objs
- `CMakeLists.txt` 依 `OPENSSL_INCLUDE_DIR` 找到 `<prefix>/bin/libcrypto-*.dll`、`libssl-*.dll`，複製到 `$<TARGET_FILE_DIR:srs>`（版本由實際安裝決定，CI 會拿到 3.x、本機 4.x）。
- 同時複製 `trunk/conf/test.conf` 並建立 `objs/`，讓輸出目錄可直接執行。
- 新增 `Docs/VSCode除錯手冊.md`（`cppvsdbg`、launch／tasks、Release exe、attach、常見錯誤）；更新建置文件與 README 的埠號與 bundle 說明。

### 驗證
- 重建後 `build/Debug` 含 `libcrypto-4-x64.dll`、`libssl-4-x64.dll`、`test.conf`、`objs/`。
- 只複製 exe／PDB／DLL／conf 到乾淨目錄執行：HTTP 200、RTMP handshake 16 bytes。
- CI artifact（OpenSSL 3）待推送後下載驗證。

## 2026.11.14 00:30 修正 Windows 事件迴圈：listener 終於會接受連線

**類型**: 修復 · **檔案**: `trunk/3rdparty/st-srs/md.h`, `trunk/3rdparty/st-srs/sched.c`, `trunk/src/app/srs_app_server.cpp`, `trunk/src/app/srs_app_server.hpp`

### 問題 - SRS 啟動後所有監聽埠都不回應，CPU 接近滿載
Nightly 的 smoke test 對 `http://localhost:1985/api/v1/versions` 逾時；本機以獨立埠重現，HTTP 與 RTMP handshake 都無回應。TCP 連線可建立（OS backlog），但應用層從不 accept；程式 CPU 約 90%。

### 根因 - 三個 Windows 移植缺陷疊在一起
- `st` 的 `MD_GET_SP` 用 32 位元 `long` 索引 64 位元的 context，堆疊指標寫到錯誤位移，協程的初始 RSP 指回建立者的堆疊或 0。
- `_st_select_dispatch` 在堆疊上放三份 `fd_set`；`FD_SETSIZE=8192` 時每份約 64KB，預設 64KB 的閒置執行緒堆疊一執行 select 就溢位。
- 訊號管理器輪詢 CRT pipe，但 ST 以 `recv()` 讀所有 fd；讀取永遠失敗且回傳值被忽略，於是拿未初始化的 `signo` 空轉，餓死排程器。

### 修改 - 修正堆疊指標與堆疊大小，Windows 改用主控台事件
- `md.h` 以 `long long` 索引 `__jmpbuf[6]`；`sched.c` 的 SP 轉型改為 `long long`。
- 閒置執行緒堆疊提高到 256KB（容納三份 `fd_set` 與排程 frames）。
- Windows 的 `SrsSignalManager` 改用 `SetConsoleCtrlHandler()`，`cycle()` 不再輪詢 pipe；POSIX 路徑仍讀 pipe，並檢查 `srs_read` 回傳值。

### 驗證
- 本機以 11985/11935 獨立埠測試：HTTP API 連續 3 次 200、RTMP handshake 回傳 16 bytes、9 秒內 CPU 僅 0.14 秒。
- Nightly smoke test 已於 push 後通過（run `43460487604`，含 Build 與 Smoke test）。

## 2026.11.13 14:30 接入 changelogkit、建立主待辦與 Pages 靜態站

**類型**: 工具鏈 · **檔案**: `.changelog.toml`, `TODO.md`, `.github/workflows/pages.yml`

### 問題 - 變更紀錄與待辦散落各處，沒有可瀏覽的歷史與單一入口
CI 修正的來龍去脈只存在 commit 訊息與對話裡；待辦同時散落在 `Docs/已知限制與待辦事項.md` 與臨時筆記。

### 根因 - 缺乏版本控管內的紀錄格式與索引工具
專案沒有 `Docs/ChangeHistory.md`，也沒有把待辦集中成一份可解析的清單；文件檢視只能靠 GitHub 逐檔翻。

### 修改 - 以 changelogkit 作為歷史與待辦的單一入口
- 新增 `.changelog.toml`：指定歷史檔、`TODO.md`，並把 `Docs/` 分成「建置與使用」「設計與限制」兩組。
- 新增 `Docs/ChangeHistory.md`（本檔）作為變更紀錄來源。
- 新增 `TODO.md` 作為主待辦清單，changelogkit 解析 `- [ ]`／`- [x]`。
- 新增 `.github/workflows/pages.yml`：push `develop` 時匯出唯讀靜態站並部署 GitHub Pages。
- Pages 安裝 changelogkit 時解析最新 tag，不釘死版本；需要可重現時用 repo 變數 `CHANGELOGKIT_REF` 覆寫。

---

## 2026.11.13 14:10 修正 Nightly smoke test 的設定檔與埠號不一致

**類型**: 修復 · **檔案**: `.github/workflows/windows-nightly.yml`, `.github/workflows/release.yml`, `trunk/conf/test.conf`

### 問題 - Smoke test 呼叫 `localhost:1985`，但 `test.conf` 的 `http_api` 監聽 19850
API 請求逾時或連錯埠，Nightly 在 smoke test 階段失敗。

### 根因 - 設定檔與 workflow 各自維護埠號，沒有單一來源
`test.conf` 使用 19850，workflow 的 URL 曾改為 1985；兩邊沒有互相驗證。

### 修改 - 統一回到原本設計的 1985，兩份 workflow 一起修正
- `test.conf` 的 `http_api listen` 改為 1985。
- `windows-nightly.yml`／`release.yml` 的 URL 統一為 `http://localhost:1985/api/v1/versions`，並延長啟動等待與請求逾時。
- 啟動前清除殘留 `objs/srs.pid` 並確保 `objs/` 目錄存在。

---

## 2026.11.13 14:00 Nightly 改為僅 push 觸發

**類型**: 調整 · **檔案**: `.github/workflows/windows-nightly.yml`

### 問題 - 固定排程每天觸發建置，沒有新推送也重複消耗 CI
排程建置與 push 建置內容重複，失敗時也難以對應到具體變更。

### 根因 - workflow 同時設定 `schedule` 與 `push` 觸發
兩者都會啟動同一份 job。

### 修改 - 移除 `schedule`，只保留 push 與手動觸發
`on` 只剩 `push: branches: [develop]` 與 `workflow_dispatch`。

---
