# CMake build system for SRS on Windows (MSVC).
# Usage:
#   cmake -B build -S ide/srs_msvc
#   cmake --build build
#
# For dependencies (OpenSSL, SRTP, Opus, FFmpeg, SRT),
# install them via vcpkg and set -DCMAKE_TOOLCHAIN_FILE.
#
cmake_minimum_required(VERSION 3.15)
project(srs VERSION 7.0.0 LANGUAGES CXX C ASM_MASM)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_C_STANDARD 99)

# ============================================================
# Platform detection
# ============================================================
if(WIN32)
    message(STATUS "Building SRS for Windows (MSVC)")
    add_definitions(-DWIN32_LEAN_AND_MEAN -D_CRT_SECURE_NO_WARNINGS -D_CRT_NONSTDC_NO_DEPRECATE)
    add_definitions(-DFD_SETSIZE=8192)
    # Disable debug iterator checking to avoid "vector iterators incompatible" asserts.
    # This is a workaround for third-party lib compatibility with Debug CRT.
    set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} /D_ITERATOR_DEBUG_LEVEL=0")
elseif(APPLE)
    message(STATUS "Building SRS for macOS")
else()
    message(STATUS "Building SRS for Linux")
endif()

# ============================================================
# Source directories relative to trunk/
# ============================================================
get_filename_component(SRS_TRUNK "${CMAKE_CURRENT_SOURCE_DIR}/../.." ABSOLUTE)
message(STATUS "SRS trunk: ${SRS_TRUNK}")

# ============================================================
# Build ST library (State Threads) from source
# ============================================================
add_subdirectory(${SRS_TRUNK}/3rdparty/st-srs ${CMAKE_BINARY_DIR}/st-srs)

# ============================================================
# Find external dependencies (install via vcpkg or provide manually)
# ============================================================
# For now, we use find_package with REQUIRED components.
# Install via: vcpkg install openssl libsrtp opus ffmpeg srt --triplet x64-windows
find_package(PkgConfig QUIET)

# OpenSSL
find_package(OpenSSL QUIET)
if(NOT OpenSSL_FOUND)
    message(STATUS "OpenSSL not found - SSL features will be disabled. Install via: vcpkg install openssl")
endif()

# SRTP (libsrtp2) - try config mode first (vcpkg), then pkg-config
find_package(libSRTP CONFIG QUIET)
if(libSRTP_FOUND)
    message(STATUS "Found libSRTP via CMake config")
    set(SRTP_FOUND TRUE)
    set(SRTP_INCLUDE_DIRS "")
    set(SRTP_LIBRARIES libSRTP::srtp2)
elseif(PkgConfig_FOUND)
    pkg_check_modules(SRTP QUIET libsrtp2)
    if(SRTP_FOUND)
        message(STATUS "Found libSRTP via pkg-config")
    endif()
endif()
if(NOT SRTP_FOUND)
    message(STATUS "SRTP not found - RTC features limited")
endif()

# Opus
find_package(Opus CONFIG QUIET)
if(NOT Opus_FOUND)
    if(PkgConfig_FOUND)
        pkg_check_modules(OPUS QUIET opus)
    endif()
    if(NOT OPUS_FOUND)
        message(STATUS "Opus not found - RTC audio limited")
    endif()
else()
    set(OPUS_FOUND TRUE)
    # Opus::opus target auto-adds include dirs and libs when linked
    message(STATUS "Found Opus: ${Opus_DIR}")
endif()

# SRT - vcpkg installs headers at include/srt/srt.h and lib/srt.lib
# No CMake config provided, check via find_path/find_library
find_path(SRT_INCLUDE_DIRS NAMES srt/srt.h)
find_library(SRT_LIBRARIES NAMES srt)
if(SRT_INCLUDE_DIRS AND SRT_LIBRARIES)
    set(SRT_FOUND TRUE)
    message(STATUS "Found SRT: ${SRT_LIBRARIES}")
