# SRS-Win 變更歷史

最新在最上。格式：`## YYYY.MM.DD HH:MM 一句話標題`，內文分「問題／根因／修改」。

## 2026.11.17 00:30 Doxygen 內建介面固定英文，避免半套中文翻譯

**類型**: 文件／工具鏈 · **檔案**: `Doxyfile`, `Docs/API導讀.md`

### 問題

`Doxyfile` 原本設定 `OUTPUT_LANGUAGE = Chinese`。Doxygen 1.16 的 Chinese
語言包未更新，產生文件時會出現 language warning，且部分內建標籤會中英
混雜。

### 根因

`OUTPUT_LANGUAGE` 只控制 Doxygen 內建 UI 字串，不會翻譯原始碼註解或
`Docs/API導讀.md`；Chinese catalog 落後目前 Doxygen 版本。這與文件索引中
「Doxygen API 頁面維持英文」的既有策略也不一致。

### 修改

- `Doxyfile` 改為 `OUTPUT_LANGUAGE = English`，並在設定旁註明原因。
- `Docs/API導讀.md` 新增「Doxygen 介面語言」：說明只影響內建標籤，不影響
  API 內容；等 Chinese 語言包追上再評估切回。

### 驗證

- 以本機 Doxygen 重新產生 `_site/api/index.html`，無 language warning。
- 既有原始碼註解的 Doxygen warning 仍在，未在本變更擴大處理。
- API 導讀、原始碼註解與搜尋內容不受影響。

## 2026.11.27 補 wall clock 與 process stub 契約測試

**類型**: 測試 · **檔案**: `dev/tests/win_compat_test.cpp`, `Docs/平台移植契約.md`, `TODO.md`

### 修改

- `win-compat-test` 新增 wall clock 前進檢查。
- 新增 `fork`／`kill` 必須回 `ENOSYS`、`getppid` 回 `0` 的 fail-loud 檢查。
- 修正契約用語：SRS 目前沒有跨平台 monotonic helper，`srs_time_now_realtime()` 是 wall clock，不可假設為 monotonic。

### 驗證

- `dev/check.ps1`：4/4 native tests passed。

## 2026.11.27 更新 workflow 使用的 Node runtime

**類型**: CI · **檔案**: `.github/workflows/test.yml`, `.github/workflows/codeql-analysis.yml`, `Docs/CI策略.md`

### 修改

- `changelogkit workflows --check` 原本回報 21 個 EOL action（`test.yml`、`codeql-analysis.yml`）。
- 更新 `actions/checkout@v7`、`github/codeql-action@v4`、`nick-fields/retry@v4`、`docker/setup-qemu-action@v4`、`docker/setup-buildx-action@v4`。
- 保留已停用 workflow，但同步更新，避免重新啟用時才踩 EOL。

### 驗證

- `changelogkit workflows --check`：35 actions、0 EOL、0 unknown、exit 0。
- YAML 可解析。

## 2026.11.27 Platform capability：PID lock 端到端測試

**類型**: 測試／CI · **檔案**: `dev/test-platform-contract.ps1`, `dev/check.ps1`, `.github/workflows/windows-nightly.yml`, `Docs/測試策略.md`, `Docs/平台移植契約.md`

### 修改

- 新增真實 server 契約測試：第一個 instance 啟動、第二個被 PID lock 拒絕、stale pid 不阻止重啟。
- `dev/check.ps1 -Platform` 可選擇執行；Nightly 加入 `Platform contract (PID lock)`。
- 契約文件與測試策略補上測試對應。

### 驗證

- 本機 `dev/check.ps1 -NoTests -Platform`：PID duplicate rejected、stale pid recovered、PASS。
- Nightly YAML 可解析；待下一次 CI 驗證。

## 2026.11.27 新增平台移植契約

**類型**: 文件／架構 · **檔案**: `Docs/平台移植契約.md`, `Docs/文件索引.md`, `.changelog.toml`, `TODO.md`

### 內容

- 定義 Build／OS／ABI 三層契約與 Verified／Unverified／Not supported 狀態用語。
- 記錄檔案替換、share-delete、poll、PID lock、process、signal、time、errno 的必須語意與測試對應。
- 建立 Windows default／full、Linux x64、macOS、ARM64 的 capability matrix；未驗證不標成支援。
- 列出平台邊界、變更流程與目前缺口（CreateProcess、IOCP、ARM64、FFmpeg stub 等）。

### 驗證

- Markdown lint 0 errors；文件索引與 changelogkit 設計分組已加入。

## 2026.11.27 CMake 升級後的 cache 自我修復

**類型**: 建置／工具鏈 · **檔案**: `dev/check.ps1`, `Docs/建置與執行說明.md`, `Docs/VSCode除錯手冊.md`

### 問題

CMake 從 4.3 更新到 4.4 後，舊的 `CMakeCache.txt` 仍記錄 `CMAKE_ROOT=C:/Program Files/CMake/share/cmake-4.3`，導致 4.4 的 compiler detection 找不到 4.3 modules，出現 `CMAKE_DETERMINE_COMPILER_ID` unknown。

