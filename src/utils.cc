// Common utility functions

#include "utils.h"

#include <shlwapi.h>

#include "globals.h"
#include "resource.h"

/* ------------------------------ Static state ------------------------------ */

// Multimon APIs (Win98 / Win2k+); resolved dynamically so the binary loads
// cleanly on NT4, which has neither export.
typedef HMONITOR(WINAPI* FnMonitorFromWindow)(HWND, DWORD);
typedef BOOL(WINAPI* FnGetMonitorInfoW)(HMONITOR, LPMONITORINFO);

/* -------------------------- Forward declarations -------------------------- */

static DWORD GetCommCtrlVersion();
static FnMonitorFromWindow ResolveMonitorFromWindow();
static FnGetMonitorInfoW ResolveGetMonitorInfoW();

/* ------------------------------- Functions -------------------------------- */

HFONT GetFont(int size, std::wstring font, bool italic) {
  HDC hdc = GetDC(nullptr);
  if (!hdc) {
    return nullptr;
  }
  if (font.empty()) {
    LOG(ERROR) << L"Empty font supplied!";
  }
  // Negative height = "character height" in logical units (the cap
  // box), so passing -size yields ~size-pixel-tall glyphs on a
  // standard MM_TEXT DC. ANTIALIASED_QUALITY keeps big text from
  // looking jagged - the rest of the app embraces a retro aliased
  // look but 72-px text without smoothing is unreadable.
  int height;
  if (size <= 0) {
    height = -MulDiv(8, GetDeviceCaps(hdc, LOGPIXELSY), 72);
  } else {
    height = -size;
  }
  ReleaseDC(nullptr, hdc);
  HFONT hGetFont = CreateFontW(height, 0, 0, 0, FW_NORMAL, italic ? TRUE : FALSE, FALSE, FALSE,
                               DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                               ANTIALIASED_QUALITY, DEFAULT_PITCH | FF_DONTCARE, font.c_str());
  if (hGetFont == nullptr) {
    LOG(ERROR) << L"failed to get " << font << L" font!";
    return nullptr;
  }
  return hGetFont;
}

bool FillRectWithColor(HDC hdc, const RECT& rc, COLORREF color) {
  bool success = true;
  if (hdc == nullptr) {
    return false;
  }
  HBRUSH hBrush = CreateSolidBrush(color);
  if (hBrush == nullptr) {
    return false;
  }
  if (!FillRect(hdc, &rc, hBrush)) {
    success = false;
  }
  DeleteObject(hBrush);
  return success;
}

const std::wstring GetExeDir() {
  wchar_t exe_path[MAX_PATH];
  HMODULE this_app = GetModuleHandleW(nullptr);
  if (!this_app) {
    return std::wstring();
  }
  DWORD got_path = GetModuleFileNameW(nullptr, exe_path, MAX_PATH);
  if (got_path == 0 || got_path >= MAX_PATH) {
    return std::wstring();
  }

  // Find the last backslash to get the directory
  std::wstring fullPath(exe_path);
  size_t lastSlash = fullPath.find_last_of(L"\\/");
  std::wstring retval;
  if (lastSlash != std::wstring::npos) {
    retval = fullPath.substr(0, lastSlash + 1); // Include trailing slash
  } else {
    retval = fullPath;
  }
  return retval;
}

// MessageBoxW with MB_OK can be dismissed several ways the user considers
// equivalent: clicking OK (IDOK), clicking the X close button (IDCANCEL),
// or pressing Esc (IDCANCEL). All of those mean "the box showed and the
// user dismissed it" - which is what these helpers want to report as
// success. Only a 0 return means the box failed to display in the first
// place (bad hWnd, OOM, no desktop access, etc.); that's the real false.
// `hWnd ? hWnd : mainHwnd` falls back to the main window when the caller
// passed null - useful from helpers that don't have an hWnd of their own.
bool InfoBox(HWND hWnd, const std::wstring& title, const std::wstring& message) {
  return MessageBoxW(hWnd ? hWnd : mainHwnd, message.c_str(), title.c_str(),
                     MB_OK | MB_ICONINFORMATION) != 0;
}

