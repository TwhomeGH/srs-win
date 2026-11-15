# Learn 1：建置、啟動與 Docker

> [!TIP]
> 這一題不新增功能，目標是把「改檔 → 建置 → 啟動 → 驗證 → 清理」跑過一次。
> 完成後再進 [Build 配置導讀](02-build-configuration.md)，理解每個開關的位置。

---

> [!WARNING]
> 請在個人分支或 worktree 操作。本文的練習檔 `srs_app_tutorial_probe.cpp` 只是用來觸發檢查，做完必須刪除，不可 commit。
> 不要把它加進 `.gitignore`：`.gitignore` 只會讓 `git status` 看不到殘留檔，無法阻止編譯或被 `git add -f`；正確做法是刪除，source list 檢查也會拒絕它。

## 目標

完成後你能：

1. 分辨 `windows-default` 與 `windows-full` 的差異。
2. 用 `dev/check.ps1` 建置並跑 native tests。
3. 啟動 Windows dev 版，確認 HTTP API。
4. 啟動 Docker Linux lane，確認 HTTP API 與 RTMP handshake。
5. 指出新增 app source 時要改哪兩個檔案。
6. 知道 Docker build context 為什麼不能包含 `trunk/ide`。
7. 知道 VSCode 的「執行與偵錯」與「工作」各自用在什麼情境。

## 前置

- Windows：Visual Studio 2022、CMake、PowerShell 7。
- 全功能：先跑 `pwsh dev/setup-vscode.ps1`，建立 `.vscode/vcpkg` junction。
- Docker：Docker Desktop 已啟動。
- 不需要先安裝 ffmpeg；Docker lane 使用系統套件。

## 1. 確認工作區

這一步不是確認「程式能不能跑」，而是確認**你現在改的是哪一份工作區、哪個 commit，而且沒有把別人的／未完成的變更混進來**。

```powershell
git rev-parse --show-toplevel   # 目前是哪個 repo／worktree
git branch --show-current       # 目前是哪個分支
git status --short              # 有沒有未提交或未追蹤的變更
git rev-parse --short HEAD      # 這次練習的 baseline commit
```

| 指令 | 確認什麼 | 通過條件 |
| --- | --- | --- |
| `git rev-parse --show-toplevel` | 是否在正確的 SRS／tutorial worktree | 路徑是你要操作的那一份 |
| `git branch --show-current` | 是否在練習分支，不是直接改 `develop` | 顯示 `tutorial/learn1` 或你自訂的練習分支 |
| `git status --short` | 有沒有未提交變更、練習殘留檔 | **沒有輸出** |
| `git rev-parse --short HEAD` | 練習的 baseline commit | 記下短 hash，方便之後比較 diff |

如果 `git status --short` 有輸出，先不要開始：

- 已完成的變更：commit 或另開分支保存。
- 不想要的變更：確認內容後再丟棄，不要直接 `git reset --hard`。
- 練習殘留檔（例如 `srs_app_tutorial_probe.cpp`）：刪除後再確認一次。

`git rev-parse --short HEAD` 只回報 commit，不會修改任何東西；它和 SRS 的版本號 `7.0.89` 是兩件事。

## 2. Windows default build

先跑來源清單檢查，再跑快速建置與測試：

```powershell
pwsh dev/check-source-lists.ps1
pwsh dev/check.ps1
```

預期看到：

```text
source list check: OK (56 app sources in configure and CMake)
...
check OK: ...s (default)
```

`dev/check.ps1` 會：

```text
檢查 source list
→ 建置 srs 與 native tests
→ 跑 ctest
```

它不會 clean，適合每次改碼後跑。

## 3. 啟動 Windows dev 版

```powershell
pwsh dev/srs-dev.ps1 -Build -Run
```

它會把 `build/Debug/srs.exe` 複製到 `dev/runtime/windows/srs-dev.exe`，並用 dev 設定啟動。

確認 API：

```powershell
Invoke-RestMethod http://127.0.0.1:11985/api/v1/versions | ConvertTo-Json
```

預期回應包含版本與 build 日期。再確認頁面：

```text
http://127.0.0.1:11985/console/
```

停止：

```powershell
pwsh dev/srs-dev.ps1 -Stop
```

