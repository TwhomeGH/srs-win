# API 註解指南

Doxygen **不會從函式簽名推測語意**，也不會把一般 `//` 註解放進 API 頁。頁面上要出現參數說明，必須在原始碼使用 Doxygen 註解，並明確寫 `@param`。

## 最小規則

| 註解形式 | Doxygen 是否採用 | 用途 |
|----------|------------------|------|
| `/** ... */` | ✅ | 類別、函式、欄位 |
| `/// ...` | ✅ | 單行成員／函式 |
| `// ...` | ❌ | 只給讀原始碼的人看 |
| `/* ... */` | 預設不當文件 | 區塊註解 |

## 函式範例

```cpp
/**
 * @brief Split one RTMP chunk into its basic header and message header.
 * @details 讀取下一個 chunk，依 fmt_ 決定要更新哪些 header 欄位。
 *
 * @param chunk 目標 chunk stream，不能為 NULL。
 * @param pmsg 輸出訊息；成功時由呼叫端負責釋放。
 * @return srs_success，或讀取／協定錯誤。
 */
virtual srs_error_t read_message_payload(SrsChunkStream *chunk, SrsRtmpCommonMessage **pmsg);
```

- `@param` 名稱必須與簽名完全一致，Doxygen 才能對應。
- `@return` 寫成功條件與錯誤種類，不要只寫「回傳 error」。
- 指標參數要寫「可不可以是 NULL」「誰負責釋放」。
- 識別字、型別、API 路徑保持英文；說明可用中文。

## 欄位範例

```cpp
/**
 * @brief RTMP chunk stream id carried by the basic header.
 */
int cid_;
```

公開欄位若沒有 Doxygen 註解，Doxygen 的 member list 只會顯示型別與名稱，也就是「能傳什麼」但不知道「代表什麼」。`SrsChunkStream` 已補上這組欄位說明。

## 涵蓋範圍

- 優先補 **公開標頭** 的 class／method／field；私有實作不需要為了文件而補。
- 新寫或修改的公開 API 一律補 `@brief`，有參數就補 `@param`。
- 既有 API 不要一次全改；依實際被 console、文件或除錯使用的順序逐步補。
- 一般 `//` 說明可以保留，但不要期待它出現在 Doxygen。

## 檢查方式

```powershell
# 產生 API 並看缺文件警告（目前 WARN_IF_UNDOCUMENTED=NO）
pwsh dev/build-docs.ps1

# 直接找某個類別的頁面
Get-Content _site/api/classSrsChunkStream.html | Select-String 'cid_|Chunk stream id'
```

若要開始量測缺口，可暫時把 `Doxyfile` 的 `WARN_IF_UNDOCUMENTED` 改成 `YES`；先當 backlog 清單，不要直接讓 CI 失敗。