bool WarnBox(HWND hWnd, const std::wstring& title, const std::wstring& message) {
  return MessageBoxW(hWnd ? hWnd : mainHwnd, message.c_str(), title.c_str(),
                     MB_OK | MB_ICONWARNING) != 0;
}

bool ErrorBox(HWND hWnd, const std::wstring& title, const std::wstring& message) {
  return MessageBoxW(hWnd ? hWnd : mainHwnd, message.c_str(), title.c_str(),
                     MB_OK | MB_ICONERROR) != 0;
}

const std::wstring GetVersionString() {
  // Build the wide version string from the integer macros, the single source
  // of truth in version.h. VERSION_STRING is a wide literal now and could be
  // returned directly; std::to_wstring is kept to stay standards-clean across
  // MinGW and MSVC alike.
  return std::to_wstring(MAJOR_VERSION) + L"." + std::to_wstring(MINOR_VERSION) + L"." +
         std::to_wstring(BUILD_VERSION);
}

const std::wstring GetAppName() {
  const std::wstring app_name = std::wstring(APP_NAME);
  return app_name;
}

static DWORD GetCommCtrlVersion() {
  // Resolve the system comctl32.dll path explicitly. GetSystemDirectoryW
  // returns 0 on failure, or >= MAX_PATH if our buffer was too small (in
  // which case it reports the required size). Either is fatal for us -
  // bail rather than fall through with an empty path that would let
  // LoadLibraryW search the standard DLL order and silently bypass the
  // "explicitly use the system one" intent.
  wchar_t systemDir[MAX_PATH];
  const UINT length = GetSystemDirectoryW(systemDir, MAX_PATH);
  if (length == 0 || length >= MAX_PATH) {
    return 0x0;
  }
  const std::wstring comctl32_path = std::wstring(systemDir) + L"\\" + kComCtl32Dll;

  HMODULE hComCtl32Dll = LoadLibraryW(comctl32_path.c_str());
  if (hComCtl32Dll == nullptr) {
    return 0x0;
  }

  DWORD dwVersion = 0x0;
  DLLGETVERSIONPROC pDllGetVersion =
      reinterpret_cast<DLLGETVERSIONPROC>(GetProcAddress(hComCtl32Dll, "DllGetVersion"));
  if (pDllGetVersion == nullptr) {
    return 0x0;
  } else {
    DLLVERSIONINFO dvi    = {sizeof(dvi)};
    const HRESULT hresult = pDllGetVersion(&dvi);
    if (hresult == S_OK) {
      dwVersion = _PACKVERSION(dvi.dwMajorVersion, dvi.dwMinorVersion);
    }
  }
  FreeLibrary(hComCtl32Dll);
  return dwVersion;
}

bool IsCommCtrlAtLeast(const DWORD to_compare) {
  const DWORD kCommCtrlVer = GetCommCtrlVersion();
  return kCommCtrlVer >= to_compare;
}

HWND AddTooltip(HWND hWndParent, HWND hWndControl, HINSTANCE hInst, const wchar_t* tooltipText) {
  if (hWndParent == nullptr || hWndControl == nullptr || tooltipText == nullptr) {
    return nullptr;
  }

  HWND hTooltip = CreateWindowExW(
      WS_EX_NOACTIVATE, TOOLTIPS_CLASS, nullptr, TTS_ALWAYSTIP | TTS_NOPREFIX, CW_USEDEFAULT,
      CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, hWndParent, nullptr, hInst, nullptr);
  if (hTooltip == nullptr) {
    return nullptr;
  }

  TOOLINFOW ti = {};
  // Windows 2000 even with I.E. 6 reports false, since system comctl32.dll is not updated.
  static const bool can_use_582_controls = IsCommCtrlAtLeast(dwComCtl32TargetVer);
  if (can_use_582_controls) {
    ti.cbSize = sizeof(ti);
  } else {
    // MinGW's TOOLINFOW always includes lpReserved (V3 layout). Windows 2000's
    // comctl32 v5.81 only supports up to V2 (through lParam) - passing
    // sizeof(ti) makes TTM_ADDTOOLW reject the struct on Win2k. Fall back to
    // the V2 size on pre-XP systems.
    ti.cbSize = TTTOOLINFOW_V2_SIZE;
  }
  ti.uFlags   = TTF_SUBCLASS | TTF_IDISHWND;
  ti.hwnd     = hWndParent;
  ti.uId      = reinterpret_cast<UINT_PTR>(hWndControl);
  ti.lpszText = const_cast<wchar_t*>(tooltipText);

  SendMessageW(hTooltip, TTM_ADDTOOLW, 0, reinterpret_cast<LPARAM>(&ti));
  return hTooltip;
}