### 修改

- `dev/check.ps1` 在 cache 存在時檢查 `CMAKE_ROOT`；路徑不存在就自動執行 `cmake --fresh`（default／full 各自 preset），再繼續建置；不刪 build 輸出。
- 建置文件與 VSCode 除錯手冊加入 `--fresh` 排錯方式。

### 驗證

- 手動把 cache 的 `CMAKE_ROOT` 改成不存在路徑，`dev/check.ps1` 自動 refresh 並成功建置；cache 恢復為 4.4 路徑。

## 2026.11.27 P0：功能旗標單一來源與 console 能力自檢

**類型**: 建置／API／UI · **檔案**: `trunk/src/core/srs_feature_flags.hpp`, `trunk/src/core/srs_core.hpp`, `trunk/ide/srs_msvc/CMakeLists.txt`, `trunk/auto/auto_headers.sh`, `trunk/src/app/srs_app_http_api.cpp`, `ui/src/pages/Configs.jsx`, `ui/src/i18n.jsx`, `Docs/功能旗標與能力自檢.md`

### 修改

- 新增 `srs_feature_flags.hpp`，把 Linux 的 `SRS_*` 與 Windows 的 `SRS_AUTO_*` 正規化為 `SRS_AUTO_*`、`SRS_AUTO_*_USE`、`SRS_*_BOOL`；缺失 alias 時以 `#error` 阻擋。
- Windows CMake 不再重複產生 `SRS_AUTO_*_USE`／`_BOOL`；Linux `auto_headers.sh` 補上 canonical `SRS_SSL`／`SRS_HLS` 輸入。
- `/api/v1/config` 新增 `capabilities`（編譯能力），並補上 HLS／DASH／DVR 的 runtime `enabled`。
- Console 設定頁改為「能力／建置／設定／狀態」表格，區分 compiled 與 configured。
- 新增 `Docs/功能旗標與能力自檢.md`。

### 驗證

- Windows default／full 建置成功（含 full RTC／SRT）。
- Linux Docker 建置與 smoke 成功；capabilities 回報 `ssl:true`，GB28181／DASH 依 lane 關閉。
- `npm --prefix ui test`：10/10 passed；UI build 成功。
- `/api/v1/config` 實測：default `rtc:false`、full `rtc:true`，Linux full `rtc:true`。

## 2026.11.27 Worktree 文件補 repair 與官方文件來源

**類型**: 文件 · **檔案**: `Docs/development/git-worktree.md`, `Docs/development/tutorial/README.md`

### 修改

- 補上 worktree 目錄被移動時用 `git worktree repair`，而不是 `prune`；記錄實測的 `gitdir incorrect` 訊息仍可成功。
- 補上 `git worktree -h`、`git help worktree`、官方文件與 Git for Windows 本機 HTML 的查詢方式。
- 明確說明 prune 後從回收站還原只保留檔案，Git worktree 連結不會自動恢復。

### 驗證

- 實際移動 worktree 後執行 `git worktree repair`，`git worktree list` 與 `status` 均正常。
- Markdown lint 0 errors。

## 2026.11.27 Worktree 安全清理：回收站優先與 helper

**類型**: 文件／工具鏈 · **檔案**: `Docs/development/git-worktree.md`, `Docs/development/tutorial/*`, `dev/remove-worktree.ps1`

### 修改

- 新增 worktree 生命週期指南：建立、檢查、回收站優先、可直接清理的產物、`git worktree remove` 行為、分支與 reflog。
- 新增 `dev/remove-worktree.ps1`：驗證註冊 worktree、拒絕主 worktree、列出未提交變更、拒絕執行中程序、送 Windows 回收站後 `git worktree prune`；不自動降級永久刪除。
- Learn 1 與教學入口改掉直接 `git worktree remove` 的收尾，連結到安全清理流程。

### 驗證

- 乾淨 worktree：`-DryRun` 通過；實際回收後目錄消失、metadata prune、分支保留。
- dirty worktree：未加 `-DiscardChanges` 會拒絕並保留；加上後送回收站。
- 主 worktree 與不存在路徑：正確拒絕。
- 函式改用 approved verb `ConvertTo-NormalizedPath`，避免 PowerShell 警告。

## 2026.11.27 Learn 1 在乾淨 worktree 實跑：補 console bundle 與 UI 建置步驟

**類型**: 文件／工具鏈 · **檔案**: `Docs/development/tutorial/01-learn1-build-and-run.md`, `dev/srs-dev.ps1`, `dev/README.md`, `TODO.md`

### 驗證

以 worktree `tutorial/learn1`（baseline `f4ce8d6c8`）從頭執行：

- `dev/check-source-lists.ps1`：OK（56 個 app source）。
- `dev/check.ps1`：4/4 native tests passed（244.2s）。
- Windows dev：API 200；`/console/` 在修正後 200。
- Docker Linux lane：`local-deploy.ps1 -Deploy` 成功；smoke HTTP 200、RTMP handshake 16 bytes。
- Tutorial probe：檢查正確 FAIL，刪除後恢復 OK。

