# Learn 1：建置、啟動與 Docker

> [!TIP]
> 這一題不新增功能，目標是把「改檔 → 建置 → 啟動 → 驗證 → 清理」跑過一次。
> 完成後再進 [Build 配置導讀](02-build-configuration.md)，理解每個開關的位置。
>
> [!WARNING]
> 請在個人分支或 worktree 操作。本文的練習檔 `srs_app_tutorial_probe.cpp` 只是用來觸發檢查，做完必須刪除，不可 commit。

## 目標

完成後你能：

1. 分辨 `windows-default` 與 `windows-full` 的差異。
2. 用 `dev/check.ps1` 建置並跑 native tests。
3. 啟動 Windows dev 版，確認 HTTP API。
4. 啟動 Docker Linux lane，確認 HTTP API 與 RTMP handshake。
5. 指出新增 app source 時要改哪兩個檔案。
6. 知道 Docker build context 為什麼不能包含 `trunk/ide`。

## 前置

- Windows：Visual Studio 2022、CMake、PowerShell 7。
- 全功能：先跑 `pwsh dev/setup-vscode.ps1`，建立 `.vscode/vcpkg` junction。
- Docker：Docker Desktop 已啟動。
- 不需要先安裝 ffmpeg；Docker lane 使用系統套件。

## 1. 確認工作區

```powershell
git status --short
git rev-parse --short HEAD
```

`git status` 應該是乾淨的；若有未提交變更，先處理，不要和練習混在一起。

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

預期失敗，訊息會指出它不在 `trunk/configure` 與 `CMakeLists.txt`：

```text
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
- [ ] Docker `-Test` 回 HTTP 200 與 RTMP 16 bytes
- [ ] 故意新增 source 時，source list 檢查失敗
- [ ] 練習檔已刪除，`git status` 乾淨

## 常見問題

| 現象 | 原因 | 處理 |
| --- | --- | --- |
| `PidFileAcquire` | `objs/` 不存在或殘留 pid | 建立 `objs/`，刪除對應 pid |
| 11985 already in use | 舊的 `srs.exe` 還在跑 | `Get-Process srs` 後停止，或用 `dev/srs-dev.ps1 -Stop` |
| Docker context 很大 | `.dockerignore` 沒有排除 build 產物 | 不要移除 `trunk/ide`、`trunk/build` 的排除規則 |
| full build 找不到 vcpkg | 沒有 `.vscode/vcpkg` junction | 跑 `pwsh dev/setup-vscode.ps1` |
