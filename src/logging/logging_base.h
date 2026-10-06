#ifndef MINI_LOGGER_LOGGING_BASE_H_
#define MINI_LOGGER_LOGGING_BASE_H_

// NOTE: This is a precompiled header file (PCH)

// clang-format off
#ifndef UNICODE
 #define UNICODE
#endif

#ifndef _UNICODE
 #define _UNICODE
#endif

#if defined(__clang__) && defined(_UNICODE)
 #pragma code_page(65001) // UTF-8
#endif

/* Including SDKDDKVer.h defines the highest available Windows platform.
   If you wish to build your application for a previous Windows platform, include WinSDKVer.h and
   set the _WIN32_WINNT macro to the platform you wish to support before including SDKDDKVer.h. */

#ifndef __MINGW32__
 #include <WinSDKVer.h> // Doesn't exist in MinGW
#endif

#ifndef _WIN32_WINNT
 #define _WIN32_WINNT 0x0500 // Windows 2000
#endif

#ifndef WINVER
 #define WINVER 0x0500 // Same as _WIN32_WINNT above
#endif

#ifndef _WIN64_WINNT
 #define _WIN64_WINNT 0x0502 // Minimum version for 64 bit, Windows Server 2003
#endif

#ifndef _WIN32_IE
 #define _WIN32_IE 0x0501 // Minimum Internet Explorer version for common controls
#endif

#if _WIN32_WINNT < 0x0601 // If we are less than Windows 7, use old ATL for safety
 #ifndef _ATL_XP_TARGETING
  #define _ATL_XP_TARGETING // For using XP-compatible ATL/MFC functions
 #endif
#endif

#ifndef __MINGW32__
 #include <SDKDDKVer.h> // Doesn't exist in MinGW
#endif

// Macro to convert to string
#if !defined(_STRINGIZER_)
 #define _STRINGIZER_
 #define _STRINGIZER(in) #in
 #define STRINGIZE(in) _STRINGIZER(in)
#endif // !defined(_STRINGIZER_)

// Main version constant
#ifndef _VERSION
 // Run stringizer above
 #define _VERSION(major,minor,build) STRINGIZE(major) "." STRINGIZE(minor) "." STRINGIZE(build)
#endif // _VERSION

// These next few lines are where we control version number and copyright year
// Adhere to semver > semver.org
#define LOGGER_MAJOR_VERSION 0
#define LOGGER_MINOR_VERSION 2
#define LOGGER_BUILD_VERSION 7

#define LOGGER_VERSION_STRING _VERSION(MAJOR_VERSION, MINOR_VERSION, BUILD_VERSION)

#define _LIBNAME L"HawkLogger"

#if __cplusplus < 201103L || !defined(__cplusplus)
 #error _LIBNAME only supports C++11 and above
#endif

#include <windows.h> // Main Windows include
#include <wincon.h>  // Console API functions
#include <tchar.h>   // Wide/Short characters
// clang-format on

// For strings, writing to console/file, etc.
#include <iomanip>
#include <iostream>
#include <limits>
#include <sstream>
#include <string>

namespace logging {
  extern bool logging_initialized; // Global boolean for safety
}

#endif // MINI_LOGGER_LOGGING_BASE_H_