### 修正

- 發現 `srs-dev.ps1 -Build` 只建立空的 `objs`，沒有把已建置的 console 複製到 runtime，導致 API 可用但 `/console/` 回 404。
- `Build-Windows` 現在會把 `build/Debug/objs/nginx/html/console` 複製到 runtime；找不到時明確提示先跑 `dev/build-ui.ps1 -Target default`。
- Learn 1 加入 console 建置步驟、404 與檔案占用排錯。

## 2026.11.16 02:50 Tutorial probe 改由檢查腳本守門，不加入 .gitignore

**類型**: 工具鏈／文件 · **檔案**: `dev/check-source-lists.ps1`, `Docs/development/tutorial/01-learn1-build-and-run.md`

### 修改

- source list 檢查會明確偵測 `srs_app_tutorial_probe.cpp`，即使它被加進兩份 manifest 也會失敗。
- 教學說明不把 probe 加進 `.gitignore`：ignore 只會讓 `git status` 看不到殘留檔，不能阻止編譯或 `git add -f`。
- 練習預期輸出加入 probe 提示。

### 驗證

- 正常狀態 `source list check: OK`。
- 建立 probe 後 FAIL，刪除後恢復 OK。

## 2026.11.16 02:40 Learn 1 補上 VSCode 工作區入口

**類型**: 文件 · **檔案**: `Docs/development/tutorial/01-learn1-build-and-run.md`, `README.md`

### 修改

- Learn 1 新增「用 VSCode 啟動」：開啟 workspace 根目錄、launch 設定對照、Tasks 清單、建議情境與 Docker 除錯邊界。
- 練習加入用 F5 啟動 SRS 與從 Tasks 跑 `dev: check Windows`。
- 明確區分：Learn 1 只教入口；Learn 3 才是完整 VSCode 除錯工作流。

## 2026.11.16 02:30 新增開發教學：Learn 1 與 Build 配置導讀

**類型**: 文件 · **檔案**: `Docs/development/tutorial/*`, `Docs/文件索引.md`, `.changelog.toml`

### 修改

- 新增教學系列入口、Learn 1（Windows default／full、dev 啟動、Docker smoke、source list 練習）與 Build 配置導讀（CMakePresets、CMakeLists、Dockerfile、compose）。
- 文件索引與 changelogkit 的「開發教學」分組加入這系列。
- 練習以 worktree 進行；source list 故意失敗的 probe 檔要求刪除，不進版本控制。

### 驗證

- Markdown lint 0 errors；所有指令與目前路徑／埠號對照。

## 2026.11.16 02:00 P0 開始：來源清單守門與 Linux build CI

**類型**: 建置／CI · **檔案**: `dev/check-source-lists.ps1`, `dev/check.ps1`, `.github/workflows/linux-build.yml`, `TODO.md`

### 修改

- 新增來源清單檢查：比對 `trunk/src/app/*.cpp`、`trunk/configure` 的 `MODULE_FILES` 與 MSVC `CMakeLists.txt`，新增檔案漏列時直接失敗。
- `dev/check.ps1` 建置前先跑這個檢查。
- 新增 Linux Build workflow：`trunk/src/**` 等 Linux 建置輸入變更或手動觸發；不設無條件每週排程。
- TODO 加上 P0 平台契約與跨平台建置項目。

### 驗證

- `dev/check-source-lists.ps1`：56 個 app source 在 configure 與 CMake 一致。
- `dev/check.ps1 -NoTests`：source check＋增量建置通過。
- Linux workflow YAML 可解析；GitHub Actions run `43743234425` 全綠（1m7s）。

## 2026.11.16 01:40 Docker Linux lane：縮小 build context、ccache 與 Linux auth 連結修正

**類型**: 建置／修復 · **檔案**: `.dockerignore`, `dev/docker/Dockerfile`, `trunk/configure`, `dev/README.md`

### 問題

- Docker build context 曾達 327.91MB，主要包含 `trunk/ide` 的 Windows build 產物與舊 `trunk/build`；每次 Windows 建置都可能讓 `COPY trunk` cache 失效。
- Linux `configure` 的 `MODULE_FILES` 沒有加入 `srs_app_console_auth`，Docker 建置在連結階段出現 `undefined reference to srs_console_auth_*`。

### 修改

- `.dockerignore` 排除整個 `trunk/ide`、`trunk/build`、`trunk/objs` 與其他非 Linux 建置輸入。
- Dockerfile 使用 BuildKit apt cache mount 與 `/root/.ccache` ccache cache mount；apt layer 不再因原始碼變更重複下載。
- `trunk/configure` 的 APP module 清單加入 `srs_app_console_auth`，修正 Linux 連結。

### 驗證

