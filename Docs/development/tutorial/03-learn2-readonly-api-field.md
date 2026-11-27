# Learn 2：新增唯讀 API 欄位

> [!TIP]
> 這一題會把一個欄位從 **C++ API → JSON → console → smoke test → Doxygen** 走完。
> 範例是 `/api/v1/config` 的 `platform` 欄位；完成後你應該能照同樣流程新增其他唯讀欄位。

---

> [!WARNING]
> 請在個人分支或 worktree 操作，不要直接改 `develop`。
> 本題只做唯讀欄位，不要順手開啟 `/api/v1/raw` 或加入可寫入的設定端點。

## 目標

完成後你能：

1. 找到 `/api/v1/config` 的實作位置。
2. 用 `SrsJsonAny` 增加一個唯讀 JSON 欄位。
3. 在 Web 控制台顯示這個欄位。
4. 用 smoke test 驗證欄位真的出現在 API 回應。
5. 判斷哪些變更需要 Doxygen 註解、哪些只需要實作註解。
6. 用 `dev/check.ps1`、UI 測試與文件檢查完成閉環。

## 這個範例改哪些檔案

| 層 | 檔案 | 改動 |
| --- | --- | --- |
| 後端 API | `trunk/src/app/srs_app_http_api.cpp` | `SrsGoApiConfig` 增加 `platform` |
| Web 控制台 | `ui/src/pages/Configs.jsx` | Server 卡片顯示執行平台 |
| 介面文字 | `ui/src/i18n.jsx` | `configPlatform` 的 zh／en 字串 |
| 回歸驗證 | `dev/smoke-test.ps1` | 檢查 `data.platform` 存在 |
| 教學索引 | `Docs/development/tutorial/README.md` | 把 Learn 2 標成完成 |

## 1. 建立練習 worktree

```powershell
cd F:\srs
git worktree add -b tutorial/learn2 ..\srs-tutorial-learn2 develop
cd ..\srs-tutorial-learn2
git status --short
git rev-parse --short HEAD
```

`git status --short` 應該沒有輸出；`git rev-parse --short HEAD` 是你的 baseline。

Linux／macOS：

```sh
git worktree add -b tutorial/learn2 ../srs-tutorial-learn2 develop
cd ../srs-tutorial-learn2
```

## 2. 先找資料流，不要先改畫面

API 的來源是：

```text
trunk/src/app/srs_app_http_api.cpp
  SrsGoApiConfig::serve_http()
```

搜尋：

```powershell
Select-String -Path trunk\src\app\srs_app_http_api.cpp `
  -Pattern 'SrsGoApiConfig::serve_http' -Context 0,60
```

你會看到 JSON 的結構是：

```text
{
  "code": 0,
  "data": {
    "version": "...",
    "build": "...",
    "build_ts": "...",
    "capabilities": { ... },
    ...
  }
}
```

`data` 是 console 讀取的部分；新增欄位要放在 `data` 內，不要放到最外層。

## 3. 後端：增加 `platform`

在 `serve_http()` 的版本與 build 欄位旁加入：

```cpp
    data->set("version", SrsJsonAny::str(RTMP_SIG_SRS_VERSION));
    data->set("build", SrsJsonAny::str(SRS_BUILD_DATE));
    data->set("build_ts", SrsJsonAny::str(SRS_BUILD_TS));
    // Compile-time platform identifier. Keep it stable and untranslated so the
    // console and API clients can branch on it without parsing build strings.
#if defined(_WIN32)
    data->set("platform", SrsJsonAny::str("windows"));
#elif defined(__APPLE__)
    data->set("platform", SrsJsonAny::str("macos"));
#else
    data->set("platform", SrsJsonAny::str("linux"));
#endif
```

設計理由：

- `platform` 是編譯期值，不會在執行中改變，適合放在唯讀 config。
- 回傳值是固定的英文識別字，不翻譯；console 自己決定要顯示什麼。
- 不要從 `build` 字串或檔名推測平台；那會讓 client 依賴不穩定的格式。

## 4. 註解：Doxygen 與 JSON 欄位是兩件事

`platform` 是 JSON 回應欄位，不是 C++ class 的成員，所以不會自動出現在 Doxygen 的 `SrsGoApiConfig` 成員列表。這種情況：

- 在 C++ 實作旁用一般 `//` 說明「為什麼」與「穩定性」。
- 若修改了公開 class／method／field，才補 `@brief`／`@param`／`@return`。
- 不要為了讓 Doxygen 顯示 JSON 欄位而虛構不存在的 C++ 參數。

本範例只需要實作註解；`SrsGoApiConfig` 的類別說明已存在。這是 change-scoped 註解原則：改到哪裡，補到哪裡。

驗證文件產生：

```powershell
pwsh dev/build-docs.ps1
```

確認 `SrsGoApiConfig` 頁面仍可開啟即可；不要在 class 頁面期待看到 JSON 的 `platform` 欄位。

## 5. 前端：在 Settings 頁顯示

`ui/src/pages/Configs.jsx` 的 Server 卡片目前顯示版本與建置日期。加在 build 日期後面：

