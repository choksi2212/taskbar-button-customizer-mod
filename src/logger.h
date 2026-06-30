// =============================================================================
// logger.h — Taskbar Button Customizer
// Thin, zero-overhead wrapper around Windhawk's Wh_Log API.
// Provides severity levels (DEBUG/INFO/WARN/ERROR) with compile-time stripping
// for release builds (define TBC_LOG_LEVEL before including this header).
//
// Usage:
//   LOG_INFO(L"Loaded %d elements", count);
//   LOG_ERROR(L"Failed to find taskbar: HRESULT=0x%08X", hr);
// =============================================================================
#pragma once

#include <windhawk_api.h>
#include <cstdio>

// Log levels (lower = more verbose)
#define TBC_LOG_DEBUG   0
#define TBC_LOG_INFO    1
#define TBC_LOG_WARN    2
#define TBC_LOG_ERROR   3
#define TBC_LOG_SILENT  4

// Default: INFO in release, DEBUG in debug builds
#ifndef TBC_LOG_LEVEL
  #ifdef _DEBUG
    #define TBC_LOG_LEVEL TBC_LOG_DEBUG
  #else
    #define TBC_LOG_LEVEL TBC_LOG_INFO
  #endif
#endif

// ---------------------------------------------------------------------------
// Internal helper — formats to a stack buffer and delegates to Wh_Log.
// Wrapped in a do/while(0) so the macro is safe in all contexts.
// ---------------------------------------------------------------------------
#define TBC_LOG_IMPL(prefix, fmt, ...)                                      \
    do {                                                                    \
        wchar_t _tbc_buf_[1024];                                            \
        _snwprintf_s(_tbc_buf_, _countof(_tbc_buf_), _TRUNCATE,            \
                     prefix L" " fmt, ##__VA_ARGS__);                      \
        Wh_Log(L"%ls", _tbc_buf_);                                         \
    } while (0)

// ---------------------------------------------------------------------------
// Public macros — each strips at compile time when below the configured level.
// ---------------------------------------------------------------------------
#if TBC_LOG_LEVEL <= TBC_LOG_DEBUG
  #define LOG_DEBUG(fmt, ...) TBC_LOG_IMPL(L"[DBG]", fmt, ##__VA_ARGS__)
#else
  #define LOG_DEBUG(fmt, ...) do {} while(0)
#endif

#if TBC_LOG_LEVEL <= TBC_LOG_INFO
  #define LOG_INFO(fmt, ...)  TBC_LOG_IMPL(L"[INF]", fmt, ##__VA_ARGS__)
#else
  #define LOG_INFO(fmt, ...)  do {} while(0)
#endif

#if TBC_LOG_LEVEL <= TBC_LOG_WARN
  #define LOG_WARN(fmt, ...)  TBC_LOG_IMPL(L"[WRN]", fmt, ##__VA_ARGS__)
#else
  #define LOG_WARN(fmt, ...)  do {} while(0)
#endif

#if TBC_LOG_LEVEL <= TBC_LOG_ERROR
  #define LOG_ERROR(fmt, ...) TBC_LOG_IMPL(L"[ERR]", fmt, ##__VA_ARGS__)
#else
  #define LOG_ERROR(fmt, ...) do {} while(0)
#endif

// Convenience: always log + return HRESULT from a failing WinAPI call.
// Evaluates EXPR only once.
#define LOG_RETURN_IF_FAILED(expr)                                          \
    do {                                                                    \
        HRESULT _hr_ = (expr);                                              \
        if (FAILED(_hr_)) {                                                 \
            LOG_ERROR(L"%hs failed: 0x%08X", #expr, (unsigned)_hr_);       \
            return _hr_;                                                    \
        }                                                                   \
    } while (0)

#define LOG_BREAK_IF_FAILED(expr)                                           \
    HRESULT _hr_ = (expr);                                                  \
    if (FAILED(_hr_)) {                                                     \
        LOG_ERROR(L"%hs failed: 0x%08X", #expr, (unsigned)_hr_);           \
        break;                                                              \
    }
