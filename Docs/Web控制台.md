# SRS Web 控制台

**一句話**：`ui/` 是 SRS 本體的 React 控制台，直接呼叫同源 `/api/v1`；Oryx 的 `SrsConsole.js` 提供資料模型與版面參考，但這裡移除了 Terraform／Redis／forwarding／bearer token 的平台耦合。

## 最短路徑

| 目的 | 指令 |
| ------ | ------ |
| 開發（Vite HMR） | `pwsh dev/serve-ui.ps1` → `http://127.0.0.1:5173/console/` |
| 建置並安裝到 SRS | `pwsh dev/build-ui.ps1` → `http://127.0.0.1:11985/console/` |
| 只建置靜態檔 | `npm --prefix ui run build` |
| 指定 SRS API | `pwsh dev/serve-ui.ps1 -Api http://127.0.0.1:1985` |

## 頁面

| 頁面 | 資料來源 | 說明 |
| ------ | ---------- | ------ |
| Overview | `/api/v1/summaries` | SRS／OS 的 CPU、記憶體、連線、網路速率、程序資訊 |
| Vhosts | `/api/v1/vhosts/` | vhost 狀態、串流／連線數、HLS |
| Streams | `/api/v1/streams/` | 發布狀態、視訊／音訊、流量、踢除 publisher |
| Clients | `/api/v1/clients/` | 連線類型、IP、存活時間、踢除 |
| Configs | `/api/v1/config`、`/vhosts/`、`/features` | 唯讀的 listeners、限制、建置能力與 vhost 摘要；不使用 raw API |
| Push Diag | `/api/v1/streams/`、HTTP-FLV | 選目前串流或手動輸入 URL，分析 FLV 標籤流、位元率、封包、關鍵影格與 GOP |
| Player | `/api/v1/streams/`、`/api/v1/config`、WHEP／HTTP-FLV／HLS | 選目前串流，支援 WebRTC（WHEP）、HTTP-FLV、HLS 播放 |

## 架構

```text
ui/ (React 18 + Vite 6 + Bootstrap 5)
├── src/api.jsx          axios → /api/v1
├── src/pages/*          Overview/Vhosts/Streams/Clients/Configs/PushDiag/Player
├── src/media.js         播放 URL 組裝（WHEP／FLV／HLS）
├── src/usePolling.jsx   3 秒輪詢，頁面隱藏時暫停
├── src/flvAnalyzer.js   FLV header/tag 解析（推流診斷）
└── src/i18n.jsx         zh-TW / en
```

- **同源 API**：建置後的檔案放在 `objs/nginx/html/console/`，SRS `http_api` 會把 `/console` 掛到該目錄；瀏覽器直接打 `/api/v1`，不需要 CORS。
- **開發代理**：Vite `server.proxy['/api']` 轉到 `SRS_API`（預設 `http://127.0.0.1:11985`）。
- **錯誤處理**：`ErrorBoundary` 接住渲染錯誤；輪詢失敗保留上一筆資料，只顯示警告，不清空畫面。
- **播放器**：WHEP 使用原生 `RTCPeerConnection`；HTTP-FLV／HLS 使用 `flv.js`／`hls.js`，以動態 import 載入，不佔初始 bundle。
- **播放 URL**：WHEP 走 API 來源的 `/rtc/v1/whep/`；FLV／HLS 走 `/api/v1/config` 回報的 `http_server` 埠。
- **認證**：目前沿用 SRS API 本身；若前面有反向代理認證，瀏覽器會帶其 session。尚未實作 Basic Auth 登入表單。

## 依賴策略

- 只使用 **Bootstrap CSS** 與原生 HTML／React markup，不引入 `react-bootstrap`、`react-aria` 或整套 icon 套件；目前 `node_modules` 約 3 千個檔案，主要是 Vite／Babel／browserslist 的建置期依賴。
- 新增 `flv.js`／`hls.js` 作為播放器；兩者以動態 import 載入，只有實際選 HTTP-FLV／HLS 時才下載。
- `playwright-core` 只用於 RTC E2E，不進入執行期 bundle。
- `node_modules` 與 `build/` 都不進版控；CI 用 `npm ci` 重建，執行期只輸出 `ui/build/` 的靜態檔。
- 套件清單以 `package.json` 為準；新增套件前先確認是否只是為了幾個元件，能用手寫 markup 就不引入。

## API 認證（可選）

`http_api.auth` 預設關閉。需要保護 API 時：

| 設定 | 行為 |
|------|------|
| `dev/windows/dev.conf`、`dev/docker/dev.conf`、`trunk/conf/srs.conf` | auth 關閉，不需要登入 |
| `trunk/conf/full.conf` 或自行開啟 `auth.enabled on` | 首次啟動產生初始密碼，之後沿用 |
| 顯式設定 username／password 或環境變數 | 不產生初始密碼，以設定值為準 |