```jsx
<div className="text-secondary small mt-2">{t('configPlatform')}</div>
<div>{config.platform || '-'}</div>
```

介面文字加在 `ui/src/i18n.jsx`：

```js
// zh
configPlatform: '執行平台',

// en
configPlatform: 'Runtime platform',
```

不要把 API 回傳的 `windows`／`linux`／`macos` 翻譯成中文；它們是穩定的識別字。要顯示友善名稱時，另外做 mapping，但 API 值保持不變。

## 6. 建置與啟動

先跑 native 檢查與 UI 測試：

```powershell
pwsh dev/check.ps1
pwsh dev/check.ps1 -Ui
```

建置 console 並啟動 dev 版：

```powershell
pwsh dev/build-ui.ps1 -Target default
pwsh dev/srs-dev.ps1 -Build -Run
```

API：

```powershell
Invoke-RestMethod http://127.0.0.1:11985/api/v1/config |
  ConvertTo-Json -Depth 6
```

預期 `data.platform` 為：

```text
windows
```

Windows default build 的 console：

```text
http://127.0.0.1:11985/console/
```

進入「設定」頁，Server 卡片應顯示：

```text
執行平台
windows
```

若 `/console/` 回 404，先確認 `dev/build-ui.ps1 -Target default` 成功，再重新跑 `srs-dev.ps1 -Build`；`srs-dev` 會把建置好的 console 複製到 runtime bundle。

停止：

```powershell
pwsh dev/srs-dev.ps1 -Stop
```

## 7. 加一條真正的回歸檢查

只手動看一次 API 不夠；把欄位加入共用 smoke test，Nightly 與 Release 都會驗證。

`dev/smoke-test.ps1` 的 config API 區塊加入：

```powershell
if (-not $config.data.platform) {
    throw 'config API missing platform field'
}
```

這樣如果未來重構 `SrsGoApiConfig` 時漏掉欄位，smoke 會直接失敗，而不是等 console 顯示 `-` 才發現。

手動跑 smoke（使用 dev 設定的埠號，避開系統保留的 1985）：

```powershell
pwsh dev/smoke-test.ps1 `
  -SrsExe trunk\ide\srs_msvc\build\Debug\srs.exe `
  -ConfFile dev\windows\dev.conf `
  -WorkDir dev\runtime\windows `
  -ApiPort 11985 `
  -RtmpPort 11935
```

預期：

```text
Config API OK: ... platform=windows
smoke: PASS
```

## 8. 文件與提交

更新：

```text
Docs/ChangeHistory.md
Docs/development/tutorial/README.md
```

ChangeHistory 標題要含時間：

```markdown
## 2026.11.27 15:11 Learn 2：新增唯讀 API 欄位
```

提交前：

```powershell
pwsh dev/lint-docs.ps1
pwsh dev/check.ps1
pwsh dev/check.ps1 -Ui
git diff --check
git status --short
```

提交訊息範例：

```text
feat(api): 在唯讀 config 增加 platform 欄位
```

## 9. 清理 worktree

先離開練習目錄，回到主 repo：

```powershell
cd F:\srs
pwsh dev/remove-worktree.ps1 -Path F:\srs-tutorial-learn2 -DryRun
pwsh dev/remove-worktree.ps1 -Path F:\srs-tutorial-learn2
git branch -d tutorial/learn2
```

`remove-worktree.ps1` 會把整個 worktree 送系統回收站，再執行 `git worktree prune`；不要用 `--force` 直接丟掉未提交內容。完整規則見 [Git worktree 安全清理](../git-worktree.md)。

## 完成標準

- [ ] 能指出 `SrsGoApiConfig::serve_http()` 在哪裡
- [ ] API 回應的 `data.platform` 為 `windows`／`linux`／`macos`
- [ ] Settings 頁的 Server 卡片顯示執行平台
- [ ] smoke test 會檢查 `platform`
- [ ] `dev/check.ps1` 4/4、`dev/check.ps1 -Ui` 通過
- [ ] `dev/lint-docs.ps1` 0 errors
- [ ] 知道 JSON 欄位不會自動出現在 Doxygen class 頁面
- [ ] 練習 worktree 已送回收站，沒有殘留分支

## 常見錯誤

| 症狀 | 原因 | 處理 |
| --- | --- | --- |
| API 沒有 `platform` | 改到 `/api/v1/versions` 或最外層 | 確認加在 `SrsGoApiConfig::serve_http()` 的 `data` |
| console 顯示 `-` | UI 沒重建，或 API 仍是舊 binary | 重跑 `build-ui.ps1` 與 `srs-dev.ps1 -Build` |
| `/console/` 404 | runtime bundle 沒有 console | 先建置 console，再啟動 |
| 想要把 `windows` 翻成中文 | 把識別字當文案 | API 保持英文；顯示層另做 mapping |
| Doxygen 頁面找不到欄位 | JSON 欄位不是 C++ 成員 | 這是預期行為；用 API 文件與 smoke 驗證 |