- `dev/local-deploy.ps1 -Deploy` 成功建置並啟動 Linux 容器。
- 原始碼未變更再次 `-Deploy`：exporting layers 0.1s，全部 layer cache hit。
- context 由 327.91MB 降至後續 incremental 542.87kB（BuildKit 重用既有 blob）；Dockerfile 首次建置仍會下載一次 apt 套件。

## 2026.11.16 01:20 VSCode 測試通道：UI npm、Windows 與 Docker Linux

**類型**: 工具鏈 · **檔案**: `.vscode/launch.json`, `.vscode/tasks.json`, `.vscode/README.md`, `Docs/VSCode除錯手冊.md`

### 修改

- 「執行與偵錯」新增 `UI: Vitest`、`UI: Vite dev server`、`UI: RTC E2E`、`UI: Auth E2E`（node-terminal）。
- 工作清單新增 Windows check、UI test、Markdown lint、Docker deploy／smoke／shell、RTC／auth E2E，並把測試類工作放進 test group。
- 文件說明 launch 下拉選單只放除錯器，測試與 Docker 指令走 `Tasks: Run Task`；Docker 目前是執行／smoke，不是 gdb 原始碼除錯。

### 驗證

- `launch.json`／`tasks.json` 以 `ConvertFrom-Json` 驗證可解析。
- `dev/lint-docs.ps1`：0 errors。

## 2026.11.16 01:10 Markdown lint：MD024 改 siblings_only、加入 autofix 腳本

**類型**: 工具鏈／文件 · **檔案**: `.markdownlint.json`, `.markdownlint-cli2.jsonc`, `.markdownlintignore`, `dev/lint-docs.ps1`

### 修改

- `MD024` 設為 `siblings_only: true`：`ChangeHistory` 在不同日期小節下重複「問題／修改／驗證」是刻意的，不應要求唯一標題。
- 忽略 vendored／upstream 文件（`trunk/**/*.md`、`.vscode/vcpkg`、`node_modules`）與建置產物，避免把上游 214 個問題算進本專案。
- 新增 `dev/lint-docs.ps1` 與 `-Fix`，使用 markdownlint-cli2 自動修正可修的格式規則。
- MD024／MD040／MD045 等不是安全 autofix 的規則仍需人工處理；本次已手動修正剩餘專案文件。

### 驗證

- 專案文件：`markdownlint: OK`（0 errors）。
- 全樹（套用 ignore）：0 errors。

## 2026.11.16 00:50 可選 HTTP API 認證：初次隨機密碼、7 天 Bearer token 與自動續期

**類型**: 功能／安全 · **檔案**: `srs_app_console_auth.*`, `srs_app_http_api.*`, `srs_protocol_http_stack.*`, `ui/src/api.jsx`, `ui/src/components/Login.jsx`

### 伺服器

- `http_api.auth` 維持可選，預設關閉；啟用但未設定帳密時，首次啟動產生隨機密碼並寫入 `objs/console-auth.txt`，日誌只印路徑。
- `SRS_HTTP_API_AUTH_USERNAME`／`SRS_HTTP_API_AUTH_PASSWORD` 可覆寫；變更密碼會使既有 token 失效。
- 新增 `POST /api/v1/console/login`（帳密換 token）與 `POST /api/v1/console/refresh`（滑動續期）。
- token 為 HMAC-SHA256 無狀態 token，有效期 7 天；SRS 重啟後仍有效（密碼不變時）。
- `SrsHttpAuthMux` 同時接受 Basic 與 Bearer；`X-SRS-Console` 請求不送出 `WWW-Authenticate`，避免瀏覽器原生 Basic 對話框。

### 控制台

- 登入表單與 `localStorage` token；不保存密碼。
- token 剩餘低於 1 天時自動呼叫 refresh，不需重新輸入密碼。
- 登出只清除本機 token；無狀態 token 本身仍有效直到過期或密碼變更。
- `dev/dev-auth.ps1` 提供穩定開發密碼，存在 gitignored `dev/runtime/dev-password.txt`。

### 驗證

- `dev/auth-e2e.ps1` PASS：登入、reload 沿用 token、refresh、登出。
- Windows Full CI `43737927172` 的 Console auth E2E PASS。
- `dev/windows/auth.conf` 未設定帳密時，實際產生 `objs/console-auth.txt`。

## 2026.11.15 23:20 RTC 媒體 E2E（WHIP／WHEP）與 console 播放器

**類型**: 功能／測試／修復 · **檔案**: `ui/src/pages/Player.jsx`, `ui/src/media.js`, `ui/e2e/*`, `dev/rtc-e2e.ps1`, `srs_app_rtc_source.cpp`

### 功能

- Web 控制台新增「播放器」頁：WebRTC（WHEP）、HTTP-FLV、HLS；可從 `/api/v1/streams/` 選目前串流。
- HTTP-FLV／HLS 以動態 import 載入 `flv.js`／`hls.js`，不進初始 bundle；主 bundle 約 247KB。
- WHEP 使用原生 `RTCPeerConnection`；video-only 串流不送 audio m-line，m-line 不匹配時自動退回 video-only。

