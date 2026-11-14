# VSCode（Windows）

Windows 除錯設定已改為 `cppvsdbg`（Visual Studio 偵錯器）。完整說明在
[`Docs/VSCode除錯手冊.md`](../Docs/VSCode除錯手冊.md)；這裡只列常用入口。

## 啟動設定

| 名稱 | 用途 |
|------|------|
| `SRS Windows (Debug)` | 預設建置，`dev/windows/dev.conf` |
| `SRS Windows (Full RTC/SRT)` | vcpkg 全功能建置（需 `VCPKG_ROOT`） |
| `SRS event loop test` | st-srs 協程／select 回歸測試 |
| `Attach to srs.exe` | 附加到已啟動的程式 |
| `SRS proxy (Go)` | proxy 除錯 |

## 工作

| 工作 | 說明 |
|------|------|
| `build-srs-debug` | 增量建置 `srs` 與測試（不 clean） |
| `build-srs-full` | `windows-full` preset（RTC／SRT） |
| `test-srs` / `test-srs-full` | 執行 `ctest` |
| `docs-api` | 產生並開啟 `_site/api` |
| `deploy-dev-container` | 建置並重建 Docker 開發容器 |

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
- 全功能建置：`$env:VCPKG_ROOT` 指向 vcpkg（例如 `H:\vcpkg`）
- Docker 車道：Docker Desktop

文件入口：[`Docs/文件索引.md`](../Docs/文件索引.md)。
