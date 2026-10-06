#ifndef TORQALC_CONSTANTS_H_
#define TORQALC_CONSTANTS_H_

#include "framework.h"

// Color constants
#define RGB_BLACK   RGB(0, 0, 0)
#define RGB_WHITE   RGB(255, 255, 255)
#define RGB_GREY    RGB(128, 128, 128)
#define RGB_DKGREY  RGB(64, 64, 64)
#define RGB_RED     RGB(255, 0, 0)
#define RGB_GREEN   RGB(0, 255, 0)
#define RGB_BLUE    RGB(0, 0, 255)
#define RGB_YELLOW  RGB(255, 255, 0)
#define RGB_CYAN    RGB(0, 255, 255)
#define RGB_MAGENTA RGB(255, 0, 255)

// Define for below, so that we can reuse the name in literals easily
#define TXT_NAME L"torqalc_results.txt"

// Output file name, written side-by-side with the .exe.
inline constexpr wchar_t kResultsFile[] = TXT_NAME;

// UTF-16 LE byte-order mark. Written as the first two bytes of the result file
// so Notepad / other readers know the encoding. The 0xFEFF code unit
// serializes to FF FE on disk for little-endian. Address-of works because
// inline constexpr variables have a unique address per program.
inline constexpr WORD kUTF16LEBOM = 0xFEFF;

// Names of the system DLLs we dynamically resolve symbols from. Centralized
// so a typo only has to be fixed once and so call sites read by intent
// (kNtDll) rather than literal ("ntdll.dll").
inline constexpr wchar_t kNtDll[]       = L"ntdll.dll";
inline constexpr wchar_t kKernel32Dll[] = L"kernel32.dll";
inline constexpr wchar_t kUser32Dll[]   = L"user32.dll";
inline constexpr wchar_t kComCtl32Dll[] = L"comctl32.dll";

// Default desired outer window size at startup.
inline constexpr INT CW_WIDTH  = 320;
inline constexpr INT CW_HEIGHT = 480;

// Min window size
inline constexpr INT CW_MINWIDTH  = 300;
inline constexpr INT CW_MINHEIGHT = 380;

// Top-pane control layout (in pixels, relative to the parent client area).
inline constexpr INT kGroupMargin   = 7;  // groupbox outer margin: left, right, bottom
inline constexpr INT kGroupOuterTop = 10; // groupbox outer top margin: (client top to frame line)
inline constexpr INT kGroupInnerPad = 10; // inner padding: frame line -> first control row
inline constexpr INT kPadLeft       = 14;
inline constexpr INT kPadTop        = 14;
inline constexpr INT kHGap          = 5;
inline constexpr INT kVGap          = 7;
inline constexpr INT kLabelWidth    = 72;
inline constexpr INT kControlHeight = 21;
inline constexpr INT kButtonWidth   = 100;
inline constexpr INT kButtonHeight  = 28;

// Child window style
inline constexpr DWORD dwCHILD = WS_CHILD | WS_VISIBLE;

// Minimum common controls version for certain functions, used for fallback codepaths
// See https://learn.microsoft.com/en-us/windows/win32/controls/common-control-versions
inline constexpr DWORD dwComCtl32TargetVer =
    _PACKVERSION(static_cast<DWORD>(5u), static_cast<DWORD>(82u));

#endif // TORQALC_CONSTANTS_H_
