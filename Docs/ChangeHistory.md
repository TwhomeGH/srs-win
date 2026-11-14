# SRS-Win 變更歷史

最新在最上。格式：`## YYYY.MM.DD HH:MM 一句話標題`，內文分「問題／根因／修改」。

## 2026.11.15 09:20 修正 Windows HLS／DASH／設定持久化的 rename 覆寫失敗

**類型**: 修復 · **檔案**: `srs_kernel_file_rename.hpp`, `srs_kernel_utility.cpp`, `srs_app_hls.cpp`, `srs_app_fragment.cpp`, `srs_app_dash.cpp`, `srs_app_config.cpp`

### 問題
- Windows HLS 寫入時出現 `rename ./objs/nginx/html/live/test.m3u8.temp => .../test.m3u8` 失敗（errno=17），接著 segment 的 `test-1.ts.tmp => test-1.ts` 失敗（errno=4），推流被 `ERROR_HLS_WRITE_FAILED`／`ERROR_SYSTEM_FRAGMENT_RENAME` 中斷。

### 根因
- Windows CRT 的 `rename()` 在目的檔已存在時回傳 `EEXIST`，不像 POSIX `rename()` 會原子覆寫。HLS 每次 segment 都重新整理同一個 m3u8，第二次 refresh 就必然失敗；舊的 ts 目標也可能已存在。
- 同樣的寫法也存在於 fMP4 m3u8、DASH MPD、片段 `SrsFragment::rename()` 與設定持久化。

### 修改
- 新增 `srs_path_rename()`（`srs_kernel_file_rename.hpp`）：Windows 用 `MoveFileExA(..., MOVEFILE_REPLACE_EXISTING)`，POSIX 用 `::rename()`；Windows 錯誤碼映射回 `errno`。
- HLS／fMP4／DASH／fragment／config persistence 全部改走這個 helper，不再直接呼叫 CRT `rename`。
- 新增 `dev/tests/path_rename_test.cpp`，覆蓋「目的檔已存在要覆寫」與「來源不存在要失敗」兩種情況，並接入 ctest。

### 驗證
- `ctest`：`st-event-loop-test`、`path-rename-test` 2/2 通過。
- 先放置 stale 的 `hls-rename-test3.m3u8`／`-0.ts` 再推流：m3u8 被更新為有效內容、ts 被覆寫為 315464 bytes，log 無 `rename`／`HlsWriteFile`／`ERROR_HLS`。

## 2026.11.15 08:00 Web 控制台：離線保留、推流診斷、唯讀 config API、單元測試

**類型**: 功能／修復／測試 · **檔案**: `trunk/src/app/srs_app_http_api.*`, `srs_app_server.cpp`, `ui/`

### 問題
- 伺服器離線時，原本的頁面會被錯誤畫面蓋掉，已顯示的資料消失；`ERR_NETWORK` 也沒有清楚的離線說明。
- `/api/v1/raw` 需要管理員開啟，且可寫入；設定頁不應該依賴它。
- 缺少推流診斷，排查 FLV／編碼／GOP 問題只能看伺服器日誌。
- UI 沒有單元測試，FLV 解析與錯誤分類只能靠手動驗證。

### 修改
- 新增 `ApiNotice`：離線時保留原頁面與最後資料，只顯示警告卡、最後更新時間與重試；`api.jsx` 攔截 response-less 錯誤，導覽列同步顯示 `API 離線`。
- 新增唯讀 REST `GET /api/v1/config`：回傳 version／build、listeners（RTMP／HTTP API／HTTP server／SRT／RTC）、enabled、crossdomain、limits、raw API 狀態與 RTC candidates；不含帳密或完整設定文字。
- Config 頁改用 `/api/v1/config`＋`/vhosts/`＋`/features`，raw API 只顯示狀態，不需要開啟。
- 新增推流診斷頁：從 `/api/v1/streams/` 選串流或手動輸入 FLV URL，以 `fetch` 讀取 HTTP-FLV，解析 tag、視訊／音訊 codec、關鍵影格、GOP、位元率與健康狀態；預設 URL 從 config 的 http_server 埠產生。
- 新增 `ui/tests/`（Vitest）：FLV 解析（含 AAC sample rate、AVC config 排除、分段讀取）與 API 錯誤分類。
- Nightly 增加 `npm test` 與 `/api/v1/config` smoke；`dev/check.ps1 -Ui` 可本機跑同一組。

