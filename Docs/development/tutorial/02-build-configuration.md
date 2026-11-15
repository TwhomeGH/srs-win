# Build 配置導讀：CMake 與 Dockerfile

> [!TIP]
> 這一篇是 [Learn 1](01-learn1-build-and-run.md) 的閱讀版：不需要改程式，只要知道「哪個檔案控制哪個平台」。
> 指令與實際檔案為準；若程式碼與本文不一致，先以程式碼為準並更新本文。

## 一張圖

```text
Windows
  trunk/ide/srs_msvc/CMakePresets.json
  trunk/ide/srs_msvc/CMakeLists.txt
        ↓
  build/         windows-default
  build-full/    windows-full（vcpkg RTC/SRT）

Linux
  trunk/configure
  dev/docker/Dockerfile
  dev/docker/docker-compose.yml
        ↓
  srs-win-dev-linux:local
  dev/runtime/docker/

Web console
  ui/vite.config.mjs
  ui/src/
        ↓
  objs/nginx/html/console/
```

## 1. CMakePresets.json

`CMakePresets.json` 只放「怎麼 configure／build／test」，不放原始碼清單。

| Preset | binaryDir | 用途 |
| --- | --- | --- |
| `windows-default` | `trunk/ide/srs_msvc/build` | 預設功能，系統 OpenSSL，RTC／SRT 關閉 |
| `windows-full` | `trunk/ide/srs_msvc/build-full` | vcpkg，RTC／SRT 開啟 |

常見指令：

```powershell
cd trunk/ide/srs_msvc

# 預設
cmake --preset windows-default
cmake --build --preset windows-default-debug --target srs

# 全功能
cmake --preset windows-full
cmake --build --preset windows-full-debug --target srs
```

測試 preset：

```powershell
ctest --preset windows-default-test
ctest --preset windows-full-test
```

`windows-full` 的 toolchain 指向：

```text
${sourceDir}/../../../.vscode/vcpkg/scripts/buildsystems/vcpkg.cmake
```

`.vscode/vcpkg` 是 workspace junction，由 `dev/setup-vscode.ps1` 建立；**不要把磁碟機代號寫死在 preset**。

## 2. CMakeLists.txt

主要區塊依序是：

1. 平台偵測
2. `st-srs` 子專案
3. 外部依賴（OpenSSL／libSRTP／Opus／SRT）
4. `SRS_CORE_SOURCES`、`SRS_KERNEL_SOURCES`、`SRS_PROTOCOL_SOURCES`、`SRS_APP_SOURCES`
5. include path 與 compat 標頭
6. 產生 `srs_auto_headers.hpp`
7. 建立 `srs`、native tests
8. post-build DLL／runtime 複製

### 平台偵測

```cmake
if(WIN32)
    add_definitions(-DWIN32_LEAN_AND_MEAN -D_CRT_SECURE_NO_WARNINGS ...)
elseif(APPLE)
    message(STATUS "Building SRS for macOS")
else()
    message(STATUS "Building SRS for Linux")
endif()
```

Windows 專屬的 `FD_SETSIZE=8192`、`_ITERATOR_DEBUG_LEVEL=0` 也在這裡。

### 依賴找不到時

CMake 的 `find_package` 是「找到就開，找不到就關」：

```cmake
find_package(libSRTP CONFIG QUIET)
if(libSRTP_FOUND)
    set(SRTP_FOUND TRUE)
endif()
```

這代表：

- `windows-default` 沒有 libsrtp → RTC 關閉
- `windows-full` 有 vcpkg → RTC／SRT 開啟
- 不要假設 `windows-default` 有 RTC

### 來源清單

Windows 的來源清單是明確列出的：

```cmake
set(SRS_APP_SOURCES
    ${SRS_TRUNK}/src/app/srs_app_config.cpp
    ${SRS_TRUNK}/src/app/srs_app_console_auth.cpp
    ...
)
```

新增 `src/app/srs_app_foo.cpp` 時，至少要做：

```text
1. 新增檔案
2. CMakeLists.txt 的 SRS_APP_SOURCES
3. trunk/configure 的 MODULE_FILES
4. pwsh dev/check-source-lists.ps1
```

第 4 步會同時比對 `src/app/*.cpp`、CMake 與 `configure`；漏一個就會失敗。

### 產生 feature header

Windows 沒有 upstream 的 `auto/auto_headers.sh`，所以 CMake configure 時直接產生：

```text
trunk/ide/srs_msvc/build/srs_auto_headers.hpp
```

內容包含：

```text
SRS_BUILD_DATE
SRS_BUILD_TS
SRS_AUTO_BUILD_TIME
SRS_RTC
SRS_SRT
```