else()
    set(SRT_FOUND FALSE)
    message(STATUS "SRT not found - SRT protocol disabled")
endif()

# ============================================================
# SRS source files
# ============================================================
set(SRS_CORE_SOURCES
    ${SRS_TRUNK}/src/core/srs_core.cpp
    ${SRS_TRUNK}/src/core/srs_core_autofree.cpp
    ${SRS_TRUNK}/src/core/srs_core_deprecated.cpp
    ${SRS_TRUNK}/src/core/srs_core_performance.cpp
    ${SRS_TRUNK}/src/core/srs_core_platform.cpp
    ${SRS_TRUNK}/src/core/srs_core_time.cpp
    ${SRS_TRUNK}/src/core/srs_core_version.cpp
    ${SRS_TRUNK}/src/core/srs_core_version3.cpp
    ${SRS_TRUNK}/src/core/srs_core_version4.cpp
    ${SRS_TRUNK}/src/core/srs_core_version5.cpp
    ${SRS_TRUNK}/src/core/srs_core_version6.cpp
    ${SRS_TRUNK}/src/core/srs_core_version7.cpp
)

set(SRS_KERNEL_SOURCES
    ${SRS_TRUNK}/src/kernel/srs_kernel_aac.cpp
    ${SRS_TRUNK}/src/kernel/srs_kernel_balance.cpp
    ${SRS_TRUNK}/src/kernel/srs_kernel_buffer.cpp
    ${SRS_TRUNK}/src/kernel/srs_kernel_codec.cpp
    ${SRS_TRUNK}/src/kernel/srs_kernel_consts.cpp
    ${SRS_TRUNK}/src/kernel/srs_kernel_error.cpp
    ${SRS_TRUNK}/src/kernel/srs_kernel_factory.cpp
    ${SRS_TRUNK}/src/kernel/srs_kernel_file.cpp
    ${SRS_TRUNK}/src/kernel/srs_kernel_flv.cpp
    ${SRS_TRUNK}/src/kernel/srs_kernel_hourglass.cpp
    ${SRS_TRUNK}/src/kernel/srs_kernel_io.cpp
    ${SRS_TRUNK}/src/kernel/srs_kernel_kbps.cpp
    ${SRS_TRUNK}/src/kernel/srs_kernel_log.cpp
    ${SRS_TRUNK}/src/kernel/srs_kernel_mp3.cpp
    ${SRS_TRUNK}/src/kernel/srs_kernel_mp4.cpp
    ${SRS_TRUNK}/src/kernel/srs_kernel_packet.cpp
    ${SRS_TRUNK}/src/kernel/srs_kernel_pithy_print.cpp
    ${SRS_TRUNK}/src/kernel/srs_kernel_ps.cpp
    ${SRS_TRUNK}/src/kernel/srs_kernel_resource.cpp
    ${SRS_TRUNK}/src/kernel/srs_kernel_rtc_queue.cpp
    ${SRS_TRUNK}/src/kernel/srs_kernel_rtc_rtcp.cpp
    ${SRS_TRUNK}/src/kernel/srs_kernel_rtc_rtp.cpp
    ${SRS_TRUNK}/src/kernel/srs_kernel_st.cpp
    ${SRS_TRUNK}/src/kernel/srs_kernel_stream.cpp
    ${SRS_TRUNK}/src/kernel/srs_kernel_ts.cpp
    ${SRS_TRUNK}/src/kernel/srs_kernel_utility.cpp
)