### 修復

- `SrsRtcConsumer::wait(0)` 原本 `queue_.size() >= 0` 恒真，realtime 模式下沒有封包時立即返回，`SrsRtcPlayStream::cycle` 空轉餓死 ST 排程器，導致 HTTP API 停止回應。改為 `nb_msgs == 0` 時等待佇列非空。

### E2E

- `ui/e2e/rtc-whip.html`：canvas H.264 WHIP 發佈器，不需攝影機權限。
- `ui/e2e/rtc-roundtrip.mjs`：Playwright 發佈 WHIP → console WHEP 播放 → 驗證 `readyState`、`videoWidth`／`videoHeight`，並確認 API 仍可回應。
- `dev/rtc-e2e.ps1` 啟動 full SRS 並執行上述測試；Windows Full CI 加入此步驟。

### 驗證

- 本機：`rtc-e2e: PASS`，video 320x180、readyState 4、paused false。
- Windows Full CI `43729141759`：RTC media E2E step PASS，video 320x180；ctest 與 full smoke 也通過。

## 2026.11.15 13:50 Phase 1 結案與功能開發門檻

**類型**: 文件／規劃 · **檔案**: `Docs/Phase1結案.md`, `Docs/文件索引.md`, `.changelog.toml`, `TODO.md`

### 內容

- 定義 Phase 1 最小出口：建置／ctest／ABI probe／檔案語意／PID／stub／smoke／Full CI／文件。
- 明確延後選擇性 utest、RTC 媒體 E2E、CreateProcess、IOCP、ARM64、FFmpeg 等，不作為功能前置條件。
- 定義功能開發護欄與 Phase 2 建議順序。
- 目前不建立 tag；Release workflow 只在明確版本時觸發。

## 2026.11.15 13:35 1E 第二批：Windows Full（RTC／SRT）CI job

**類型**: 測試網／CI · **檔案**: `.github/workflows/windows-full.yml`, `Docs/CI策略.md`, `Docs/測試策略.md`

### 修改

- 新增每週日＋手動的 `Windows Full (RTC/SRT)` workflow。
- 使用 runner 的 `C:\vcpkg` 安裝 `libsrtp`／`libsrt`／`opus`，快取 `C:\vcpkg\installed`。
- 建立 `.vscode/vcpkg` junction、configure `windows-full`、建置 srs 與所有 native tests。
- 跑 `ctest --preset windows-full-test`，再用 `dev/smoke-test.ps1` 驗證 full 版本啟動。

### 已知限制

- full smoke 只驗證 API／RTMP／config API 與監聽啟動；WHIP／WHEP 媒體 E2E 尚未納入。

### 驗證

- run `43724683576` 全綠（12m10s）：vcpkg install/cache、full build、4 個 native tests、full smoke 都通過。

## 2026.11.15 13:20 1E 第一批：Nightly／Release 共用 smoke test

**類型**: 測試網 · **檔案**: `dev/smoke-test.ps1`, `.github/workflows/windows-nightly.yml`, `.github/workflows/release.yml`

### 問題

- Nightly 與 Release 各自維護一份 PowerShell smoke；兩邊檢查項目與等待邏輯容易漂移。
- Release 只檢查 HTTP API，沒有 RTMP handshake 或 config API。

### 修改

- 新增 `dev/smoke-test.ps1`：啟動 SRS、等待 API（不用固定 sleep）、RTMP handshake、可選 `/console/`、`/api/v1/config`，最後停止 process。
- Nightly 與 Release 都改用這個腳本；Release 多了 RTMP 與 config 檢查。
- 支援 `-ApiPort`／`-RtmpPort`，本機埠被占用時可覆寫。

### 驗證

- 本機以 dev 埠執行：`API OK / RTMP handshake OK / Console OK / Config API OK / smoke: PASS`。
- 兩個 workflow YAML 可解析，PowerShell parser 無錯。

## 2026.11.15 13:05 1D：建置資訊與 README 功能宣稱對齊

**類型**: 文件／建置 · **檔案**: `CMakeLists.txt`, `README.md`, `Docs/項目概覽說明.md`, `Docs/組件盤點與技術債.md`

### 問題

- `srs_auto_headers.hpp` 的 `SRS_BUILD_DATE` 固定為 `2026-06-05`、`SRS_BUILD_TS` 固定 `0`，所有 build 在 API 看起來同一天。
- README 把 WebRTC／SRT／GB28181 都標成 ✅，但預設 `windows-default` 關閉 RTC／SRT。

### 修改

- CMake configure 時用 UTC 時間產生 `SRS_BUILD_DATE`、`SRS_BUILD_TS`、`SRS_AUTO_BUILD_TIME`；`SRS_USER_CONFIGURE`／`SRS_CONFIGURE` 明確標為 `cmake`。
- README 支援矩陣拆成「預設建置／全功能建置」，WebRTC／SRT 僅在 `windows-full` 為 ✅，GB28181 標為未完成端到端驗證。
- 項目概覽同步說明預設與 full 的差異。

