# Git worktree：建立、驗證與安全清理

> [!TIP]
> 這篇只談 worktree 的生命週期。建置與啟動流程見 [Learn 1](tutorial/01-learn1-build-and-run.md)。

---

> [!WARNING]
> 清理 worktree 時**預設送回收站**，不要直接 `Remove-Item`、`rm -rf` 或 `git worktree remove --force`。`git worktree remove` 是直接刪除，沒有回收站。

## 1. worktree 是什麼

```text
F:\srs                        主 worktree（develop）
F:\srs-tutorial-learn1        另一個 worktree（tutorial/learn1）
```

同一個 repository 可以同時 checkout 不同分支到不同目錄。每個 worktree 有：

- 自己的檔案（tracked、untracked、ignored）
- 自己的 `HEAD` 與 index
- 一筆 Git metadata：`.git/worktrees/<name>`

它**不是**一個獨立 clone；分支、commit、remote 都共用主 repository。

## 2. 建立

```powershell
cd F:\srs
git worktree add -b tutorial/learn1 F:\srs-tutorial-learn1 develop
cd F:\srs-tutorial-learn1
```

Linux／macOS：

```sh
git worktree add -b tutorial/learn1 ../srs-tutorial-learn1 develop
cd ../srs-tutorial-learn1
```

一個分支同時間只能被一個 worktree checkout；要換到另一個 worktree，先切換或移除原本的。

## 3. 清理前先檢查

```powershell
cd F:\srs
git worktree list
git -C F:\srs-tutorial-learn1 status --short
git -C F:\srs-tutorial-learn1 log --oneline develop..HEAD
```

要確認：

| 檢查 | 目的 |
| --- | --- |
| `git worktree list` | 目標是否已註冊、路徑是否正確 |
| `status --short` | 有沒有未提交／未追蹤的變更 |
| `log develop..HEAD` | 有沒有尚未合併的 commit |
| `Get-Process`／Docker | 有沒有 SRS、node、LSP 等程序占用檔案 |

## 4. 什麼送回收站、什麼可以直接清

**預設：整個 worktree 目錄送回收站。** 即使 `git status` 乾淨，也可能有 ignored 的執行時產物或未追蹤內容。

| 目標 | 處理 | 理由 |
| --- | --- | --- |
| worktree 目錄（含 source、untracked） | **回收站** | 可能還有未提交內容，可還原 |
| 不確定用途的檔案／日誌 | **回收站** | 先保留證據 |
| `trunk/ide/srs_msvc/build*` | 可直接清（可重建） | CMake 產物，重建即可 |
| `dev/runtime/` | 可直接清（可重建） | HLS、log、bundle 等執行時狀態 |
| `ui/node_modules/`、`ui/build/` | 可直接清（可重建） | npm／Vite 產物 |
| `_site/` | 可直接清（可重建） | Doxygen／changelog 產物 |
| 測試暫存檔 | 先確認內容，再回收 | 可能是除錯證據 |

「可以直接清」只適用於**明確可重建**的產物，而且不要用整個 worktree 當目標。若沒有把握，選回收站。

## 5. 推薦流程：回收站優先

使用 repo 內的安全 helper：

```powershell
cd F:\srs

# 先預覽：不移動任何東西
pwsh dev/remove-worktree.ps1 -Path F:\srs-tutorial-learn1 -DryRun

# 確認後送回收站，並清除 Git 的 stale worktree metadata
pwsh dev/remove-worktree.ps1 -Path F:\srs-tutorial-learn1
```

helper 會：

1. 確認路徑是已註冊的 worktree，且不是主 worktree。
2. 列出未提交／未追蹤變更；沒有 `-DiscardChanges` 就拒絕。
3. 拒絕有程序正在從該 worktree 執行。
4. 使用 Windows Shell 的 Recycle Bin API 將整個目錄送回收站。
5. 執行 `git worktree prune`，確認 metadata 已移除。
6. **不刪分支**；需要時另外 `git branch -d`。

> helper 預設在回收後執行 `git worktree prune`。若你要保留「從回收站還原後仍能被 Git 認得」的能力，先不要 prune；prune 後回收站只保留檔案，Git worktree 連結已消失。

有未提交變更但確定不要時：

```powershell
pwsh dev/remove-worktree.ps1 -Path F:\srs-tutorial-learn1 -DiscardChanges
```

這仍然送回收站，不是永久刪除。若 helper 無法回收，會失敗並保留原狀；不會自動降級成 `Remove-Item`。

## 6. 手動送回收站（Windows）

如果不用 helper：

1. 先停止 SRS、Docker 與 VS Code 對該目錄的檔案監看。
2. 在檔案總管選取 worktree 資料夾，按一般 `Delete`（**不要 Shift+Delete**）。
3. 確認它出現在資源回收筒。
4. 回到主 repo 清除 metadata：

```powershell
git worktree prune
git worktree list
```

> 不要自行把資料夾搬到 `$RECYCLE.BIN`。請用檔案總管或 Shell API，才有原始路徑與還原資訊。

