# SRS Windows 移植技術文件

## 1. 建置系統

### 1.1 CMake 建置架構

```
trunk/ide/srs_msvc/CMakeLists.txt   ← 主建置檔 (407 行)
trunk/3rdparty/st-srs/CMakeLists.txt ← st-srs 子建置 (61 行)
```

**主要特性：**
- **Generator**: Visual Studio 17 2022, x64
- **強制引入**: `/FI"force_include.h"` — 每個 translation unit 都先載入 POSIX 相容層
- **ASM_MASM**: x64 組合語言支援 (`ml64.exe`)
- **自動生成標頭**: `srs_auto_headers.hpp` 位於 `trunk/objs/`，由 CMake 產生，模擬 Linux `./configure` 結果

### 1.2 外部相依（vcpkg）

| 套件 | vcpkg 名稱 | 用途 |
|------|-----------|------|
| OpenSSL | `openssl` | SSL/TLS/DTLS (3.x) |
| libSRTP | `srtp` | WebRTC SRTP 加解密 |
| Opus | `opus` | WebRTC 音訊編解碼 |
| SRT | `srt` | SRT 協議傳輸 |

所有相依皆為**選用**，未找到時功能會自動降級（graceful degradation）。

### 1.3 FFmpeg Stub 機制

由於 FFmpeg 在 Windows 上的連結較為複雜，實作了一層 stub：
- **檔案**: `trunk/ide/srs_msvc/ffmpeg_stubs/ffmpeg_stubs.c` (262 行)
- **功能**: 提供 20+ 個 FFmpeg API 的空實作（`avcodec_open2()` 回傳 -1）
- **效果**: 二進位檔可正常連結，但內部轉碼功能不可用
- **外部轉碼**: SRS 可 spawn `ffmpeg.exe` 作為外部行程來處理轉碼

### 1.4 CMakePresets.json

提供四種預設組態：
- `default` — VS Debug 組態
- `release` — Release 組態
- `ninja` — Ninja 快速建置
- `vcpkg` — 使用 vcpkg toolchain

## 2. POSIX 相容層 (`force_include.h`)

**路徑**: `trunk/ide/srs_msvc/force_include.h` (547 行)

這是 Windows 移植最重要的單一檔案，強制引入每個編譯單元，提供：

### 2.1 Windows 標頭管理
```c
// 順序重要: winsock2.h 必須在 windows.h 之前
#include <winsock2.h>
#include <ws2tcpip.h>
#include <mswsock.h>
#include <windows.h>
```

### 2.2 型別定義
| POSIX 型別 | Windows 對應 |
|---|---|
| `ssize_t` | `__int64` (x64) / `int` (x86) |
| `socklen_t` | `int` |
| `mode_t` | `unsigned short` |
| `pid_t` | `int` |
| `uid_t` / `gid_t` | `int` |
| `iovec` | `WSABUF` 相容結構 |
| `off_t` | `__int64` |
| `clockid_t` | `int` |
| `sigset_t` | `int` |
| `sighandler_t` | Function pointer |

### 2.3 POSIX 函式對應

| 原始函式 | Windows 對應 |
|---|---|
| `strcasecmp` | `_stricmp` |
| `strncasecmp` | `_strnicmp` |
| `chdir` | `_chdir` |
| `getcwd` | `_getcwd` |
| `putenv` | `_putenv` |
| `unlink` | `_unlink` |
| `access` | `_access` |
| `getpid` | `_getpid` |
| `isatty` | `_isatty` |
| `sleep` | `Sleep` (毫秒) |
| `usleep` | `Sleep` (微秒換算) |

### 2.4 Stub（回傳 ENOSYS 或 no-op）

- `fork()` — 回傳 -1, errno = ENOSYS
- `kill()` — 回傳 -1, errno = ENOSYS
- `getppid()` — 回傳 -1
- `closefrom()` — no-op
- `sigaction()` — 回傳 0 (no-op)
- `inotify_init1()` — 回傳 -1, errno = ENOSYS
- `inotify_add_watch()` — 回傳 -1
- `epoll_create()` / `epoll_ctl()` / `epoll_wait()` — 回傳 -1
- `waitpid()` — 回傳 -1
- `mmap()` / `munmap()` — 回傳 MAP_FAILED