> Windows dev 設定的埠號是 11935／11985／18088；若被其他程式佔用，先停止舊的 `srs.exe`，不要直接改設定檔逃避衝突。

## 3.1 用 VSCode 啟動（可選）

VSCode 不是另一條建置路徑；它呼叫的是同一批 CMake target 與 `dev/` 腳本。先開**workspace 根目錄**：

```text
File → Open Folder → F:\srs
```

不要只開 `trunk/` 或 `.vscode/`，否則 `${workspaceFolder}` 與 task 路徑會不對。

### 「執行與偵錯」下拉

| 設定 | 用途 |
| --- | --- |
| `SRS Windows (default features, Debug)` | 日常 API／HLS／RTMP／console，feature set 較小 |
| `SRS Windows (full RTC/SRT, Debug)` | WebRTC／SRT 測試 |
| `SRS event loop test` | 只跑 st 協程測試，不是 SRS 服務 |
| `Attach to srs.exe` | 附加到已啟動的程序 |
| `UI: Vite dev server` | 啟動 console 前端 |
| `UI: Vitest (Windows)` | Web 單元測試 |
| `UI: RTC E2E (full, no auth)` | 自動啟動 full SRS 並跑 WHIP／WHEP |
| `UI: Auth E2E (full, auth)` | 自動啟動 auth SRS 並跑登入 |

日常完整功能測試選：

```text
SRS Windows (full RTC/SRT, Debug)
```

兩個 SRS 設定都是 Debug build；差別是 feature set，不是 Release 最佳化。

### 「工作」清單

按：

```text
Ctrl+Shift+P → Tasks: Run Task
```

常用工作：

| 工作 | 用途 |
| --- | --- |
| `dev: check Windows` | 增量建置＋native tests |
| `dev: check Windows full` | RTC／SRT 建置＋tests |
| `dev: test UI` | Web Vitest |
| `dev: lint docs` | Markdown 檢查 |
| `dev: Docker deploy` | 建置並重建 Linux 容器 |
| `dev: Docker smoke` | 部署後 HTTP／RTMP smoke |
| `dev: Docker shell` | 進入 Linux 容器 |
| `dev: RTC E2E (no auth)` | Windows full＋Playwright |
| `dev: Auth E2E (auth)` | Windows auth＋Playwright |

測試類工作也可以用：

```text
Tasks: Run Test Task
```

### 建議情境

```text
日常 SRS：SRS Windows (default features, Debug)
完整功能：SRS Windows (full RTC/SRT, Debug)
前端：UI: Vite dev server
Linux：Tasks → dev: Docker smoke／dev: Docker shell
```

Docker 車道目前是**執行與 smoke**，不是 gdb 原始碼除錯；不要期待在 Docker task 裡設中斷點。

### 練習

用 VSCode 再跑一次同一個流程：

1. 選 `SRS Windows (default features, Debug)` 按 F5。
2. 確認 `http://127.0.0.1:11985/api/v1/versions` 回應。
3. 按 Shift+F5 停止，或執行 `dev/srs-dev.ps1 -Stop`。
4. 從 `Tasks: Run Task` 執行一次 `dev: check Windows`。

完整 launch／tasks 說明見 [VSCode 除錯手冊](../../VSCode除錯手冊.md)。

## 4. 全功能建置（需要 RTC／SRT 時）

```powershell
pwsh dev/check.ps1 -Full
```

差異：

| 建置 | binaryDir | 功能 |
| --- | --- | --- |
| `windows-default` | `trunk/ide/srs_msvc/build` | 預設功能，無 RTC／SRT |
| `windows-full` | `trunk/ide/srs_msvc/build-full` | vcpkg 的 RTC／SRT |

兩個都是 Debug build；差別是 feature set，不是 Release 最佳化。

如果你只是測 HTTP API、HLS、RTMP，default 就夠了。

## 5. Docker Linux lane

建置並啟動：

```powershell
pwsh dev/local-deploy.ps1 -Deploy
```

只檢查狀態、不重建：

```powershell
pwsh dev/local-deploy.ps1 -Check
```

部署後跑 smoke：

```powershell
pwsh dev/local-deploy.ps1 -Test
```

預期：

```text
HTTP API: 200
RTMP handshake: 16 bytes
```