### 驗證
- `npm test`：2 個檔、6 個測試通過；`dev/check.ps1 -Ui`：C++ ctest＋Vitest 通過。
- Playwright：設定頁顯示 version／listeners／features／vhost；推流診斷對 `pushdiag-test` 顯示 H.264、227 封包、271.6 KB、370.7 kbps。
- 停止 SRS 後頁面仍保留串流表，顯示「伺服器離線」與重試；導覽列變 `API 離線`。

## 2026.11.15 07:40 更新 GitHub Actions 到 Node.js 24 版本

**類型**: CI · **檔案**: `.github/workflows/windows-nightly.yml`, `.github/workflows/release.yml`

### 問題
- Nightly 仍使用 `actions/checkout@v4`／`actions/upload-artifact@v4`，GitHub 標記 Node.js 20 deprecated，runner 強制改用 Node.js 24。

### 修改
- `actions/checkout@v4` → `@v7`（`runs.using: node24`）。
- `actions/upload-artifact@v4` → `@v7`（`runs.using: node24`）。
- Release 的 `softprops/action-gh-release@v2` → `@v3`（Node.js 24）。
- Pages 已使用 `checkout@v7`、`setup-python@v7`、`configure-pages@v6`、`upload-pages-artifact@v5`、`deploy-pages@v5`，不需變更。

### 驗證
- 三個 YAML 可解析；Windows Nightly 與 Pages 重新跑過。`test.yml`／`codeql-analysis.yml` 目前是 `workflow_dispatch` 停用狀態，保留舊版 action，不影響自動流程。

## 2026.11.15 07:20 Doxygen 增加中文首頁與雙語 API 註解

**類型**: 文件 · **檔案**: `Doxyfile`, `Docs/API導讀.md`, `srs_app_server.hpp`, `srs_app_rtmp_source.hpp`, `srs_app_rtmp_conn.hpp`

### 問題
- API 站雖然可搜尋，但首頁與類別說明都是英文，維護者需要另外翻譯或猜測模組關係。

### 修改
- `Docs/API導讀.md` 設為 Doxygen 首頁（`USE_MDFILE_AS_MAINPAGE`），提供中文的模組地圖、啟動生命週期、常見入口與註解語言策略。
- `OUTPUT_LANGUAGE` 改為 `Chinese`；`SrsServer`、`SrsLiveSource`、`SrsRtmpConn` 的公開註解加上中文 `@details`，Doxygen 類別頁直接顯示中英雙語。
- 建立慣例：新寫／改動的公開 API 用英文 `@brief` ＋ 中文 `@details`，識別字保持英文；不靠瀏覽器翻譯當唯一方案。

### 驗證
- `dev/build-docs.ps1` 成功，`_site/api/index.html` 標題為「SRS-Win API 導讀」，`_site/api/classSrsServer.html` 同時出現英文 `@brief` 與中文 `@details`；API 導讀的交叉連結不再產生 Doxygen `\ref` 警告。

## 2026.11.15 06:30 新增 SRS Web 控制台（React/Vite），修正程序記憶體指標

**類型**: 功能／修復 · **檔案**: `ui/`, `dev/build-ui.ps1`, `dev/serve-ui.ps1`, `trunk/ide/srs_msvc/compat/sys/resource.h`, `.github/workflows/windows-nightly.yml`

### 問題
- SRS 只有舊的 AngularJS console；Oryx 有完整的 React console 參考，但它綁定 `/terraform/v1`、bearer token、Redis 與 forwarding，不能直接給 SRS 用。
- 總覽的「記憶體」顯示 63GB，因為 Windows 的 `getrusage().ru_maxrss` 回傳機器總實體記憶體，不是程序工作集。