// Menu-state helpers. The .rc's CHECKED flags double as default-setting
// storage - ApplyMenuDefaults reads each item's initial state at startup
// and pushes it into the engine, so adjusting the defaults is just a
// matter of toggling CHECKED in the .rc.
bool IsMenuChecked(HMENU menu, UINT id) {
  if (menu == nullptr) {
    return false;
  }
  const UINT state = GetMenuState(menu, id, MF_BYCOMMAND);
  // GetMenuState returns 0xFFFFFFFF when the item isn't found.
  if (state == static_cast<UINT>(-1)) {
    return false;
  }
  return (state & MF_CHECKED) != 0;
}

// Reads MF_GRAYED off a menu item so the .rc's GRAYED flag can act as a
// "this feature is disabled at build time" switch the same way CHECKED
// acts as a default-on / default-off toggle. Returns false if the item
// isn't found - missing items aren't "greyed", they're just absent.
bool IsMenuGrayed(HMENU menu, UINT id) {
  if (menu == nullptr) {
    return false;
  }
  const UINT state = GetMenuState(menu, id, MF_BYCOMMAND);
  if (state == static_cast<UINT>(-1)) {
    return false;
  }
  return (state & MF_GRAYED) != 0;
}

// Flips a checkable menu item and returns the new state. Used by the
// WM_COMMAND handlers so a single line covers "toggle + push into engine".
bool ToggleMenuCheck(HWND hWnd, UINT id) {
  HMENU menu = GetMenu(hWnd);
  if (menu == nullptr) {
    return false;
  }
  const bool now_checked = !IsMenuChecked(menu, id);
  CheckMenuItem(menu, id, MF_BYCOMMAND | (now_checked ? MF_CHECKED : MF_UNCHECKED));
  return now_checked;
}

// Confirmation dialog for exit
bool ConfirmExit(HWND hWnd) {
  const int exit_dialog = MessageBoxW(hWnd, L"Are you sure you want to Exit?", L"Confirm Exit",
                                      MB_YESNOCANCEL | MB_ICONQUESTION | MB_DEFBUTTON1);
  return exit_dialog == IDYES;
}

static FnMonitorFromWindow ResolveMonitorFromWindow() {
  static FnMonitorFromWindow pfn = nullptr;
  static bool s_resolved         = false;
  if (!s_resolved) {
    HMODULE hUser32 = GetModuleHandleW(kUser32Dll);
    if (hUser32 != nullptr) {
      pfn = reinterpret_cast<FnMonitorFromWindow>(GetProcAddress(hUser32, "MonitorFromWindow"));
    }
    s_resolved = true;
  }
  return pfn;
}

static FnGetMonitorInfoW ResolveGetMonitorInfoW() {
  static FnGetMonitorInfoW pfn = nullptr;
  static bool s_resolved       = false;
  if (!s_resolved) {
    HMODULE hUser32 = GetModuleHandleW(kUser32Dll);
    if (hUser32 != nullptr) {
      pfn = reinterpret_cast<FnGetMonitorInfoW>(GetProcAddress(hUser32, "GetMonitorInfoW"));
    }
    s_resolved = true;
  }
  return pfn;
}