macOS 用 Finder 的「移到垃圾桶」；Linux 用 `gio trash <path>`（或桌面環境的垃圾桶），之後同樣執行 `git worktree prune`。

## 6.1 `git worktree prune` 會切斷 Git 連結

`git worktree prune` 不是刪檔案，也不是刪分支；它移除的是 `.git/worktrees/<name>` 這個**指向已不存在目錄**的 Git 管理記錄。

```powershell
git worktree list          # 可能顯示 prunable
git worktree prune -n -v   # 只預覽，不移除
git worktree prune -v      # 真正移除 stale metadata
```

兩種情況要分清楚：

| 情況 | 正確處理 |
| --- | --- |
| worktree 被移到新路徑，目錄還在 | 用 `git worktree repair` 修正路徑；**不要 prune** |
| worktree 目錄已刪除／已送回收站 | 確認不再需要 Git worktree 還原後，再 prune |

> **回收站保留的是檔案；prune 會讓 Git 不再認得那個 worktree。**
> 已 prune 後再從回收站還原到同一路徑，`git status` 會顯示 `not a git repository`；即使跑 `git worktree repair` 也無法重建已移除的管理記錄。此時只能重新 `git worktree add`，或把還原的檔案當一般檔案搬回。
> 如果還想保留「從回收站還原後直接繼續使用同一個 worktree」的能力，先不要 prune；等確定不還原時再執行。

## 7. `git worktree remove` 的行為

```powershell
git worktree remove F:\srs-tutorial-learn1
```

| 行為 | 說明 |
| --- | --- |
| 直接刪除 | 不進回收站，沒有 undo |
| 拒絕 dirty worktree | 有修改或 untracked 檔案時失敗 |
| `--force` | 連 modified／untracked 一起刪除，**不可回收** |
| 不移除分支 | `tutorial/learn1` 仍存在 |
| 不移除主 worktree | 不能刪目前 repo 的主目錄 |
| 目錄不存在時 | 用 `git worktree prune` 清 metadata，不是 remove |

### 剛建立的 worktree 也可能顯示 modified

Windows checkout 可能因 CRLF／BOM 讓新 worktree 出現只差行尾的修改，例如：

```text
 M trunk/3rdparty/srt-1-fit/scripts/test_vista.c
```

先確認是不是只有行尾差異：

```powershell
git -C F:\srs-tutorial-learn1 diff --ignore-space-at-eol --stat
git -C F:\srs-tutorial-learn1 diff --numstat
```

不要因為看到 `M` 就直接 `git worktree remove --force`。若確認不是你改的內容，仍建議用 `dev/remove-worktree.ps1 -DiscardChanges` 送回收站；這比直接刪除可還原。

只有在「這個 worktree 是拋棄式、內容已確認不需要、也不需要可還原」時才用 `git worktree remove --force`。一般教學與日常開發請用回收站流程。

## 8. 分支與 commit 的清理

worktree 移除後，分支不會自動消失：

```powershell
# 已合併
git branch -d tutorial/learn1

# 尚未合併但確定不要（會丟 commit 參照）
git branch -D tutorial/learn1
```

分支不是檔案，不進回收站。若誤刪，短時間內可從 reflog 找回：

```powershell
git reflog
git branch tutorial/learn1 <sha>
```

在刪分支前先確認：

```powershell
git log --oneline develop..tutorial/learn1
```

## 9. 常見狀況

| 現象 | 原因 | 處理 |
| --- | --- | --- |
| `contains modified or untracked files` | worktree 不乾淨 | commit、存 patch，或明確用 `-DiscardChanges` |
| 資料夾被占用／無法移動 | SRS、node、VS Code、LSP 正在使用 | 先停止程序，再回收 |
| `is not a working tree` | 路徑不是已註冊 worktree | `git worktree list` 確認路徑 |
| 目錄已手動刪除但 `git worktree list` 還在 | metadata stale | `git worktree prune` |
| 分支無法刪除 | 還被某個 worktree checkout | 先移除該 worktree，再刪分支 |
| 回收站不支援 | 網路磁碟／特殊檔案系統 | 停止，改用支援的儲存位置或手動保留；不要改成永久刪除 |

## 10. 一次完整的安全收尾

```powershell
cd F:\srs

# 1. 確認
git worktree list
git -C F:\srs-tutorial-learn1 status --short
git -C F:\srs-tutorial-learn1 log --oneline develop..HEAD

# 2. 停止占用程序
pwsh dev/srs-dev.ps1 -Stop
docker compose -p srs-dev -f dev/docker/docker-compose.yml down

# 3. 預覽後回收
pwsh dev/remove-worktree.ps1 -Path F:\srs-tutorial-learn1 -DryRun
pwsh dev/remove-worktree.ps1 -Path F:\srs-tutorial-learn1

# 4. 分支已合併才刪
git branch -d tutorial/learn1

# 5. 確認
git worktree list
git status --short
```

## 相關

- [Learn 1：建置、啟動與 Docker](tutorial/01-learn1-build-and-run.md)
- [Git worktree 官方文件](https://git-scm.com/docs/git-worktree)
- [文件索引](../文件索引.md)