### 根因
- 沒有獨立的 SRS API SPA；舊 console 與新 API 的資料模型脫節。
- `compat/sys/resource.h` 的 `getrusage` 用 `GlobalMemoryStatusEx().ullTotalPhys` 填 `ru_maxrss`，語義與 Linux 的 KB 工作集不同。

### 修改
- 新增 `ui/`（React 18 + Vite 6 + Bootstrap 5）：Overview／Vhosts／Streams／Clients／Configs，同源呼叫 `/api/v1`，支援 zh-TW／en、自動刷新、錯誤邊界與踢除連線；參考 Oryx `SrsConsole.js` 的資料模型但移除平台耦合。
- `dev/serve-ui.ps1`（Vite HMR + `/api` proxy）與 `dev/build-ui.ps1`（建置並安裝到 `objs/nginx/html/console/`）。
- Nightly 建置 console、複製到 SRS 的 `/console/`、加入 artifact，smoke 增加 `/console/` 200 檢查；smoke 改從 `build/Debug` 執行以使用打包的 `test.conf` 與 `objs`。
- `getrusage.ru_maxrss` 改用 `GetProcessMemoryInfo().WorkingSetSize / 1024`，並連結 `psapi`。

### 驗證
- Playwright：`http://127.0.0.1:11985/console/` 載入成功、API 全部 200、無 console error；RTMP 推流後 Streams 顯示 `console-test`（H264 640x352）、可 Kick；語言切換正常。
- 記憶體修正後 `/api/v1/summaries` 的 `mem_kbyte` 由 ~66,451,144 降為 ~27,328。

## 2026.11.15 05:30 新增固定 dev 容器部署、快速檢查腳本與 Windows VSCode 偵錯設定

**類型**: 工具鏈／文件 · **檔案**: `dev/local-deploy.ps1`, `dev/local-deploy.ini.example`, `dev/check.ps1`, `dev/build-docs.ps1`, `.vscode/*`

### 問題
- 改了碼、建了 image，但 dev 容器可能還跑舊版；只能手動 `docker compose up`，無法回答「容器現在是哪個 revision」。
- VSCode 的 `launch.json`／`tasks.json`／`settings.json` 仍是 macOS 設定（`vscode-build`、`cppdbg`、`sclion`），Windows 按 F5 直接失敗；IntelliSense 找不到 `src/core` 等標頭與 vcpkg SRT。
- 每次檢查都要手打完整 CMake／ctest 指令；本機也沒有 API 文件產生入口。

### 根因
- 沒有像 Oryx `local-deploy.ps1` 那樣把 revision／version 寫進 image label 並比對容器狀態的機制。
- 開發工具設定仍是上游 macOS 版本，沒有改成 Windows 的 `cppvsdbg` 與 `srs_msvc` 路徑。
- Doxygen 只在 CI 使用，沒有本機 script。

### 修改
- `dev/local-deploy.ps1`：建 image（label `srs.revision`／`srs.version`）、回報源碼 vs image vs 容器、`-Deploy` force-recreate、`-Test` 打 HTTP／RTMP；設定可存 `dev/local-deploy.ini`（gitignored，附 `.example`）。
- `dev/check.ps1`：增量建置 `srs`＋`st-event-loop-test` 後跑 `ctest`，`-Full` 用 vcpkg preset，`-Docs` 順便更新 API。
- `dev/build-docs.ps1`：有 Doxygen 直接跑，否則用 Ubuntu 容器產生 `_site/api`。
- `.vscode`：Windows 五個 launch 設定（default／full／event-loop test／attach／proxy）、增量 build／test／docs／deploy tasks、CMake 路徑改 `srs_msvc`、IntelliSense 補 `src/*` 與 vcpkg include。

### 驗證
- `pwsh dev/check.ps1`：4.1s，ctest 1/1 passed。
- `pwsh dev/local-deploy.ps1 -Deploy -Test`：image rev 與源碼一致、容器 up to date、HTTP 200、RTMP 16 bytes。
- `local-deploy.ps1 -Check` 在沒有 image／容器時能正確回報。

