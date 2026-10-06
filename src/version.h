#ifndef TORQALC_VERSION_H_
#define TORQALC_VERSION_H_

// This file is for specifying the target windows version, as well as application
// version constants.

// We need to define _UNICODE and UNICODE for TCHAR
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

#include "winsdkver.h"

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

#include "sdkddkver.h"

// clang-format off: Version DEFINES left alone

// These next few lines are where we control version number and copyright year
// Adhere to semver -> semver.org
#define MAJOR_VERSION 0
#define MINOR_VERSION 1
#define BUILD_VERSION 0

// Macro to convert to string
#if !defined(_STRINGIZER_)
 #define _STRINGIZER_
 #define _STRINGIZER(in) #in
 #define STRINGIZE(in) _STRINGIZER(in)
  // Wide-string variant: L ## "x" -> L"x". Two levels so the argument expands
 // before the L## paste widens the resulting narrow literal.
 #define _WIDEN(in) L ## in
 #define WIDEN(in) _WIDEN(in)
#endif // !defined(_STRINGIZER_)

// Main version constant
#ifndef _VERSION
 // Run stringizer above
 #define _VERSION(major,minor,build) WIDEN(STRINGIZE(major.minor.build))
#endif // _VERSION

// String constants
#define VERSION_STRING _VERSION(MAJOR_VERSION, MINOR_VERSION, BUILD_VERSION)

// Base name/identifier as BARE tokens (not "..." literals) so STRINGIZE can fold
// them into the composite single literals below. Never pass an existing literal
// to STRINGIZE: it escapes the inner quotes (\"), which is wrong content and
// which llvm-rc rejects outright (RC escapes a quote as "", not \").
#define APP_NAME_TOKENS Torqalc
#define INTERNAL_TOKENS torqalc

#define APP_NAME        WIDEN(STRINGIZE(APP_NAME_TOKENS)) // L"Torqalc"
#define MAIN_WNDCLASS   L"TorqalcWin32"  // Our main Window Class unique name

#define COMMENTS        L"https://github.com/Alex313031/torqalc" // Project GitHub URL
#define COMPANYNAME     L"Alex313031" // My developer name
#define FILE_DESCRIPT   L"Electromechanical energy calculator for Win32" // File description
#define INTERNAL_NAME   WIDEN(STRINGIZE(INTERNAL_TOKENS)) // L"torqalc"
#define ORIG_FILENAME   WIDEN(STRINGIZE(INTERNAL_TOKENS.exe)) // L"torqalc.exe"
#define PRODUCT_NAME    APP_NAME // Product name
#define TRADEMARKS      L"BSD-3" // License
#define LEGAL_COPYRIGHT L"\251 2026 Alex313031" // \251 is the © symbol

#define ABOUT_TITLE     WIDEN(STRINGIZE(About APP_NAME_TOKENS)) // L"About Torqalc"
#define ABOUT_CONTENT   L"Electromechanical energy calculator for Windows"
#define ABOUT_VERSION   WIDEN(STRINGIZE(Version MAJOR_VERSION.MINOR_VERSION.BUILD_VERSION)) // L"Version 0.1.0"
#define ABOUT_COPYRIGHT LEGAL_COPYRIGHT

#ifndef _PACKVERSION
 #define _PACKVERSION(major,minor) MAKELONG((minor), ((major) << 8))
#endif

#ifndef VS_FF_RELEASE
 #define VS_FF_RELEASE 0x00000000L
#endif

// clang-format on: Done with version DEFINES

#endif // TORQALC_VERSION_H_
