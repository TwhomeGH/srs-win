# Tutorial：從建置到第一個改動

> [!TIP]
> 這系列是**照著做**的練習，不是 API 參考。完成後要能回答：我改了哪個檔案、為什麼改這裡、怎麼驗證、下一個功能會重用哪個部分。

---

> [!WARNING]
> 請在個人分支或 `git worktree` 操作，不要直接改已部署的 `develop`。練習用的暫存檔不要 commit。

## 題目

| 題目 | 類型 | 狀態 |
| --- | --- | --- |
| [Learn 1：建置、啟動與 Docker](01-learn1-build-and-run.md) | 建置流程、Windows／Docker、VSCode 入口 | 完成 |
| [Build 配置導讀：CMake 與 Dockerfile](02-build-configuration.md) | CMake、Dockerfile、來源清單 | 完成 |
| [Learn 2：新增唯讀 API 欄位](03-learn2-readonly-api-field.md) | C++ API + console＋Doxygen 驗證 | 完成 |
| Learn 3：VSCode 除錯工作流 | launch／tasks／attach／Docker 邊界 | 規劃中 |
| Learn 4：新增 console 頁面 | React／Vite | 規劃中 |

## 練習模式

先開一份 worktree，避免影響目前工作區：

```powershell
git worktree add -b tutorial/learn1 ../srs-tutorial-learn1 develop
cd ../srs-tutorial-learn1
```

Linux／macOS：

```sh
git worktree add -b tutorial/learn1 ../srs-tutorial-learn1 develop
cd ../srs-tutorial-learn1
```

- baseline 是建立這份教學時的 `develop`；`git rev-parse --short HEAD` 會顯示實際 commit。
- 練習期間只改 worktree。
- 完成後先離開 worktree，再依照 [Git worktree 安全清理](../git-worktree.md) 送回收站；不要直接用 `git worktree remove --force` 丟掉未提交內容。
- 需要確認 `prune`／`repair` 的實際行為時，先查 [`git worktree` 官方文件與本機說明](../git-worktree.md#11-查證來源)。

```powershell
cd F:\srs
pwsh dev/remove-worktree.ps1 -Path F:\srs-tutorial-learn1 -DryRun
pwsh dev/remove-worktree.ps1 -Path F:\srs-tutorial-learn1
git branch -d tutorial/learn1
```

`git worktree remove` 是直接刪除，不進回收站；完整說明與手動流程見 [Git worktree 安全清理](../git-worktree.md)。

## 學習順序

```text
1. 先確認 Windows default 能建置、啟動、打 API
2. 再確認 Docker Linux lane 能建置、啟動、smoke
3. 讀 CMakePresets／CMakeLists／Dockerfile，知道每個開關在哪裡
4. 用 source list 檢查做一次「故意失敗」的練習
5. 進入 Learn 2，走一次唯讀 API → console → smoke 的改動
6. 才進入更大型的功能改動
```

## 為什麼先學建置

SRS 有兩條建置路徑：

```text
Windows：trunk/ide/srs_msvc/CMakeLists.txt + CMakePresets.json
Linux：  trunk/configure + dev/docker/Dockerfile
```

兩邊的來源清單、feature flag 與啟動方式不同。先搞清楚「哪個入口產生哪個執行檔」，後面的功能改動才不會改到一半才發現某一邊沒編到。

## 指令速查

| 目的 | 指令 |
| --- | --- |
| Windows 快速檢查 | `pwsh dev/check.ps1` |
| Windows 全功能 | `pwsh dev/check.ps1 -Full` |
| 只檢查來源清單 | `pwsh dev/check-source-lists.ps1` |
| 建置並啟動 Docker | `pwsh dev/local-deploy.ps1 -Deploy` |
| Docker smoke | `pwsh dev/local-deploy.ps1 -Test` |
| 本機 API 文件 | `pwsh dev/build-docs.ps1 -Open` |

## 完成標準

- [ ] 知道 `windows-default` 與 `windows-full` 的差別
- [ ] 能用 `dev/check.ps1` 建置並跑 native tests
- [ ] 能啟動 Docker Linux lane，HTTP API 回 200、RTMP handshake 成功
- [ ] 能指出新增 app source 時要改哪兩個檔案
- [ ] 知道 Docker build context 為什麼不能包含 `trunk/ide`
- [ ] 跑過一次 source list 的失敗檢查，並清掉練習檔