set(SRS_PROTOCOL_SOURCES
    ${SRS_TRUNK}/src/protocol/srs_protocol_amf0.cpp
    ${SRS_TRUNK}/src/protocol/srs_protocol_conn.cpp
    ${SRS_TRUNK}/src/protocol/srs_protocol_format.cpp
    ${SRS_TRUNK}/src/protocol/srs_protocol_http_client.cpp
    ${SRS_TRUNK}/src/protocol/srs_protocol_http_conn.cpp
    ${SRS_TRUNK}/src/protocol/srs_protocol_http_stack.cpp
    ${SRS_TRUNK}/src/protocol/srs_protocol_http_stack_llhttp.cpp
    ${SRS_TRUNK}/src/protocol/srs_protocol_http_stack_llhttpadapter.cpp
    ${SRS_TRUNK}/src/protocol/srs_protocol_http_stack_llhttpapi.cpp
    ${SRS_TRUNK}/src/protocol/srs_protocol_http_stack_llhttphttp.cpp
    ${SRS_TRUNK}/src/protocol/srs_protocol_io.cpp
    ${SRS_TRUNK}/src/protocol/srs_protocol_json.cpp
    ${SRS_TRUNK}/src/protocol/srs_protocol_log.cpp
    ${SRS_TRUNK}/src/protocol/srs_protocol_protobuf.cpp
    ${SRS_TRUNK}/src/protocol/srs_protocol_raw_avc.cpp
    ${SRS_TRUNK}/src/protocol/srs_protocol_rtc_stun.cpp
    ${SRS_TRUNK}/src/protocol/srs_protocol_rtmp_conn.cpp
    ${SRS_TRUNK}/src/protocol/srs_protocol_rtmp_handshake.cpp
    ${SRS_TRUNK}/src/protocol/srs_protocol_rtmp_msg_array.cpp
    ${SRS_TRUNK}/src/protocol/srs_protocol_rtmp_stack.cpp
    ${SRS_TRUNK}/src/protocol/srs_protocol_rtp.cpp
    ${SRS_TRUNK}/src/protocol/srs_protocol_rtsp_stack.cpp
    ${SRS_TRUNK}/src/protocol/srs_protocol_sdp.cpp
    ${SRS_TRUNK}/src/protocol/srs_protocol_st.cpp
    ${SRS_TRUNK}/src/protocol/srs_protocol_stream.cpp
    ${SRS_TRUNK}/src/protocol/srs_protocol_utility.cpp
)

# SRT protocol requires libsrt
if(SRT_FOUND)
    list(APPEND SRS_PROTOCOL_SOURCES ${SRS_TRUNK}/src/protocol/srs_protocol_srt.cpp)
endif()

set(SRS_APP_SOURCES
    ${SRS_TRUNK}/src/app/srs_app_async_call.cpp
    ${SRS_TRUNK}/src/app/srs_app_caster_flv.cpp
    ${SRS_TRUNK}/src/app/srs_app_circuit_breaker.cpp
    ${SRS_TRUNK}/src/app/srs_app_config.cpp
    ${SRS_TRUNK}/src/app/srs_app_coworkers.cpp
    ${SRS_TRUNK}/src/app/srs_app_dash.cpp
    ${SRS_TRUNK}/src/app/srs_app_dvr.cpp
    ${SRS_TRUNK}/src/app/srs_app_edge.cpp
    ${SRS_TRUNK}/src/app/srs_app_encoder.cpp
    ${SRS_TRUNK}/src/app/srs_app_factory.cpp
    ${SRS_TRUNK}/src/app/srs_app_ffmpeg.cpp
    ${SRS_TRUNK}/src/app/srs_app_forward.cpp
    ${SRS_TRUNK}/src/app/srs_app_fragment.cpp
    ${SRS_TRUNK}/src/app/srs_app_gb28181.cpp
    ${SRS_TRUNK}/src/app/srs_app_hds.cpp
    ${SRS_TRUNK}/src/app/srs_app_heartbeat.cpp
    ${SRS_TRUNK}/src/app/srs_app_hls.cpp
    ${SRS_TRUNK}/src/app/srs_app_http_api.cpp
    ${SRS_TRUNK}/src/app/srs_app_http_client.cpp
    ${SRS_TRUNK}/src/app/srs_app_http_conn.cpp
    ${SRS_TRUNK}/src/app/srs_app_http_hooks.cpp
    ${SRS_TRUNK}/src/app/srs_app_http_static.cpp
    ${SRS_TRUNK}/src/app/srs_app_http_stream.cpp
    ${SRS_TRUNK}/src/app/srs_app_ingest.cpp
    ${SRS_TRUNK}/src/app/srs_app_latest_version.cpp
    ${SRS_TRUNK}/src/app/srs_app_listener.cpp
    ${SRS_TRUNK}/src/app/srs_app_log.cpp
    ${SRS_TRUNK}/src/app/srs_app_mpegts_udp.cpp
    ${SRS_TRUNK}/src/app/srs_app_ng_exec.cpp
    ${SRS_TRUNK}/src/app/srs_app_process.cpp
    ${SRS_TRUNK}/src/app/srs_app_recv_thread.cpp
    ${SRS_TRUNK}/src/app/srs_app_refer.cpp
    ${SRS_TRUNK}/src/app/srs_app_reload.cpp
    ${SRS_TRUNK}/src/app/srs_app_rtmp_conn.cpp
    ${SRS_TRUNK}/src/app/srs_app_rtmp_source.cpp
    ${SRS_TRUNK}/src/app/srs_app_rtsp_conn.cpp
    ${SRS_TRUNK}/src/app/srs_app_rtsp_source.cpp
    ${SRS_TRUNK}/src/app/srs_app_security.cpp
    ${SRS_TRUNK}/src/app/srs_app_server.cpp
    ${SRS_TRUNK}/src/app/srs_app_st.cpp
    ${SRS_TRUNK}/src/app/srs_app_statistic.cpp
    ${SRS_TRUNK}/src/app/srs_app_stream_bridge.cpp
    ${SRS_TRUNK}/src/app/srs_app_stream_token.cpp
    ${SRS_TRUNK}/src/app/srs_app_utility.cpp
)