### 驗證

- `/api/v1/config` 回報 `build=2026-11-14`、`build_ts=1794700572`（實際 configure 時間）。
- 本機 `srs` Debug 建置成功。

## 2026.11.15 12:45 更正官方 Windows 支援脈絡

**類型**: 文件 · **檔案**: `README.md`, `Docs/項目概覽說明.md`, `Docs/Windows移植技術文件.md`, `trunk/3rdparty/srs-docs/doc/windows.md`

### 修正

- 原本寫「官方 SRS 已在 v7.0.60+ 終止 Windows 支援」，容易誤解為 v7.0.60 之後才移除。
- 依 git 證據：官方 Windows 支援是 **Cygwin64**（自 v5.0.89），在 **v7.0.60（2025-08-21，#4447）** 移除；v7.0.60 本身就是移除點。
- 官方 SRS 7 沒有原生 MSVC Windows 支援；本 fork 以 **v7.0.89** 為基礎，重新建立原生 MSVC/CMake port。
- 用語更正：官方是**正式停止支援（drop support）並移除實作**，不只是「不再維護」；「不再維護」是結果，但不足以描述程式碼已被刪除。
- README、項目概覽與 Windows 技術文件統一這個說法。

## 2026.11.15 12:30 1C 第二輪：wait／timer／resource stub 與集中 errno 映射

**類型**: 根基／修復 · **檔案**: `compat/sys/wait.h`, `compat/sys/time.h`, `compat/sys/resource.h`, `compat/ifaddrs.h`, `compat/srs_win32_errno.h`

### 問題

- `wait()` 回 -1 沒設 errno；`waitpid()` 沒有填 `status`，呼叫端讀到未初始化值。
- `getitimer`／`setitimer`、`getrlimit`／`setrlimit` 回 0，讓呼叫端誤以為成功。
- Win32 錯誤碼映射散落在檔案 rename／open，其他 compat 函式沒有共用。

### 修改

- 新增 `compat/srs_win32_errno.h`，集中 Win32 error → errno；file open／rename、wait、ifaddrs 共用。
- `wait()` 回 `-1` + `ENOSYS`；`waitpid()` 只接受特定 pid，正確處理 `WNOHANG`、設定 POSIX 形式 exit status，並在找不到 pid 時回 `ESRCH`。
- `getitimer`／`setitimer` 與 `getrlimit`／`setrlimit` 改回 `-1` + `ENOSYS`。
- `win_compat_test` 擴充上述契約；文件更新 Windows 不支援清單。

### 驗證

- `win compat stubs: PASS`。
- `check.ps1` 4/4 ctest 通過；`srs` Debug／Release 可建置。

## 2026.11.15 12:05 1C 第一輪：POSIX stub fail-loud 與設定檢查

**類型**: 根基／修復 · **檔案**: `force_include.h`, `compat/sys/epoll.h`, `srs_app_config.cpp`, `dev/tests/win_compat_test.cpp`

### 問題

- `fcntl(F_SETLK)` 在 Windows 回 0，讓呼叫端以為取得了檔案鎖。
- `epoll_*` 回 -1 但沒有設定 `errno`，除錯時看不到原因。
- `asprocess`／`inotify_auto_reload` 需要 POSIX 的 parent-death／inotify 語意，Windows 版本卻可能啟動成功後才失效。

### 修改

- `srs_fcntl` 對 `F_SETLK`／`F_SETLKW` 明確回 `-1` + `ENOSYS`；`F_GETFD`／`F_SETFD` 等維持安全的 no-op 並加註。
- `epoll_create`／`epoll_ctl`／`epoll_wait` 設定 `errno = ENOSYS`。
- `SrsConfig::check_normal_config()` 在 Windows 拒絕 `asprocess` 與 `inotify_auto_reload`，不再等到啟動後才失敗。
- 新增 `dev/tests/win_compat_test.cpp`：驗證 fcntl／epoll／inotify 的失敗契約；接入 ctest、`dev/check.ps1`、Nightly。
- `Docs/Windows移植技術文件.md` 增加 Windows 不支援／降級清單。

### 驗證

- `win-compat-test`：`win compat stubs: PASS`。
- 帶 `asprocess on;` 的設定啟動時以 code 1023 拒絕，訊息為 `asprocess is not supported on Windows`。

## 2026.11.15 11:35 修正 Release 協程 context switch 的 ABI 暫存器缺漏

**類型**: 根基／修復 · **檔案**: `md_win64.asm`, `md.h`, `CMakeLists.txt`, `Docs/Windows移植技術文件.md`

### 問題

- `st-event-loop-test` 在 Release 會 segfault，Debug 正常；cdb 顯示 `st_cond_timedwait` 讀到損壞指標。
- `_st_md_cxt_save`／`restore` 只保存 RBX、RBP、R12-R15，漏了 Windows x64 ABI 的 RDI／RSI（也漏 XMM6–XMM15、MXCSR／x87 control）。Debug 沒觸發，Release 最佳化後才踩到。