bool CenterWindowOnScreen(HWND hWnd, bool multimon) {
  if (hWnd == nullptr) {
    return false;
  }
  RECT window_rect;
  if (!GetWindowRect(hWnd, &window_rect)) {
    return false;
  }
  const int window_w = window_rect.right - window_rect.left;
  const int window_h = window_rect.bottom - window_rect.top;

  // NT4 has neither MonitorFromWindow nor GetMonitorInfoW. When the caller
  // asks for multimon placement but the OS can't deliver, silently fall
  // through to the single-monitor path rather than failing - the window
  // still gets centred, just on the primary display.
  FnMonitorFromWindow pfnMonitorFromWindow = multimon ? ResolveMonitorFromWindow() : nullptr;
  FnGetMonitorInfoW pfnGetMonitorInfoW     = multimon ? ResolveGetMonitorInfoW() : nullptr;
  const bool use_multimon = multimon && (pfnMonitorFromWindow != nullptr) &&
                            (pfnGetMonitorInfoW != nullptr);

  RECT screen_rect;
  if (use_multimon) {
    // Pick the monitor `hWnd` currently sits on; NEARESTONOTNULL guarantees
    // a valid HMONITOR even for off-screen windows. rcWork excludes the
    // taskbar / docked appbars so the centred window doesn't end up half
    // under them.
    HMONITOR hMon            = pfnMonitorFromWindow(hWnd, MONITOR_DEFAULTTONEAREST);
    MONITORINFO monitor_info = {};
    monitor_info.cbSize      = sizeof(monitor_info);
    if (hMon == nullptr || !pfnGetMonitorInfoW(hMon, &monitor_info)) {
      return false;
    }
    screen_rect = monitor_info.rcWork;
  } else {
    // Classic single-monitor path: GetDesktopWindow is the whole primary
    // screen rect, taskbar and all. Predates the multimon APIs and keeps
    // working on Win2k for callers that don't care about per-monitor
    // placement (and on NT4 where the multimon APIs don't exist).
    if (!GetWindowRect(GetDesktopWindow(), &screen_rect)) {
      return false;
    }
  }
  const int screen_w = screen_rect.right - screen_rect.left;
  const int screen_h = screen_rect.bottom - screen_rect.top;
  const int new_x    = (screen_rect.left + (screen_w - window_w)) / 2;
  const int new_y    = (screen_rect.top + (screen_h - window_h)) / 2;
  // SWP_NOSIZE / NOZORDER / NOACTIVATE: pure reposition, don't disturb
  // size, stacking order, or focus.
  return SetWindowPos(hWnd, nullptr, new_x, new_y, 0, 0,
                      SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE) != FALSE;
}

// Log name and version
const std::wstring GetWelcomeMessage() {
  std::wostringstream wostr;
  wostr << L"---- Welcome to " << GetAppName() << L" ----" << L"\n"
        << L"       Version: " << GetVersionString() << (is_debug ? L" DEBUG" : L"");
  const std::wstring welcome = wostr.str();
  return welcome;
}

bool IsRunningOnWine(std::string* outWineVer) {
  HMODULE ntdll = GetModuleHandleW(kNtDll);
  if (ntdll == nullptr) {
    return false;
  }
  // Cleaner one-liner via a typedef than splitting the function-pointer
  // declaration and the assignment across two lines.
  typedef const char*(CDECL * WineGetVersion_t)(void);
  const WineGetVersion_t pwine_get_version =
      reinterpret_cast<WineGetVersion_t>(GetProcAddress(ntdll, "wine_get_version"));
  if (pwine_get_version == nullptr) {
    return false;
  }
  // Wine's implementation always returns a valid string in practice, but
  // std::string(nullptr) is undefined behavior - guard it.
  const char* wineVer = pwine_get_version();
  if (wineVer == nullptr) {
    return false;
  }
  // outWineVer is optional: callers that only care about the bool can pass
  // nullptr. Without this null-check we'd crash on the dereference below.
  if (outWineVer != nullptr) {
    *outWineVer = wineVer;
  }
  return true;
}