```nginx
http_api {
    enabled on;
    listen 1985;
    auth {
        enabled on;
    }
}
```

未設定 `username`／`password` 時，SRS 首次啟動會產生隨機密碼並寫入 `objs/console-auth.txt`；日誌只印檔案路徑，不印密碼。可用 `SRS_HTTP_API_AUTH_USERNAME`／`SRS_HTTP_API_AUTH_PASSWORD` 覆寫，或使用 `dev/dev-auth.ps1` 取得穩定的開發密碼（存在 `dev/runtime/dev-password.txt`，不進版控）。

> 只有「auth 啟用且沒有顯式帳密」才會初始化。**不是每次啟動都產生新密碼**：同一個 `work_dir` 會沿用檔案；刪除檔案、清空容器 `objs/` 或換 `work_dir` 才會重新產生。容器部署建議用環境變數或持久化 `objs/`，避免重啟後密碼與 token 失效。

登入流程：

1. `POST /api/v1/console/login` 以帳密換取 7 天有效的 Bearer token。
2. 控制台只把 token 存在 `localStorage`，不保存密碼。
3. token 剩餘時間低於 1 天時，控制台呼叫 `/api/v1/console/refresh` 滑動續期；重新整理頁面仍會沿用 token。
4. SRS 重啟後 token 仍有效，除非 `http_api.auth` 密碼被變更。

> Basic Auth 仍保留給 API 工具；瀏覽器原生 Basic 對話框會被 `X-SRS-Console` 抑制，控制台使用自己的登入表單。

## 與 Oryx 的差異

| Oryx | 本專案 |
| ------ | -------- |
| `/terraform/v1` 平台 API、bearer token | 直接 `/api/v1` |
| Forwarding、Redis、AI、平台設定頁 | 只保留 SRS 核心頁面 |
| 多 scenario（source／live／record／transcode…） | MVP 先做監控與踢除；scenario 後續再加 |
| 播放器依賴平台 `/players` | 尚未整合播放器 |

## 建置與 CI

- 本機：`dev/build-ui.ps1` 建置並複製到 `build/Debug`（`-Target full` 可選 `build-full`）。
- RTC 媒體 E2E：`pwsh dev/rtc-e2e.ps1 -SrsExe ... -ConfFile dev/windows/dev.conf -WorkDir ...`；用 `ui/e2e/rtc-whip.html` 發佈合成 H.264 canvas 串流，再從 console WHEP 播放並檢查 `videoWidth`。
- 認證 E2E：`pwsh dev/auth-e2e.ps1 -SrsExe ... -WorkDir ...`；用 `dev/dev-auth.ps1` 的開發密碼驗證登入、token 保存、續期與登出。
- CI：Nightly 在 `ui/` 執行 `npm ci`＋`npm run build`，複製到 `build/Debug/objs/nginx/html/console`，並在 smoke 檢查 `/console/` 回 200；artifact 也會帶上 console。Windows Full 另外跑 RTC 媒體 E2E。
- Pages：與控制台無關；API 參考仍由 `Doxyfile` 產生。

## 已知限制

- `/api/v1/raw` 預設關閉；控制台改用唯讀 `/api/v1/config`，不需要開啟可寫入的 raw API。
- 推流診斷需要瀏覽器能讀取 HTTP-FLV；若 HTTP server 在另一個來源，請開啟 `http_server.crossdomain`（或用同源反向代理）。
- RTC 播放目前以 WHEP 為主；SRT 播放器尚未接入。
- RTC 媒體 E2E 使用 Chrome／Edge 的 canvas 合成串流，驗證的是 WHIP 發佈＋WHEP 播放路徑，不是實體攝影機相容性。
- 認證為可選功能；預設關閉。啟用但未設定帳密時，初始密碼在 `objs/console-auth.txt`（Windows 檔案 ACL 由部署環境決定）。
- 目前 token 存在 `localStorage`，可被同源 XSS 讀取。正式對外部署前應升級為 **HttpOnly cookie session**：`HttpOnly`＋`SameSite=Strict/Lax`＋（HTTPS 時）`Secure`，並為會改變狀態的 API 加 CSRF 防護與伺服器端撤銷。
- `HttpOnly` 本身不要求 HTTPS；但 `Secure` 旗標要求 HTTPS，且 token／密碼在 HTTP 上仍會明文傳輸，所以非本機環境應一律使用 HTTPS。
- 尚未提供多使用者、角色或撤銷單一 session 的功能；token 以設定密碼簽章，換密碼即全部失效。