### 修改

- 補齊 RDI／RSI、XMM6–XMM15、MXCSR、x87 control word 的保存與還原。
- Windows 的 `__jmpbuf` 改成 8-byte slots（`long long[32]`）；XMM 每個需要 2 個 slot，若壓成 1 個會讓 XMM15 與 MXCSR 重疊。
- 新增 `dev/tests/st_abi_probe.asm`：在 context switch 前把完整 non-volatile 集合載入 sentinel，yield 後逐項比對並回傳失敗 bitmask；`st-event-loop-test` 在 Debug／Release 都執行。
- Release 的 `st-event-loop-test` 加上 `/Zi`／`/DEBUG`，CI 失敗時可直接 symbolize。
- `Docs/Windows移植技術文件.md` 補上「為什麼需要組合語言」、ABI 暫存器表與 jmp_buf 佈局。

### 驗證

- Release／Debug `st-event-loop-test` 都顯示 `context switch ABI: mask=0x00000000`。
- ABI probe 在未修正 XMM layout 時抓到 `mask=0x00020000`（XMM15 與 MXCSR 重疊），證明檢查不是只看函式有沒有回傳。
- Nightly 改回 Release 組態；artifact 名稱 `srs-win-nightly-x64`。

## 2026.11.15 11:10 本地優先的 CI 分層與 API 欄位註解

**類型**: 文件／CI · **檔案**: `.github/workflows/windows-nightly.yml`, `.github/workflows/pages.yml`, `Docs/CI策略.md`, `Docs/API註解指南.md`, `srs_protocol_rtmp_stack.hpp`

### 問題

- Nightly 與 Pages 每次 push 都跑，沒有改動也會消耗 CI；Pages 的每日排程也難以理解為什麼要固定產出。
- Doxygen 頁面上的公開欄位只有型別／名稱，因為原始碼使用一般 `//` 註解，Doxygen 不會採用；函式也缺 `@param`。

### 修改

- Nightly 移除 push 觸發，改為每日 03:00（台北）＋手動；schedule 會先比對上次成功 nightly 的 commit，沒有新 commit 就跳過 Windows runner。
- Pages 移除無條件每日排程，改為文件／API 標頭變更的 path filter＋手動。
- 新增 `Docs/CI策略.md`，明確區分本機、Nightly、Release、Pages。
- `SrsChunkStream` 的公開欄位與建構子改成 Doxygen 註解；新增 `Docs/API註解指南.md` 說明 `/** */`、`///`、`@param` 的用法。
- Nightly 暫時維持 Debug：`st-event-loop-test` 的 Release 組態目前會 segfault，修好前不把 Release 產物稱為穩定 nightly。

### 驗證

- `doxygen` 重新產生後，`classSrsChunkStream.html` 顯示 `cid_`、`header_`、`fmt_` 的說明與建構子 `_cid` 參數。
- 兩個 workflow YAML 可解析；本地 Release native test 的 segfault 已記錄為待辦。

## 2026.11.15 10:50 用 LockFileEx 實作 Windows pid 檔互斥

**類型**: 修復／根基 · **檔案**: `srs_app_server.cpp`, `srs_app_server.hpp`

### 問題

- Windows 相容層的 `fcntl(F_SETLK)` 是 no-op，pid 檔其實沒有鎖；第二個 SRS 實例可以同時啟動，造成埠號衝突或兩個 process 同時寫入。

### 修改

- `SrsPidFileLocker` 在 Windows 改用 `CreateFileA` + `LockFileEx(LOCKFILE_EXCLUSIVE_LOCK | LOCKFILE_FAIL_IMMEDIATELY)`；鎖持有到 process 結束。
- 鎖檔存在但 process 已結束時，下一次啟動可直接取得鎖並覆寫 pid；不需要手動刪 pid。
- 保留 POSIX 的 `fcntl` 路徑不變。

### 驗證

- 同一 pid 檔啟動兩個實例：第一個存活，第二個以 code 1034 拒絕啟動。
- 強制結束第一個 process 後直接啟動第二個：成功啟動並 `GET /api/v1/versions` 回 200。

## 2026.11.15 10:35 Windows HLS 檔案讀取分享與 rename 競態

**類型**: 修復 · **檔案**: `srs_kernel_file_open.hpp`, `srs_kernel_file_rename.hpp`, `srs_kernel_file.cpp`, `dev/tests/file_share_test.cpp`

### 問題

- HLS 在 HTTP 讀取同一個 m3u8 時 refresh，可能撞到 Windows 的 `ERROR_SHARING_VIOLATION`。
- CRT `_open` 的分享模式不含 `FILE_SHARE_DELETE`；即使加上，Windows 仍可能拒絕直接 `MoveFileEx(..., MOVEFILE_REPLACE_EXISTING)` 覆寫被開啟的目標。

### 修改

