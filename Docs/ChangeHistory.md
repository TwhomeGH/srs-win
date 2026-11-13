# SRS-Win 變更歷史

最新在最上。格式：`## YYYY.MM.DD HH:MM 一句話標題`，內文分「問題／根因／修改」。

## 2026.11.13 14:30 接入 changelogkit、建立主待辦與 Pages 靜態站

**類型**: 工具鏈 · **檔案**: `.changelog.toml`, `TODO.md`, `.github/workflows/pages.yml`

### 問題 - 變更紀錄與待辦散落各處，沒有可瀏覽的歷史與單一入口
CI 修正的來龍去脈只存在 commit 訊息與對話裡；待辦同時散落在 `Docs/已知限制與待辦事項.md` 與臨時筆記。

### 根因 - 缺乏版本控管內的紀錄格式與索引工具
專案沒有 `Docs/ChangeHistory.md`，也沒有把待辦集中成一份可解析的清單；文件檢視只能靠 GitHub 逐檔翻。

### 修改 - 以 changelogkit 作為歷史與待辦的單一入口
- 新增 `.changelog.toml`：指定歷史檔、`TODO.md`，並把 `Docs/` 分成「建置與使用」「設計與限制」兩組。
- 新增 `Docs/ChangeHistory.md`（本檔）作為變更紀錄來源。
- 新增 `TODO.md` 作為主待辦清單，changelogkit 解析 `- [ ]`／`- [x]`。
- 新增 `.github/workflows/pages.yml`：push `develop` 時匯出唯讀靜態站並部署 GitHub Pages。
- Pages 安裝 changelogkit 時解析最新 tag，不釘死版本；需要可重現時用 repo 變數 `CHANGELOGKIT_REF` 覆寫。

---

## 2026.11.13 14:10 修正 Nightly smoke test 的設定檔與埠號不一致

**類型**: 修復 · **檔案**: `.github/workflows/windows-nightly.yml`, `.github/workflows/release.yml`, `trunk/conf/test.conf`

### 問題 - Smoke test 呼叫 `localhost:1985`，但 `test.conf` 的 `http_api` 監聽 19850
API 請求逾時或連錯埠，Nightly 在 smoke test 階段失敗。

### 根因 - 設定檔與 workflow 各自維護埠號，沒有單一來源
`test.conf` 使用 19850，workflow 的 URL 曾改為 1985；兩邊沒有互相驗證。

### 修改 - 統一回到原本設計的 1985，兩份 workflow 一起修正
- `test.conf` 的 `http_api listen` 改為 1985。
- `windows-nightly.yml`／`release.yml` 的 URL 統一為 `http://localhost:1985/api/v1/versions`，並延長啟動等待與請求逾時。
- 啟動前清除殘留 `objs/srs.pid` 並確保 `objs/` 目錄存在。

---

## 2026.11.13 14:00 Nightly 改為僅 push 觸發

**類型**: 調整 · **檔案**: `.github/workflows/windows-nightly.yml`

### 問題 - 固定排程每天觸發建置，沒有新推送也重複消耗 CI
排程建置與 push 建置內容重複，失敗時也難以對應到具體變更。

### 根因 - workflow 同時設定 `schedule` 與 `push` 觸發
兩者都會啟動同一份 job。

### 修改 - 移除 `schedule`，只保留 push 與手動觸發
`on` 只剩 `push: branches: [develop]` 與 `workflow_dispatch`。

---
