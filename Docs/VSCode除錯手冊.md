# VSCode 除錯手冊（Windows）

用 Visual Studio 偵錯器（`cppvsdbg`）啟動 `build/Debug/srs.exe`；PDB、`test.conf`、OpenSSL DLL 都在同一個輸出目錄，不需要另外設定 PATH。

## 前置需求

| 項目 | 說明 |
|------|------|
| Visual Studio 2022 | 需勾選「使用 C++ 的桌面開發」工作負載（含 MSVC v143、MASM） |
| VSCode 擴充 | **C/C++**（`ms-vscode.cpptools`）；CMake Tools 可選 |
| OpenSSL | `C:\Program Files\OpenSSL-Win64` 或 vcpkg；CMake 會自動尋找 |

> Windows 請用 `cppvsdbg`，不要用 `cppdbg`——後者需要 gdb／lldb。

## 建置

```powershell
cmake -B trunk/ide/srs_msvc/build -S trunk/ide/srs_msvc -G "Visual Studio 17 2022" -A x64
cmake --build trunk/ide/srs_msvc/build --config Debug
```

輸出目錄 `trunk/ide/srs_msvc/build/Debug` 應包含：

| 檔案 | 用途 |
|------|------|
| `srs.exe` | 主程式 |
| `srs.pdb` | 符號檔（中斷點與呼叫堆疊） |
| `test.conf` | 建置時複製的測試設定 |
| `objs/` | 執行時寫入 pid／log，必須可寫 |
| `libcrypto-*.dll`、`libssl-*.dll` | OpenSSL 執行檔，建置時自動複製 |

## `.vscode/launch.json`

```json
{
  "version": "0.2.0",
  "configurations": [
    {
      "name": "SRS (Debug)",
      "type": "cppvsdbg",
      "request": "launch",
      "program": "${workspaceFolder}/trunk/ide/srs_msvc/build/Debug/srs.exe",
      "args": ["-c", "${workspaceFolder}/trunk/ide/srs_msvc/build/Debug/test.conf"],
      "cwd": "${workspaceFolder}/trunk/ide/srs_msvc/build/Debug",
      "console": "integratedTerminal",
      "stopAtEntry": false,
      "preLaunchTask": "build-srs-debug"
    },
    {
      "name": "SRS (Release)",
      "type": "cppvsdbg",
      "request": "launch",
      "program": "${workspaceFolder}/trunk/ide/srs_msvc/build/Release/srs.exe",
      "args": ["-c", "${workspaceFolder}/trunk/ide/srs_msvc/build/Release/test.conf"],
      "cwd": "${workspaceFolder}/trunk/ide/srs_msvc/build/Release",
      "console": "integratedTerminal",
      "stopAtEntry": false,
      "preLaunchTask": "build-srs-release"
    },
    {
      "name": "Attach to srs.exe",
      "type": "cppvsdbg",
      "request": "attach",
      "processId": "${command:pickProcess}"
    }
  ]
}
```

## `.vscode/tasks.json`

```json
{
  "version": "2.0.0",
  "tasks": [
    {
      "label": "build-srs-debug",
      "type": "shell",
      "command": "cmake",
      "args": ["--build", "trunk/ide/srs_msvc/build", "--config", "Debug"],
      "problemMatcher": ["$msCompile"],
      "group": { "kind": "build", "isDefault": true }
    },
    {
      "label": "build-srs-release",
      "type": "shell",
      "command": "cmake",
      "args": ["--build", "trunk/ide/srs_msvc/build", "--config", "Release"],
      "problemMatcher": ["$msCompile"]
    }
  ]
}
```

## 偵錯生產版 `.exe`

生產版用 Release：

```powershell
cmake --build trunk/ide/srs_msvc/build --config Release
```

- `launch.json` 的 `program` 指到 `build/Release/srs.exe`，`srs.pdb` 必須在同目錄。
- Release 會最佳化，部分區域變數會被省略；要逐行追蹤請用 Debug。
- Release 使用一般 CRT，可在沒有安裝 Visual Studio 的機器執行；Debug 需要 `*D.dll`（`MSVCP140D`、`VCRUNTIME140D`、`ucrtbased`），這些**不可轉散布**，只適合開發機。

## 建議中斷點

| 位置 | 用途 |
|------|------|
| `SrsTcpListener::do_cycle` | 觀察 accept 是否被呼叫、回傳哪個 fd |
| `SrsServer::do_cycle` | 主迴圈與關閉旗標 |
| `SrsSignalManager::start` | Windows 主控台事件是否安裝成功 |
| `SrsHttpApi` 的請求處理 | API 逾時、解析問題 |

條件中斷點很好用，例如在 `SrsTcpListener::do_cycle` 設 `srs_netfd_fileno(lfd_) == <port fd>`，避免每個 listener 都停。

## 常見錯誤

| 症狀 | 原因 | 處理 |
|------|------|------|
| `0xc0000135`／找不到 `libcrypto-3-x64.dll` | exe 旁沒有 OpenSSL DLL | 重新建置（POST_BUILD 會複製）；或把 OpenSSL `bin` 加進 PATH |
| `code=1034 PidFileAcquire` | `objs/` 不存在或殘留 `srs.pid` | 建立 `objs/`、刪除 `objs/srs.pid` |
| API／RTMP 連線逾時 | 埠被占用或程式沒 accept | 改 `test.conf` 的埠；確認 `Get-NetTCPConnection -LocalPort <port>` |
| 啟動即 `0xC0000005` | 舊版 ST 堆疊指標／閒置執行緒堆疊 bug | 更新到含 `3748df104` 之後的版本 |

## 相關文件

- [建置與執行說明](建置與執行說明.md)
- [Windows 移植技術文件](Windows移植技術文件.md)
- [已知限制與待辦事項](已知限制與待辦事項.md)
