#ifndef SRS_FEATURE_FLAGS_HPP
#define SRS_FEATURE_FLAGS_HPP

// Canonical feature flags for every build lane.
//
// Linux configure and Windows CMake detect features with different tools, but
// the source tree must see the same macro spellings and the same boolean
// aliases. Keep those aliases here instead of duplicating them in
// auto_headers.sh and CMakeLists.txt.
//
// Inputs may use either spelling:
//   - SRS_*        from upstream configure
//   - SRS_AUTO_*   from the MSVC CMake build
// Outputs are normalized:
//   - SRS_AUTO_<feature> and SRS_AUTO_<feature>_USE where applicable
//   - SRS_<feature> for older source paths
//   - SRS_<feature>_BOOL for API feature reporting

#if defined(SRS_AUTO_RTC) || defined(SRS_RTC) || defined(SRS_AUTO_RTC_USE)
    #ifndef SRS_AUTO_RTC
        #define SRS_AUTO_RTC
    #endif
    #ifndef SRS_AUTO_RTC_USE
        #define SRS_AUTO_RTC_USE
    #endif
    #ifndef SRS_RTC_BOOL
        #define SRS_RTC_BOOL true
    #endif
#else
    #ifndef SRS_RTC_BOOL
        #define SRS_RTC_BOOL false
    #endif
#endif

#if defined(SRS_AUTO_SRT) || defined(SRS_SRT) || defined(SRS_AUTO_SRT_USE)
    #ifndef SRS_AUTO_SRT
        #define SRS_AUTO_SRT
    #endif
    #ifndef SRS_AUTO_SRT_USE
        #define SRS_AUTO_SRT_USE
    #endif
    #ifndef SRS_SRT_BOOL
        #define SRS_SRT_BOOL true
    #endif
#else
    #ifndef SRS_SRT_BOOL
        #define SRS_SRT_BOOL false
    #endif
#endif

#if defined(SRS_AUTO_SSL) || defined(SRS_SSL)
    #ifndef SRS_AUTO_SSL
        #define SRS_AUTO_SSL
    #endif
    #ifndef SRS_AUTO_SSL_USE
        #define SRS_AUTO_SSL_USE
    #endif
#endif

#if defined(SRS_AUTO_H265) || defined(SRS_H265)
    #ifndef SRS_AUTO_H265
        #define SRS_AUTO_H265
    #endif
    #ifndef SRS_AUTO_H265_USE
        #define SRS_AUTO_H265_USE
    #endif
#endif

#if defined(SRS_GB28181) || defined(SRS_AUTO_GB28181)
    #ifndef SRS_AUTO_GB28181
        #define SRS_AUTO_GB28181
    #endif
    #ifndef SRS_GB28181
        #define SRS_GB28181
    #endif
#endif

#if defined(SRS_HDS) || (defined(SRS_HDS_BOOL) && SRS_HDS_BOOL)
    #ifndef SRS_HDS_BOOL
        #define SRS_HDS_BOOL true
    #endif
#else
    #ifndef SRS_HDS_BOOL
        #define SRS_HDS_BOOL false
    #endif
#endif

// Catch a generator that emits the canonical feature but forgets the alias
// used by the source tree. These are compile-time guards, not runtime checks.
#if defined(SRS_AUTO_RTC) && !defined(SRS_AUTO_RTC_USE)
    #error "SRS_AUTO_RTC requires SRS_AUTO_RTC_USE"
#endif
#if defined(SRS_AUTO_SRT) && !defined(SRS_AUTO_SRT_USE)
    #error "SRS_AUTO_SRT requires SRS_AUTO_SRT_USE"
#endif
#if defined(SRS_AUTO_H265) && !defined(SRS_AUTO_H265_USE)
    #error "SRS_AUTO_H265 requires SRS_AUTO_H265_USE"
#endif
#if defined(SRS_AUTO_GB28181) && !defined(SRS_GB28181)
    #error "SRS_AUTO_GB28181 requires SRS_GB28181"
#endif

#endif
