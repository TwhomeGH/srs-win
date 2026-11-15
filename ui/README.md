# SRS Web Console

完整說明在 [`../Docs/Web控制台.md`](../Docs/Web控制台.md)。

```powershell
pwsh dev/serve-ui.ps1      # Vite HMR → http://127.0.0.1:5173/console/
pwsh dev/build-ui.ps1      # 建置並安裝到 objs/nginx/html/console/
pwsh dev/rtc-e2e.ps1 ...   # full build 的 WHIP→WHEP 媒體 E2E
```

技術：React 18 + Vite 6 + Bootstrap 5，直接呼叫同源 `/api/v1`。播放器支援 WHEP、HTTP-FLV、HLS；FLV／HLS 以動態 import 載入。