Docker lane 使用的是：

```text
dev/docker/Dockerfile
dev/docker/docker-compose.yml
dev/docker/dev.conf
```

不是 Windows CMake 的 `build-full`。

停止：

```powershell
docker compose -p srs-dev -f dev/docker/docker-compose.yml down
```

## 6. 做一次「來源清單故意失敗」

新增一個空的 app source：

```powershell
Set-Content trunk/src/app/srs_app_tutorial_probe.cpp "// tutorial probe"
pwsh dev/check-source-lists.ps1
```

預期失敗，訊息會先提示練習檔，再指出它不在 `trunk/configure` 與 `CMakeLists.txt`：

```text
tutorial probe detected: delete trunk/src/app/srs_app_tutorial_probe.cpp
missing from trunk/configure:
  srs_app_tutorial_probe
missing from CMakeLists.txt:
  srs_app_tutorial_probe
source list check: FAIL
```

刪掉練習檔：

```powershell
Remove-Item trunk/src/app/srs_app_tutorial_probe.cpp
git status --short
```

`git status` 應該沒有這個檔案。這個練習說明：新增 app source 時，**兩個清單都要改**；檢查腳本只是防止忘記。

這個練習故意讓 probe **同時漏掉兩邊**，讓你一次看到兩個方向的錯誤。實際事故不是「CMake 漏檔」，而是：

```text
srs_app_console_auth.cpp 加到 CMake
但沒有加到 trunk/configure
→ Windows 正常
→ Linux 連結 undefined reference
```

所以檢查腳本比對的是 **CMake 與 `trunk/configure` 兩份清單是否一致**，不是只檢查 CMake。真正事故會顯示：

```text
missing from trunk/configure:
  srs_app_console_auth
```

## 7. 你應該能回答

- `windows-default` 和 `windows-full` 分別輸出到哪個目錄？
- 為什麼只是測 HLS／API 時不需要 full build？
- Docker build context 為什麼排除 `trunk/ide`？
- 新增 `trunk/src/app/srs_app_foo.cpp` 時，要改哪兩個檔案？
- `dev/check.ps1` 和 `dev/local-deploy.ps1` 分別驗證哪個平台？

## 檢查點

- [ ] `dev/check-source-lists.ps1` 回 OK
- [ ] `dev/check.ps1` 建置＋ctest 通過
- [ ] Windows dev API 回 200
- [ ] 能用 VSCode 的 launch 設定啟動 SRS，並從 Tasks 跑一次 `dev: check Windows`
- [ ] Docker `-Test` 回 HTTP 200 與 RTMP 16 bytes
- [ ] 故意新增 source 時，source list 檢查失敗
- [ ] 練習檔已刪除，`git status` 乾淨

## 延伸閱讀：API 參考

Learn 1 不教 API 註解或 Doxygen 設定；這些放在單一文件入口，避免教學與工具文件各自漂移。

需要查本地 API 參考時：

```powershell
pwsh dev/build-docs.ps1 -Open
```

輸出：

```text
_site/api/index.html
```

| 想知道什麼 | 看哪裡 |
| --- | --- |
| 指令與最短入口 | [文件索引](../../文件索引.md) |
| Doxygen 首頁與閱讀順序 | [API 導讀](../../API導讀.md) |
| `@brief`／`@param`／`@return` 怎麼寫 | [API 註解指南](../../API註解指南.md) |

Learn 2 開始改 API 時，才會把「產生 API 參考 → 檢查類別／函式頁面」當成驗證步驟寫進題目。

## 常見問題

| 現象 | 原因 | 處理 |
| --- | --- | --- |
| `PidFileAcquire` | `objs/` 不存在或殘留 pid | 建立 `objs/`，刪除對應 pid |
| 11985 already in use | 舊的 `srs.exe` 還在跑 | `Get-Process srs` 後停止，或用 `dev/srs-dev.ps1 -Stop` |
| Docker context 很大 | `.dockerignore` 沒有排除 build 產物 | 不要移除 `trunk/ide`、`trunk/build` 的排除規則 |
| full build 找不到 vcpkg | 沒有 `.vscode/vcpkg` junction | 跑 `pwsh dev/setup-vscode.ps1` |