# ============================================================
# Include directories
# ============================================================
# Comapt stubs MUST be first for POSIX headers (sys/socket.h, unistd.h, etc.)
# They shadow non-existent Windows system headers.
# NOTE: compat/srtp2/srtp.h is intentionally NOT included here — the real
# vcpkg-provided srtp2 header takes precedence via find_package below.
include_directories(BEFORE ${CMAKE_CURRENT_SOURCE_DIR}/compat)

# SRS source paths
include_directories(
    ${SRS_TRUNK}/objs                    # For generated srs_auto_headers.hpp
    ${SRS_TRUNK}/src/core
    ${SRS_TRUNK}/src/kernel
    ${SRS_TRUNK}/src/protocol
    ${SRS_TRUNK}/src/app
    ${SRS_TRUNK}/src/service
    ${SRS_TRUNK}/src/main
    ${SRS_TRUNK}/src/utest
    ${SRS_TRUNK}/3rdparty/st-srs         # For st.h
    ${CMAKE_CURRENT_SOURCE_DIR}          # For srs_win32_compat.h
)

# External library includes (vcpkg-provided or system)
if(OpenSSL_FOUND)
    include_directories(${OPENSSL_INCLUDE_DIR})
endif()
if(libSRTP_FOUND)
    # libSRTP::srtp2 target automatically adds include dirs when linked
    # Also add explicitly for sources that use #include <srtp2/srtp.h>
    get_target_property(_srtp_incdirs libSRTP::srtp2 INTERFACE_INCLUDE_DIRECTORIES)
    if(_srtp_incdirs)
        include_directories(${_srtp_incdirs})
    endif()
endif()
if(SRT_FOUND)
    include_directories(${SRT_INCLUDE_DIRS})
endif()