## 2026.11.15 05:00 修正協程內 C++ 例外、啟用 Windows 全功能 RTC／SRT，補 API 文件產生

**類型**: 修復／功能／文件 · **檔案**: `trunk/3rdparty/st-srs/common.h`, `sched.c`, `trunk/src/protocol/srs_protocol_srt.cpp`, `trunk/ide/srs_msvc/CMakeLists.txt`, `CMakePresets.json`, `Doxyfile`, `.github/workflows/pages.yml`

### 問題
- Windows 全功能建置（libsrtp＋SRT）啟動即退出，`srt_accept` 的例外變成 second-chance exception；SRT 推流也會讓程式消失。
- `st-event-loop-test` 加入「協程內 throw/catch」後同樣以 0xE06D7363 結束。
- 全功能與預設建置共用 `trunk/objs/srs_auto_headers.hpp`，先建全功能再建預設會把 RTC／RTSP 巨集帶進來，連結失敗。
- 手寫文件沒有 API 參考，找類別／函式只能全檔搜尋。

### 根因
- st-srs 手動切換協程堆疊，但沒有同步 TEB 的 `StackBase`／`StackLimit`；MSVC C++ 例外 unwind 會驗證堆疊範圍，因此任何 throw（SRT C API 在非阻塞 accept 無連線時會 throw）都無法被 catch。
- CMake 產生的 feature header 寫在原始碼樹，兩個 build dir 互相覆蓋。

### 修改
- `st-srs` 在 context restore 前更新 TEB 堆疊界線，回到 primordial 時還原；`st-event-loop-test` 加入協程例外測試。
- `SrsSrtSocket::accept` 先等 SRT epoll 事件再呼叫 `srt_accept`，避免常態 no-pending 例外。
- `srs_auto_headers.hpp` 改產生在 `${CMAKE_BINARY_DIR}`，預設與全功能建置互不干擾。
- 新增 `CMakePresets.json`（`windows-default`／`windows-full`）；全功能用 vcpkg `libsrtp` 2.8.1、`libsrt` 1.5.7、`opus` 1.6.1。
- 新增 `Doxyfile` 與 `Docs/文件索引.md`；Pages 在 changelogkit 之後產生 `/api/` Doxygen 站。

### 驗證
- 預設與全功能 `ctest` 皆 1/1 passed（含協程例外）。
- 全功能 Windows：HTTP 200、RTMP 16 bytes、SRT 推流成功（`/api/v1/streams` 顯示 H264 320x240、recv_bytes 29328）。
- Doxygen 本機以 Ubuntu 容器產生 `_site/api/index.html`（`<title>SRS-Win: 首页</title>`）。
- RTC 監聽埠（UDP 19000）已綁定；WHIP／WHEP 媒體路徑尚待驗證。

## 2026.11.15 04:30 Docker 車道改用系統 libsrtp／SRT／FFmpeg 6，補上 st 事件迴圈測試

**類型**: 修復／測試 · **檔案**: `dev/docker/Dockerfile`, `dev/docker/docker-compose.yml`, `dev/tests/st_event_loop_test.cpp`, `trunk/src/core/srs_core.hpp`, `trunk/src/protocol/srs_protocol_st.cpp`, `trunk/src/app/srs_app_rtmp_conn.cpp`, `.github/workflows/windows-nightly.yml`

### 問題 - Linux 車道建不起來，Windows 事件層沒有回歸測試
Windows checkout 的 CRLF 讓 vendored `libsrtp-2-fit/configure` 變成 `/bin/sh^M`；修掉後又連續出現 `SOCKET`／`rtc` 未宣告，因為 Windows 移植的程式碼沒有在 Linux 編譯過。