### 2.5 Socket API 包裝
- `getsockopt` / `setsockopt` — char\* vs void\* 轉換
- `srs_fcntl()` — 使用 `ioctlsocket` 模擬
- `srs_pipe()` — 使用 `CreatePipe` 實作
- `SIGPIPE` — 定義為 0 (忽略)

## 3. Windows 相容層 (`srs_win32_compat.h`)

**路徑**: `trunk/ide/srs_msvc/srs_win32_compat.h` (264 行)

提供檔案與 socket 的底層包裝：
- `srs_read_sock()` / `srs_write_sock()` / `srs_close_sock()` — Winsock 包裝
- `srs_read_file()` / `srs_write_file()` / `srs_open()` — CRT 檔案 I/O
- `srs_getpagesize()` — `GetSystemInfo`
- `srs_clock_gettime_monotonic()` — `QueryPerformanceFrequency/Counter`
- `srs_gettimeofday()` — `GetSystemTimeAsFileTime`
- `srs_socketpair()` — TCP loopback 模擬
- `srs_set_nonblock()` — `ioctlsocket(FIONBIO)`
- `srs_set_cloexec()` — 無操作 (Windows 無 fork，不需 CLOEXEC)

## 4. POSIX 偽標頭 (compat/)

**路徑**: `trunk/ide/srs_msvc/compat/`

共 20+ 個偽標頭，模擬 POSIX 系統標頭：

| 標頭 | 實作方式 |
|---|---|
| `sys/socket.h` | `msghdr`, `sockaddr_un`, AF_UNIX → AF_INET |
| `sys/epoll.h` | 所有函式回傳 ENOSYS |
| `sys/inotify.h` | 所有函式回傳 ENOSYS |
| `sys/mman.h` | 所有函式回傳 ENOSYS |
| `sys/wait.h` | `waitpid` 回傳 -1 |
| `sys/uio.h` | `iovec` 結構定義 |
| `sys/time.h` | `timeval` 結構 |
| `sys/resource.h` | `getrlimit`/`setrlimit` 回傳 0 (no-op) |
| `sys/utsname.h` | `uname` 回傳固定值 |
| `sys/sysctl.h` | `sysctl` 回傳 -1 |
| `unistd.h` | 完整 POSIX 函式模擬 (gethostname, usleep, isatty 等) |
| `netdb.h` | `gai_strerror`, `AI_NUMERICSERV`, `h_errno` |
| `arpa/inet.h` | `inet_ntop`/`inet_pton` via WSA API |
| `netinet/in.h` | 標準 Winsock 相容定義 |
| `netinet/tcp.h` | `TCP_NODELAY` 等常數 |
| `net/if.h` | 介面常數 |
| `dlfcn.h` | `dlopen`/`dlsym` 回傳 NULL |
| `execinfo.h` | `backtrace` 回傳 0 |
| `ifaddrs.h` | `getifaddrs`/`freeifaddrs` stub |
| `openssl/compat.h` | OpenSSL 1.1 → 3.x API 映射 |
| `sanitizer/asan_interface.h` | ASAN callback stub |

## 5. st-srs (State Threads) Windows 移植

### 5.1 概述

State Threads 是 SRS 的協程程式庫，原設計僅支援 POSIX 系統。
本移植在三個層面上修改：

1. **`md_win64.asm`** — x64 原生上下文切換（組合語言）
2. **`md.h`** — 平台巨集定義
3. **`public.h`** — Windows 型別與結構定義
4. **`stk.c`** — 使用 `VirtualAlloc` 替代 `mmap`
5. **`io.c` / `sched.c` / `sync.c` / `event.c`** — Winsock API 適配

### 5.2 上下文切換 ASM (`md_win64.asm`)

**路徑**: `trunk/3rdparty/st-srs/md_win64.asm` (67 行)

- **平台**: x64 Windows MSVC (ml64.exe)
- **函式**: `_st_md_cxt_save` / `_st_md_cxt_restore`
- **呼叫慣例**: Microsoft x64 (rcx, rdx, r8, r9 → stack)
- **保留暫存器**: RBX, RBP, R12-R15 (non-volatile)
- **jmp_buf 佈局**: 8 個 entry (JB_RBX=0, JB_RBP=1, JB_R12=2, JB_R13=3, JB_R14=4, JB_R15=5, JB_RSP=6, JB_PC=7)
- **`_st_md_cxt_save`**: 儲存 non-volatile 暫存器、堆疊指標（跳過 return address）、程式計數器；回傳 0
- **`_st_md_cxt_restore`**: 回存暫存器、確保回傳值非零（val 或 1）、跳躍至 saved PC