# Bundled stub header fallback (used only when vcpkg packages aren't available)
# ffmpeg-4-fit provides libavcodec, libavutil, etc.
include_directories(${SRS_TRUNK}/3rdparty/ffmpeg-4-fit)
# Generate avconfig.h if missing (normally generated by FFmpeg's ./configure)
set(_avconfig_h ${SRS_TRUNK}/3rdparty/ffmpeg-4-fit/libavutil/avconfig.h)
if(NOT EXISTS ${_avconfig_h})
    file(WRITE ${_avconfig_h}
"/* Auto-generated for MSVC build */
#ifndef AVUTIL_AVCONFIG_H
#define AVUTIL_AVCONFIG_H
#define AV_HAVE_BIGENDIAN 0
#define AV_HAVE_FAST_UNALIGNED 1
#define AV_HAVE_INCOMPATIBLE_LIBAV_ABI 0
#endif
")
    message(STATUS "Generated ${_avconfig_h}")
endif()
# libsrtp-2-fit provides srtp2/srtp.h stub (lower priority than vcpkg)
include_directories(${SRS_TRUNK}/3rdparty/libsrtp-2-fit/include)
# srt-1-fit provides srt/srt.h stub (lower priority than vcpkg)
include_directories(${SRS_TRUNK}/3rdparty/srt-1-fit/headers)

# ============================================================
# Generate srs_auto_headers.hpp for Windows
# ============================================================
set(SRS_AUTO_HEADERS ${SRS_TRUNK}/objs/srs_auto_headers.hpp)

# Generate the auto headers file for Windows build
set(_srs_rtc_enabled FALSE)
set(_srs_srt_enabled FALSE)
if(libSRTP_FOUND)
    set(_srs_rtc_enabled TRUE)
endif()
if(SRT_FOUND)
    set(_srs_srt_enabled TRUE)
endif()

file(WRITE ${SRS_AUTO_HEADERS}
"// Auto generated by configure for Windows MSVC build.
#ifndef SRS_AUTO_HEADERS_HPP
#define SRS_AUTO_HEADERS_HPP

// Build info (emulates auto/auto_headers.sh)
#define SRS_AUTO_BUILD_TS \"${CMAKE_BUILD_TYPE}\"
#define SRS_AUTO_BUILD_TIME \"${CMAKE_TIMESTAMP}\"
#define SRS_AUTO_UNAME \"Windows\"
#define SRS_AUTO_USER_CONFIGURE \"cmake\"
#define SRS_PACKAGER \"\"
#define SRS_BUILD_TS \"0\"
#define SRS_BUILD_DATE \"2026-06-05\"
#define SRS_USER_CONFIGURE \"${SRS_AUTO_USER_CONFIGURE}\"
#define SRS_CONFIGURE \"${SRS_AUTO_CONFIGURE}\"
#define SRS_DEFAULT_CONFIG \"conf/srs.conf\"
#define RTMP_SIG_SRS_AUTHORS \"ossrs\"

// Features
#define SRS_AUTO_SSL
#define SRS_AUTO_SSL_USE
#define SRS_AUTO_GB28181
#define SRS_AUTO_H265
#define SRS_AUTO_H265_USE
#define SRS_RTSP
#define SRS_DASH

// Feature _BOOL variants (for API version info)
#define SRS_OSX_BOOL false
#define SRS_CROSSBUILD_BOOL false
#define SRS_HDS_BOOL false

// Conditional features (enabled only when libraries are found)
"
)
if(_srs_rtc_enabled)
    file(APPEND ${SRS_AUTO_HEADERS}
"#define SRS_AUTO_RTC_USE
#define SRS_AUTO_RTC
#define SRS_RTC_BOOL true
"
    )
else()
    file(APPEND ${SRS_AUTO_HEADERS}
"#define SRS_RTC_BOOL false
"
    )
endif()
if(_srs_srt_enabled)
    file(APPEND ${SRS_AUTO_HEADERS}
"#define SRS_AUTO_SRT
#define SRS_AUTO_SRT_USE
#define SRS_SRT_BOOL true
"
    )
else()
    file(APPEND ${SRS_AUTO_HEADERS}
"#define SRS_SRT_BOOL false
"
    )
endif()
file(APPEND ${SRS_AUTO_HEADERS}
"
// Enable FFMPEG stub for transcoding/ingest (spawns ffmpeg process)
#define SRS_FFMPEG_STUB

// Headers
#define SRS_AUTO_SSL_OPENSSL

// Versions
#define VERSION_MAJOR 7
#define VERSION_MINOR 0
#define VERSION_REVISION 0
#define SRS_AUTO_VERSION_MAJOR 7
#define SRS_AUTO_VERSION_MINOR 0
#define SRS_AUTO_VERSION_REVISION 0

#endif
"
)
message(STATUS "Generated ${SRS_AUTO_HEADERS} (RTC=${_srs_rtc_enabled}, SRT=${_srs_srt_enabled})")

# Add RTC source files (requires libSRTP)
if(_srs_rtc_enabled)
    list(APPEND SRS_APP_SOURCES
        ${SRS_TRUNK}/src/app/srs_app_rtc_api.cpp
        ${SRS_TRUNK}/src/app/srs_app_rtc_codec.cpp
        ${SRS_TRUNK}/src/app/srs_app_rtc_conn.cpp
        ${SRS_TRUNK}/src/app/srs_app_rtc_dtls.cpp
        ${SRS_TRUNK}/src/app/srs_app_rtc_network.cpp
        ${SRS_TRUNK}/src/app/srs_app_rtc_server.cpp
        ${SRS_TRUNK}/src/app/srs_app_rtc_source.cpp
    )
endif()

# Add SRT source files (requires libsrt)
if(_srs_srt_enabled)
    list(APPEND SRS_APP_SOURCES
        ${SRS_TRUNK}/src/app/srs_app_srt_conn.cpp
        ${SRS_TRUNK}/src/app/srs_app_srt_listener.cpp
        ${SRS_TRUNK}/src/app/srs_app_srt_server.cpp
        ${SRS_TRUNK}/src/app/srs_app_srt_source.cpp
    )
endif()

# ============================================================
# Build the SRS server executable (core + kernel + protocol only for now)
# ============================================================
set(SRS_SOURCES
    ${SRS_CORE_SOURCES}
    ${SRS_KERNEL_SOURCES}
    ${SRS_PROTOCOL_SOURCES}
    ${SRS_APP_SOURCES}
    ${SRS_TRUNK}/src/main/srs_main_server.cpp
)

# FFmpeg stub implementations (for linking without real FFmpeg DLLs)
set(SRS_FFMPEG_STUBS
    ${CMAKE_CURRENT_SOURCE_DIR}/ffmpeg_stubs/ffmpeg_stubs.c
)

# Add the main executable with all sources including stubs
add_executable(srs ${SRS_SOURCES} ${SRS_FFMPEG_STUBS})

# Apply force_include.h to all translation units for POSIX compat on Windows
target_compile_options(srs PRIVATE /FI"${CMAKE_CURRENT_SOURCE_DIR}/force_include.h")

# Link ST library
target_link_libraries(srs PRIVATE st-srs)

# Windows-specific libraries and compile flags
if(WIN32)
    target_link_libraries(srs PRIVATE
        ws2_32
        iphlpapi
        userenv
        crypt32
        bcrypt
        ntdll
    )
    target_compile_options(srs PRIVATE /wd4996 /wd4267 /wd4244 /wd4018 /bigobj /utf-8)
endif()

# Link external dependencies if found
if(OpenSSL_FOUND)
    target_link_libraries(srs PRIVATE OpenSSL::SSL OpenSSL::Crypto)
endif()
if(libSRTP_FOUND)
    target_link_libraries(srs PRIVATE libSRTP::srtp2)
endif()
if(SRT_FOUND)
    target_link_libraries(srs PRIVATE ${SRT_LIBRARIES})
endif()

# ============================================================
# Copy config file
# ============================================================
if(WIN32)
    # Copy config file at configure time
    file(COPY ${SRS_TRUNK}/conf/srs.conf DESTINATION ${CMAKE_BINARY_DIR})
    # Copy console web assets for http_server at configure time
    file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/objs/nginx/html)
    file(COPY ${SRS_TRUNK}/research/console/ DESTINATION ${CMAKE_BINARY_DIR}/objs/nginx/html)
endif()