### 根因
- Dockerfile 仍編譯 vendored 2.3.0 libsrtp（2019），且 `dos2unix` 把它的 `configure` 判定為 binary 而跳過。
- `srs_protocol_st.cpp` 的 `setsockopt((SOCKET)fd, ...)` 沒有 `_WIN32` 守衛。
- Linux `auto_headers.sh` 產生 `SRS_RTC`／`SRS_SRT`，原始碼卻用 `SRS_AUTO_*_USE`；Windows CMake 兩套都定義，所以只有 Linux 失敗。
- st-srs 的三個 Windows 缺陷（SP 寬度、閒置堆疊、訊號 pipe 空轉）沒有任何自動測試。

### 修改
- Docker 基底改 Ubuntu 24.04，安裝系統 OpenSSL 3、libsrtp2、SRT 1.5、FFmpeg 6.1、Opus，configure 加 `--use-sys-srtp=on --use-sys-srt=on --use-sys-ffmpeg=on`；不再編譯 vendored 舊版。
- `srs_fd_reuseaddr/reuseport/keepalive` 依平台選 `SOCKET` 或 `int`。
- `srs_core.hpp` 加 `SRS_RTC`→`SRS_AUTO_RTC_USE`、`SRS_SRT`→`SRS_AUTO_SRT_USE` 別名；`srs_app_rtmp_conn.cpp` 的 bridge 改用 `SRS_AUTO_RTC_USE && SRS_FFMPEG_FIT` 守衛。
- 新增 `dev/tests/st_event_loop_test.cpp`：8 個協程 × 100 次 yield，以及 32 條 loopback TCP 的 accept／read／write；CMake 目標 `st-event-loop-test`，`ctest -R st-event-loop-test` 通過。
- Nightly 增加 `ctest` 步驟，smoke 增加 RTMP handshake（1537 bytes → 16 bytes）。
- `dev/docker` 的 dev.conf 加 `srt_server`／`rtc_server`，compose 映射 UDP 11980／18000。

### 驗證
- Docker：HTTP 200、RTMP handshake 16 bytes；日誌顯示 `srt listen at udp://0.0.0.0:10080`、`WebRTC listen at udp://0.0.0.0:8000`、`RTC: connection manager run`。
- Windows：`ctest` 1/1 passed（0.18s）；本機以替代埠驗證 HTTP 200、RTMP 16 bytes。
- 待辦：Windows native 的 RTC／SRT 仍需 MSVC 可用的 libsrtp／libsrt。

## 2026.11.14 04:10 新增 repo 內的 dev 測試環境（Windows srs-dev.exe ＋ Docker Linux）

**類型**: 新增 · **檔案**: `dev/srs-dev.ps1`, `dev/windows/dev.conf`, `dev/docker/`, `dev/README.md`, `.dockerignore`, `.gitignore`

### 問題 - 開發測試沒有固定入口，產物與設定散落在 AppData／暫存目錄
每次測試都要手動找 `build/Debug/srs.exe`、複製 DLL、改埠號；Linux 版只能另外找方法跑，與 Windows 版的行為差異難以對照。

### 根因 - 專案沒有把「開發測試」視為一級工作流
建置產物、測試設定與執行時狀態（pid／log）沒有集中位置，也沒有區分 dev 與正式 artifact。

### 修改 - 以 `dev/` 集中兩條車道
- `dev/srs-dev.ps1`：Windows 建置→複製成 `dev/runtime/windows/srs-dev.exe`（含 DLL、PDB、`objs/`）→啟動／停止／狀態；Docker 則 build／up／down。
- `dev/windows/dev.conf`（11935／11985／18088）與 `dev/docker/dev.conf`（1935／1985／8088）；主機端都映射到 11935／11985／18088，避開本機已占用的 1985。
- `dev/docker/Dockerfile` 從本地 `trunk/` 建置上游 Linux 版，`docker-compose.yml` 負責映像、埠與設定掛載。
- `dev/runtime/` 加入 `.gitignore`；`.dockerignore` 排除產物與文件；changelogkit 的 `exclude` 也排除 `dev/runtime/**`。

### 驗證
- Windows 車道：`-Build -Run` 後 API 200，`-Stop` 可正常結束。
- Docker 車道：建置上游 Linux 版並啟動，API 200（待本機 Docker 實測）。

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