### 5.3 事件系統

Windows 使用 **`select()`** 作為 I/O 多工機制，而非 Linux 的 epoll：
- `#define MD_HAVE_SELECT` — 啟用 select
- `#define FD_SETSIZE 8192` — 較大的檔案描述元上限
- `#undef MD_HAVE_EPOLL` — 不使用 epoll（stub）

### 5.4 記憶體管理

- `VirtualAlloc` / `VirtualFree` 替代 `mmap` / `munmap`
- `#define MD_NO_PROTECT` — 停用記憶體保護（非必要）

### 5.5 執行緒區域儲存 (TLS)

- 使用 `TlsGetValue` / `TlsSetValue` 替代 `pthread_getspecific` / `pthread_setspecific`

## 6. 原始碼修改摘要

### 6.1 `srs_core_platform.hpp` (core)

新增 `#ifndef _WIN32` / `#else` 分支：
- 引入 `winsock2.h`, `ws2tcpip.h`, `windows.h`（修正順序相依性）
- 定義 `ssize_t`, `strcasecmp`/`strncasecmp`, `socklen_t`

### 6.2 `srs_core.hpp` (core)

新增 `_M_AMD64`, `_M_X64`, `_M_IX86`, `_M_ARM64` 到 CPU 架構檢查巨集。

### 6.3 `srs_kernel_error.hpp` (kernel)

處理 Windows 巨集衝突：
- `#pragma push_macro("ERROR")` / `#undef ERROR` — 防止 `wingdi.h` 的 `#define ERROR 0` 破壞所有 `ERROR_*` token
- `#pragma push_macro("ERROR_NOT_SUPPORTED")` — 防止 `winerror.h` 的 `#define ERROR_NOT_SUPPORTED 50L`

### 6.4 `srs_kernel_file.cpp` (kernel)

- Windows 使用 `::_write` / `::_read` 替代 `::write` / `::read`
- 新增 `O_BINARY` flag 在 `SrsFileReader::open()`

### 6.5 `srs_kernel_utility.cpp` (kernel)

- `srs_net_address_any()`: 使用 `closesocket()` 替代 `close()`（Windows 分支）

### 6.6 `srs_main_server.cpp` (main)

- `run_directly_or_daemon()`: `#ifndef _WIN32` / `#else` 守衛
- daemon 模式（double-fork）在 Windows 上不可用，改為 foreground 模式 + 警告
- `#pragma message` 替代 `#warning`（MSVC 相容）

### 6.7 `srs_protocol_st.cpp` (protocol)

- 引入 `srs_close_socket(fd)` 巨集：Windows 使用 `closesocket()`，Linux 使用 `::close()`
- 部分 POSIX 標頭以 Winsock 標頭替代

### 6.8 `srs_app_utility.cpp` (app)

- `get_proc_system_stat()`: Windows 實作使用 `GetSystemTimes()`
- `get_proc_self_stat()`: Windows 實作使用 `GetProcessTimes()` / `FILETIME`

### 6.9 `srs_protocol_rtmp_handshake.cpp` (protocol)

- Windows 相容的 HMAC/SHA256 處理
- DH (Diffie-Hellman) 金鑰交換的 Windows 適配

### 6.10 `srs_protocol_rtc_stun.cpp` (protocol)

- Winsock 相容的 STUN 訊息收發
- UDP socket 操作的 Windows 適配

## 7. 建置輸出結構

```
trunk/ide/srs_msvc/build/
├── Debug/
│   ├── srs.exe           ← 最終二進位 (7+ MB)
│   ├── srs.pdb           ← 偵錯符號
│   ├── srs.lib           ← 匯入程式庫
│   ├── live.conf         ← 執行時期設定 (複製 test.conf)
│   ├── test.conf
│   ├── test2.conf
│   ├── srtp2.dll         ← 外部 DLL
│   ├── srt.dll
│   ├── libssl-3-x64.dll
│   ├── libcrypto-3-x64.dll
│   └── test_open.dll
├── srs.sln               ← Visual Studio 方案
├── st-srs/               ← st-srs 子建置
├── x64/Debug/            ← ZERO_CHECK / ALL_BUILD 暫存
└── *.log                 ← 建置/執行日誌
```