這是 **configure 時間**，不是 link 時間；重建 configure 才會更新。

### compat include 的順序

```cmake
include_directories(BEFORE ${CMAKE_CURRENT_SOURCE_DIR}/compat)
```

Windows 的 `compat/` 必須排在系統標頭前面，否則 `sys/socket.h`、`unistd.h` 等會被誤判。

### 測試 target

native tests 與 `srs` 分開建置：

```text
st-event-loop-test
path-rename-test
file-share-test
win-compat-test
```

`dev/check.ps1` 會一起建置並跑 ctest；CI 也是同一個入口。

## 3. Dockerfile

`dev/docker/Dockerfile` 是 **Linux upstream 建置**，不是 Windows CMake 的鏡像。

### 基底與套件

```dockerfile
FROM ubuntu:24.04
RUN apt-get update && apt-get install -y \
    build-essential cmake pkg-config \
    libssl-dev libsrtp2-dev libsrt-openssl-dev \
    libopus-dev libavcodec-dev libavformat-dev libavutil-dev \
    libswscale-dev libswresample-dev \
    unzip automake libtool patch git tcl ccache
```

它刻意使用系統套件，而不是 vendored 的舊版 OpenSSL／FFmpeg／SRT／libsrtp。

### BuildKit cache

```dockerfile
RUN --mount=type=cache,target=/var/cache/apt,sharing=locked \
    --mount=type=cache,target=/var/lib/apt/lists,sharing=locked \
    apt-get update && apt-get install ...
```

- apt 套件只下載一次，之後由 cache 重用。
- `/root/.ccache` 也是 cache mount；原始碼變更時只重編受影響的檔案。

### 原始碼與 configure

```dockerfile
WORKDIR /srs
COPY trunk /srs/trunk
WORKDIR /srs/trunk
RUN ./configure \
      --use-sys-srtp=on \
      --use-sys-srt=on \
      --use-sys-ffmpeg=on \
    && make -j"$(nproc)"
```

這裡走的是 `trunk/configure`，不是 CMake。新增 app source 若只加到 CMake，Docker 會在連結階段失敗。

### 為什麼 context 很小

`.dockerignore` 排除：

```text
trunk/ide
trunk/build
trunk/objs
dev/runtime
ui/node_modules
_site
```

`trunk/ide` 是 Windows CMake 與本機 build 產物，Linux lane 不需要；若沒排除，每次 Windows 建置都會讓 Docker 的 `COPY trunk` cache 失效。

### compose 與 local-deploy

`dev/docker/docker-compose.yml` 負責：

```text
host 11935 → container 1935   RTMP
host 11985 → container 1985   HTTP API
host 18088 → container 8088   HTTP server
host 11980 → container 10080  SRT
host 18000 → container 8000   RTC
```

`dev/local-deploy.ps1` 負責：

```text
build image
→ 寫入 revision/version label
→ docker compose up -d --force-recreate
→ -Test 打 HTTP 與 RTMP
```

它不負責 Windows build，也不修改 CMake preset。

## 4. 改動從哪裡動手

| 想改什麼 | 先看哪個檔案 |
| --- | --- |
| Windows 建置選項、來源、target | `trunk/ide/srs_msvc/CMakeLists.txt` |
| Windows preset／binaryDir | `trunk/ide/srs_msvc/CMakePresets.json` |
| Linux configure 來源清單 | `trunk/configure` |
| Linux Docker 套件／configure 參數 | `dev/docker/Dockerfile` |
| Docker 埠號、mount、服務名 | `dev/docker/docker-compose.yml` |
| dev 容器啟動設定 | `dev/docker/dev.conf` |
| Windows dev 啟動設定 | `dev/windows/dev.conf` |
| 新增 C++ app 功能 | `trunk/src/app/` ＋ 兩份來源清單 |
| 新增 Web console 頁面 | `ui/src/` ＋ `ui/vite.config.mjs` |
| CI 觸發條件 | `.github/workflows/` ＋ `Docs/CI策略.md` |

## 5. 不要做的事

```text
不要把 Windows build 產物放進 Docker context
不要只改 CMake 而漏掉 trunk/configure
不要只改 configure 而漏掉 CMake
不要用 --no-cache 當成日常建置方式
不要把 macOS／ARM64 的 #ifdef 直接塞進 app 邏輯
不要在沒有 capability test 的情況下宣稱平台支援
```

## 6. 下一步

讀完後回到 [Learn 1](01-learn1-build-and-run.md) 的檢查點，確認：

- Windows default 建置、啟動、API
- Docker Linux lane 建置、smoke
- source list 故意失敗與清理

下一個功能教學會在這個基礎上，從「新增一個唯讀 API 欄位」開始。
