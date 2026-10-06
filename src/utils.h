#ifndef TORQALC_UTILS_H_
#define TORQALC_UTILS_H_

#include "constants.h"
#include "framework.h"

// clang-format off
#include <logging.h>
// clang-format on

extern bool g_debug_mode;

// Gets the desired font at the specified size (in pixels). Face name
// defaults to Tahoma. Caller owns the returned HFONT and must
// DeleteObject it when done. Returns nullptr on failure.
HFONT GetFont(int size, std::wstring font = L"Tahoma", bool italic = false);

// Fills a rect with a solid color. Wraps the CreateSolidBrush + FillRect
// + DeleteObject trio so call sites don't have to repeat all three (and
// can't forget the DeleteObject and leak a GDI brush).
bool FillRectWithColor(HDC hdc, const RECT& rc, COLORREF color);

// Gets the current side by side directory, regardless of where .exe is started from
const std::wstring GetExeDir();

// Helper functions for MessageBoxW
bool InfoBox(HWND hWnd, const std::wstring& title, const std::wstring& message);

bool WarnBox(HWND hWnd, const std::wstring& title, const std::wstring& message);

bool ErrorBox(HWND hWnd, const std::wstring& title, const std::wstring& message);

// Gets version as human readable wstring.
const std::wstring GetVersionString();

// Returns APP_NAME as wstring, for easier usage.
const std::wstring GetAppName();

// For checking system's commctl32.dll
bool IsCommCtrlAtLeast(const DWORD to_compare);

// Creates a tooltip window owned by hWndParent and attaches it to hWndControl
// with TTF_SUBCLASS so hover detection is handled automatically - no manual
// WM_MOUSEMOVE relaying or TTM_RELAYEVENT plumbing needed. Returns the
// tooltip HWND on success, or nullptr if any argument is null or the
// tooltip window can't be created. The tooltip text is captured by pointer
// (TOOLINFOW::lpszText), so the buffer must remain valid for the tooltip's
// lifetime - pass a string literal or a long-lived constant.
HWND AddTooltip(HWND hWndParent,
                HWND hWndControl,
                HINSTANCE hInst,
                const wchar_t* tooltipText = L"Dummy tooltip");

// Gets if a given menu has an item CHECKED or not.
bool IsMenuChecked(HMENU menu, UINT id);

// Gets if a given menu has an item GRAYED or not.
bool IsMenuGrayed(HMENU menu, UINT id);

// Toggles a given menu IDs CHECKED state.
bool ToggleMenuCheck(HWND hWnd, UINT id);

// Confirmation dialog for exit
bool ConfirmExit(HWND hWnd);

// Centers `hWnd` on screen. When `multimon` is true, uses
// MonitorFromWindow + GetMonitorInfo so the window centres inside the
// work area (minus the taskbar) of whichever monitor it currently sits
// on - DPI / multi-display friendly. When false (or when running on NT4
// where the multimon APIs don't exist), falls back to the primary
// display's full desktop rect via GetDesktopWindow, the classic
// single-monitor approach that ignores the taskbar. Returns false on
// failure (null hWnd, GetMonitorInfo failure, etc.).
bool CenterWindowOnScreen(HWND hWnd, bool multimon);

// Welcome message to be displayed in console
const std::wstring GetWelcomeMessage();

// Gets if program is running on Wine, with optional string to get version.
bool IsRunningOnWine(std::string* outWineVer = nullptr);

#endif // TORQALC_UTILS_H_
