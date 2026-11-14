# SRS Web 控制台

**一句話**：`ui/` 是 SRS 本體的 React 控制台，直接呼叫同源 `/api/v1`；Oryx 的 `SrsConsole.js` 提供資料模型與版面參考，但這裡移除了 Terraform／Redis／forwarding／bearer token 的平台耦合。

## 最短路徑

| 目的 | 指令 |
|------|------|
| 開發（Vite HMR） | `pwsh dev/serve-ui.ps1` → `http://127.0.0.1:5173/console/` |
| 建置並安裝到 SRS | `pwsh dev/build-ui.ps1` → `http://127.0.0.1:11985/console/` |
| 只建置靜態檔 | `npm --prefix ui run build` |
| 指定 SRS API | `pwsh dev/serve-ui.ps1 -Api http://127.0.0.1:1985` |

## 頁面

| 頁面 | 資料來源 | 說明 |
|------|----------|------|
| Overview | `/api/v1/summaries` | SRS／OS 的 CPU、記憶體、連線、網路速率、程序資訊 |
| Vhosts | `/api/v1/vhosts/` | vhost 狀態、串流／連線數、HLS |
| Streams | `/api/v1/streams/` | 發布狀態、視訊／音訊、流量、踢除 publisher |
| Clients | `/api/v1/clients/` | 連線類型、IP、存活時間、踢除 |
| Configs | `/api/v1/raw` | 原始設定；未啟用時顯示 code 1061 提示 |

## 架構

```text
ui/ (React 18 + Vite 6 + Bootstrap 5)
├── src/api.jsx          axios → /api/v1
├── src/pages/*          Overview/Vhosts/Streams/Clients/Configs
├── src/usePolling.jsx   3 秒輪詢，頁面隱藏時暫停
└── src/i18n.jsx         zh-TW / en
```

- **同源 API**：建置後的檔案放在 `objs/nginx/html/console/`，SRS `http_api` 會把 `/console` 掛到該目錄；瀏覽器直接打 `/api/v1`，不需要 CORS。
- **開發代理**：Vite `server.proxy['/api']` 轉到 `SRS_API`（預設 `http://127.0.0.1:11985`）。
- **錯誤處理**：`ErrorBoundary` 接住渲染錯誤；輪詢失敗保留上一筆資料，只顯示警告，不清空畫面。
- **認證**：目前沿用 SRS API 本身；若前面有反向代理認證，瀏覽器會帶其 session。尚未實作 Basic Auth 登入表單。

## 與 Oryx 的差異

| Oryx | 本專案 |
|------|--------|
| `/terraform/v1` 平台 API、bearer token | 直接 `/api/v1` |
| Forwarding、Redis、AI、平台設定頁 | 只保留 SRS 核心頁面 |
| 多 scenario（source／live／record／transcode…） | MVP 先做監控與踢除；scenario 後續再加 |
| 播放器依賴平台 `/players` | 尚未整合播放器 |

## 建置與 CI

- 本機：`dev/build-ui.ps1` 建置並複製到 `build/Debug`（`-Target full` 可選 `build-full`）。
- CI：Nightly 在 `ui/` 執行 `npm ci`＋`npm run build`，複製到 `build/Debug/objs/nginx/html/console`，並在 smoke 檢查 `/console/` 回 200；artifact 也會帶上 console。
- Pages：與控制台無關；API 參考仍由 `Doxyfile` 產生。

## 已知限制

- `/api/v1/raw` 預設可能關閉（code 1061），Configs 頁只顯示提示。
- RTC／SRT 媒體驗證仍以 API 為準；播放器（WHEP／SRT player）尚未接入。
- 尚未提供登入／權限管理；請在反向代理或 SRS API 層處理。
