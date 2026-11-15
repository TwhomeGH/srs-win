# VSCode（Windows）

Windows 除錯設定已改為 `cppvsdbg`（Visual Studio 偵錯器）。完整說明在
[`Docs/VSCode除錯手冊.md`](../Docs/VSCode除錯手冊.md)；這裡只列常用入口。

## 啟動設定

| 名稱 | 用途 |
| ------ | ------ |
| `SRS Windows (Debug)` | 預設建置，`dev/windows/dev.conf` |
| `SRS Windows (Full RTC/SRT)` | vcpkg 全功能建置（先跑 `dev/setup-vscode.ps1`） |
| `SRS event loop test` | st-srs 協程／select 回歸測試 |
| `Attach to srs.exe` | 附加到已啟動的程式 |
| `SRS proxy (Go)` | proxy 除錯 |
| `UI: Vitest (Windows)` | 在整合終端執行 Web 單元測試 |
| `UI: Vite dev server` | 啟動 console dev server |
| `UI: RTC E2E (Windows full)` | 啟動 full SRS 並跑 WHIP／WHEP |
| `UI: Auth E2E (Windows auth)` | 啟動 auth SRS 並跑登入流程 |

## 工作

| 工作 | 說明 |
| ------ | ------ |
| `build-srs-debug` | 增量建置 `srs` 與測試（不 clean） |
| `build-srs-full` | `windows-full` preset（RTC／SRT） |
| `test-srs` / `test-srs-full` | 執行 `ctest` |
| `docs-api` | 產生並開啟 `_site/api` |
| `deploy-dev-container` | 建置並重建 Docker 開發容器 |
| `dev: check Windows` | 增量 Windows 建置＋native tests |
| `dev: check Windows full` | 全功能（RTC／SRT）建置＋tests |
| `dev: test UI` | Web console Vitest |
| `dev: lint docs` | Markdown 檢查 |
| `dev: Docker deploy` | 建置並重建 Linux dev 容器 |
| `dev: Docker smoke` | 部署後檢查 HTTP API／RTMP |
| `dev: Docker shell` | 進入 Linux 容器 bash |
| `dev: RTC E2E` | Windows full SRS＋Playwright |
| `dev: Auth E2E` | Windows auth SRS＋Playwright |

## 測試通道怎麼選

VSCode 的「執行與偵錯」下拉選單只放**除錯器**；測試與 Docker 指令放在「工作」清單，按 `Ctrl+Shift+P` → `Tasks: Run Task`（或 `Tasks: Run Test Task`）直接選。

| 目的 | 入口 |
|------|------|
| Windows C++ 除錯 | `SRS Windows (Debug)`／`SRS Windows (Full RTC/SRT)` |
| Windows native tests | 工作 `dev: check Windows`／`dev: check Windows full` |
| Web 單元測試 | 除錯選單 `UI: Vitest (Windows)` 或工作 `dev: test UI` |
| Web／RTC／auth E2E | 除錯選單 `UI: RTC E2E`／`UI: Auth E2E`，或對應 `dev:` 工作 |
| Linux Docker 測試 | 工作 `dev: Docker smoke`（部署＋HTTP／RTMP） |
| Linux Docker shell | 工作 `dev: Docker shell` |

Linux C++ 的原始碼層除錯還沒接進 `launch.json`；目前 Docker 車道是**執行與 smoke 測試**，不是 gdb 除錯。若要在容器內逐步偵錯，下一步應加 Dev Containers（`ms-vscode-remote.remote-containers`）與 gdb，而不是把 `docker compose` 假裝成 debugger。

## 命令列替代方案

```powershell
pwsh dev/check.ps1                 # 快速增量建置＋測試
pwsh dev/check.ps1 -Full           # 全功能（RTC/SRT）
pwsh dev/local-deploy.ps1 -Deploy  # 固定 Docker dev 容器
pwsh dev/build-docs.ps1 -Open      # 本機 API 參考
```

## 前置需求

- Visual Studio 2022（C++ 桌面開發、MASM）
- VSCode 擴充：**C/C++**（`ms-vscode.cpptools`）
- 全功能建置：先跑 `pwsh dev/setup-vscode.ps1` 建立 `.vscode/vcpkg` junction（自動尋找 vcpkg，不需固定磁碟機）
- Docker 車道：Docker Desktop

文件入口：[`Docs/文件索引.md`](../Docs/文件索引.md)。
