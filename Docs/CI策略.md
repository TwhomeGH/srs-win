# CI 策略

**原則**：本地優先。日常開發不需要等 CI 才有可執行的產物；CI 只負責夜間穩定驗證與發佈。

## 分層

| 層級 | 觸發 | 產物 | 用途 |
|------|------|------|------|
| 本機預設 | 開發者主動執行 | `trunk/ide/srs_msvc/build/Debug` | 快速建置、VSCode 偵錯、`dev/check.ps1` |
| 本機 dev lane | `dev/srs-dev.ps1 -Build -Run` | `dev/runtime/windows`／Docker | 測試執行檔與容器，不依賴 CI |
| 本機部署 | `dev/local-deploy.ps1 -Deploy` | Docker container | 固定開發容器 |
| **Windows Nightly** | 每天 03:00（台北）**且 develop 有新 commit**＋手動 | `srs-win-nightly-x64-debug`（Debug） | 晚間診斷通道；Release 建置待修 |
| Release | tag `v*` | Release package | 正式版本 |
| Pages | `Docs/**`／`README.md`／`trunk/src/**/*.hpp` 等文件變更＋手動 | `_site`（changelog／Doxygen） | 文件與 API 站 |

## 為什麼不每 push 建置

- 每次 push 都跑 Windows 建置與 console 建置會消耗 CI 時間，且本地 `dev/check.ps1` 已能提供更快的回饋。
- Nightly 用 **Debug** 組態；Release 建置目前被 `st-event-loop-test` 的 Release segfault 擋住，修好前不產 Release nightly。日常開發與 Nightly 使用同一種組態，避免把未驗證的 Release 產物當成穩定版。
- Nightly 的 schedule 只是一個觸發點；workflow 會先比對「上次成功 nightly 的 commit」與目前 `develop`，沒有新 commit 就直接跳過，不燒 Windows runner。
- 需要立即驗證時，在 GitHub Actions 頁面按 **Run workflow**（`workflow_dispatch`）。
- Pages 沒有無條件每日排程；只有文件來源變更（`Docs/**`、`README.md`、API 標頭 `trunk/src/**/*.hpp` 等）或手動觸發才重建。

## 本機指令

```powershell
# C++／Web 快速檢查
pwsh dev/check.ps1 -Ui

# 專屬測試執行檔＋容器
pwsh dev/srs-dev.ps1 -Build -Run
pwsh dev/srs-dev.ps1 -Platform docker -Build -Run

# 本機 API 文件
pwsh dev/build-docs.ps1
```

## Nightly 的責任

- 建置 Debug（預設 RTC／SRT 關閉）。
- 跑 `st-event-loop-test`、`path-rename-test`、`file-share-test`。
- 跑 UI Vitest、建置 console。
- Smoke：HTTP API、RTMP handshake、`/console/`、`/api/v1/config`。
- 上傳 artifact 保留 14 天。

## 待修

- `st-event-loop-test` 在 Release 組態會 segfault（Debug 正常）；修好後再把 Nightly 改成 Release artifact。

## 目前不做

- 不在每次 push 自動建置。
- 不把 Pages 當成每個 commit 的部署開關。
- 不在 CI 執行 `windows-full`（RTC／SRT）；那是後續獨立的 full job。