- `SrsFileReader` 在 Windows 改用 `CreateFileA` 開啟唯讀檔，明確加上 `FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE`。
- `srs_path_rename` 在 Windows 遇到 `ACCESS_DENIED`／`SHARING_VIOLATION` 時走兩段式替換：先把開啟中的舊目標移到唯一的 `.old.N`，再放入新檔，最後盡力刪除舊檔；失敗時嘗試把舊檔放回。
- Windows 錯誤映射集中到 `srs_win32_error_to_errno`，供開檔與 rename 共用。
- 新增 `dev/tests/file_share_test.cpp`：在目標仍被開啟時執行 rename，確認新內容成功寫入。

### 驗證

- `ctest`：`st-event-loop-test`、`path-rename-test`、`file-share-test` 3/3 通過。
- 實際推流時用另一個 handle 持續開啟 m3u8，HLS 下一次 refresh 無 `rename`／`HlsWriteFile`／`sharing` 錯誤。

## 2026.11.15 10:10 用 workspace junction 取代固定的 vcpkg 路徑

**類型**: 工具鏈／文件 · **檔案**: `dev/setup-vscode.ps1`, `CMakePresets.json`, `.vscode/c_cpp_properties.json`, `dev/check.ps1`, `rebuild.ps1`, `.markdownlint.json`

### 問題

- IntelliSense 與 CMakePresets 直接寫死 `C:\vcpkg`／`H:\vcpkg` 或 `${env:VCPKG_ROOT}`；前者不能換位置，後者在變數未定義時展開成 `/installed/x64-windows/include`。
- 文件表格的 delimiter 列使用 `|------|`，不符合 `MD060` 的 compact 樣式。

### 修改

- 新增 `dev/setup-vscode.ps1`：依序嘗試 `-VcpkgRoot`、`VCPKG_ROOT`、PATH 上的 `vcpkg.exe`、各磁碟根目錄／家目錄下的 `vcpkg`，建立 `.vscode/vcpkg` junction。
- `.vscode/c_cpp_properties.json` 與 `CMakePresets.json` 改讀 `${workspaceFolder}/.vscode/vcpkg`；`dev/check.ps1 -Full` 會自動建立 junction。
- `rebuild.ps1` 改用腳本所在目錄與 junction，不再寫死 `F:\srs`／`C:\vcpkg`。
- 文件表格 delimiter 統一為 `| --- |`；新增 `.markdownlint.json`（`MD060: compact`）與 `.markdownlintignore`（排除 vendored／build／runtime）。
- **MD013（固定行長）刻意關閉**：本專案文件以中文為主，表格、指令、URL 與長句較多；硬折行會讓 diff 與複製貼上變差。行長改由編輯器軟換行處理，不作為 CI／編輯器錯誤。

### 驗證

- `dev/setup-vscode.ps1` 建立 `.vscode/vcpkg -> H:\vcpkg`。
- `cmake --preset windows-full --fresh` 的 `CMAKE_TOOLCHAIN_FILE` 指向 `.vscode/vcpkg`；`dev/check.ps1 -Full -NoTests` 通過（96.6s）。
- `markdownlint-cli2` 不再回報 `MD060` 或 `MD013`（其他既有規則仍有錯誤，未在這批處理）。

## 2026.11.15 09:45 HLS 輸出路徑移到可預期的 runtime 目錄

**類型**: 設定／可用性 · **檔案**: `dev/windows/dev.conf`, `dev/docker/dev.conf`, `dev/docker/docker-compose.yml`, `dev/local-deploy.ps1`, `srs_app_http_api.cpp`, `ui/src/pages/Configs.jsx`

### 問題

- HLS 預設 `hls_path=./objs/nginx/html` 是相對於**工作目錄**；若從 `%TEMP%` 或其他目錄啟動，輸出就會跑到那裡，不利排查。Docker 車道的 HLS 也只存在容器內，主機看不到。

### 修改

- Windows dev lane：`dev/windows/dev.conf` 明確設定 `hls_path ./objs/nginx/html/hls`；`srs-dev.ps1` 的 cwd 固定在 `dev/runtime/windows`，輸出為 `dev/runtime/windows/objs/nginx/html/hls/<app>/`。
- Docker lane：`dev/docker/dev.conf` 使用 `/srs/trunk/objs/nginx/html/hls`，compose bind mount 到 `dev/runtime/docker/hls`；`local-deploy.ps1 -Deploy` 會先建立該目錄。
- `GET /api/v1/config` 增加 `hls_path`；設定頁顯示 HLS 輸出位置。
- 文件標註「相對於工作目錄」，不是 Windows `%TEMP%`；要改位置用 `work_dir`／`hls_path` 或絕對路徑。

### 驗證

- `docker compose config` 顯示 bind source `F:\srs\dev\runtime\docker\hls`。
- `/api/v1/config` 回應 `hls_path: ./objs/nginx/html/hls`；UI 測試 6/6、建置成功。
- `srs-dev.ps1 -Build -Run` 可啟動並回應 API 200。

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
