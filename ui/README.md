# SRS Web Console

`ui/` 是 SRS 本體的 Web 控制台，參考 Oryx `ui/src/pages/SrsConsole.js` 的
資料模型與監控版面，重新整理成只依賴 SRS 原生 HTTP API 的獨立 SPA。

## 快速開始

```powershell
# 開發模式（Vite HMR，需 SRS 已啟動）
pwsh dev/serve-ui.ps1
# 開啟 http://127.0.0.1:5173/console/

# 建置並安裝到 SRS 的 /console/
pwsh dev/build-ui.ps1
# 開啟 http://127.0.0.1:11985/console/
```

## 設計

| 項目 | 選擇 |
|------|------|
| 技術 | React 18 + Vite 6 + Bootstrap 5 + react-bootstrap |
| API | 同源 `/api/v1`（開發時由 Vite proxy 轉到 SRS） |
| 頁面 | Overview／Vhosts／Streams／Clients／Configs |
| 認證 | 目前依賴 SRS API 本身；若前面有反向代理認證，瀏覽器會沿用其 session |
| 建置輸出 | `ui/build/` → `objs/nginx/html/console/`（SRS `http_api` 的 `/console` 掛載點） |

## 與 Oryx 的差異

Oryx 的 console 依賴 `/terraform/v1` 平台 API、Redis、forwarding 與 bearer token；
這個 SRS 版本只保留直接對應 SRS HTTP API 的部分：

- 移除 forwarding／Redis／AI／平台設定頁。
- `srsApi` 直接呼叫 `/api/v1`，不再經過 Oryx 平台代理。
- 保留 Overview、Vhosts、Streams、Clients、Configs 與錯誤邊界。
- 新增 zh-TW／en 切換與自動刷新。

## 已知限制

- SRS `/api/v1/raw` 未啟用時（`code=1061`），Configs 頁顯示提示而不是原始設定。
- RTC／SRT 的媒體驗證仍以 API 為準；播放器頁面尚未整合。
- 尚未加入 SRS API 的 Basic Auth 登入表單（預設 SRS API 不驗證）。
