#include "console.h"

#include <io.h>

#include "check.h"
#include "logging.h"

namespace logging {
  // For tracking console attach state.
  volatile bool console_attached        = false;
  static ATTACH_CONSOLE_ pAttachConsole = nullptr;
}

bool logging::GetIsConsoleAttached() {
  if (GetCurrentConsole() != nullptr) {
    CHECK(console_attached); // Should always be true
  }
  return console_attached;
}

int logging::AttachConsoleImpl() {
  int retval = 0;
  if (GetIsConsoleAttached()) {
    retval = 1;
  }
  if (retval == 1) {
    // 1 Soft error, already attached
    MessageBoxW(nullptr, L"Console Already Attached!", L"Console Attach Warning",
                MB_OK | MB_ICONWARNING);
  } else {
    // Allow and allocate conhost for cmd.exe logging window
    const bool attached_console = RouteStdioToConsole(true /* open cmd if none */);

    if (attached_console) {
      retval = 0; // 0 Means TRUE/OK
      // Title the console so the user can tell at a glance which process's
      // logging window this is. Only on the fresh-attach path - if a console
      // was already attached (retval == 1) it was set up by something else
      // and we shouldn't rename it. kProgName is populated by InitLogging
      // before it calls us.
      if (!kProgName.empty()) {
        SetLogConsoleTitle(kProgName + L" Logging Console");
      }
    } else {
      MessageBoxW(nullptr, L"Failed to attach console!", L"Console Attach Error",
                  MB_OK | MB_ICONERROR);
      retval = 2; // Other error
    }
  }
  console_attached = retval == 0 || retval == 1;
  return retval;
}

bool logging::DetachConsoleImpl() {
  if (!GetIsConsoleAttached()) {
    MessageBoxW(nullptr, L"Console Already Detached!", L"Console Detach Warning",
                MB_OK | MB_ICONWARNING);
    return true;
  }
  if (FreeConsole()) {
    console_attached = false;
    return true;
  } else {
    const std::wstring msg =
        L"Failed to detach console! Error = " + std::to_wstring(GetLastError());
    MessageBoxW(nullptr, msg.c_str(), L"Console Detach Failure", MB_OK | MB_ICONERROR);
    return false;
  }
}

bool logging::SetLogConsoleTitle(const std::wstring& title) {
  return SetConsoleTitleW(title.c_str());
}

typedef HWND(WINAPI* FnGetConsoleWindow)(void);

// Resolves GetConsoleWindow at runtime. Win2k+ exports it from kernel32.dll;
// pure NT4 does not. Returns nullptr when unavailable so the caller can
// dispatch to the FindWindow fallback below.
static FnGetConsoleWindow ResolveGetConsoleWindow() {
  static FnGetConsoleWindow pfn = nullptr;
  static bool s_resolved        = false;
  if (!s_resolved) {
    HMODULE hKernel32 = GetModuleHandleW(L"kernel32.dll");
    if (hKernel32 != nullptr) {
      pfn = reinterpret_cast<FnGetConsoleWindow>(GetProcAddress(hKernel32, "GetConsoleWindow"));
    }
    s_resolved = true;
  }
  return pfn;
}

// NT4 fallback: there is no documented HWND lookup for a console, but a
// long-standing workaround is to set a unique title, FindWindowW for it,
// then restore the original. csrss.exe processes the rename asynchronously
// so a short Sleep gives it time to land. Cached after first success so the
// title only flickers once per process.
static HWND FindConsoleHwndNT4() {
  static HWND s_cached   = nullptr;
  static bool s_searched = false;
  if (s_searched) {
    return s_cached;
  }
  s_searched = true;

  wchar_t saved_title[1024];
  wchar_t unique_title[64];
  if (GetConsoleTitleW(saved_title, ARRAYSIZE(saved_title)) == 0) {
    saved_title[0] = L'\0';
  }
  swprintf(unique_title, ARRAYSIZE(unique_title), L"__console_hwnd_lookup_%lu__",
           static_cast<unsigned long>(GetCurrentProcessId()));
  if (!SetConsoleTitleW(unique_title)) {
    return nullptr;
  }
  Sleep(40u); // Give csrss 40ms. to apply the rename.
  s_cached = FindWindowW(nullptr, unique_title);
  SetConsoleTitleW(saved_title);
  return s_cached;
}

HWND logging::GetCurrentConsole() {
  FnGetConsoleWindow pfn = ResolveGetConsoleWindow();
  if (pfn != nullptr) {
    return pfn();
  }
  return FindConsoleHwndNT4();
}

// ShowWindow's BOOL return is "was the window previously visible",
// NOT "did the call succeed" - so SW_SHOW / SW_SHOWNOACTIVATE on a
// hidden window returns 0 even though the show worked, and SW_HIDE
// on an already-hidden window also returns 0. We can't use it as a
// success indicator. Just trust the call to do its job; both
// helpers report true unless the console isn't even attached.

bool logging::ShowConsole(const bool activate) {
  const int showstate = activate ? SW_SHOW : SW_SHOWNOACTIVATE;
  const HWND console  = GetCurrentConsole();
  if (console == nullptr) {
    CLOG(ERROR) << L"ShowConsole() called when console not attached!";
    FLOG(ERROR) << L"ShowConsole() called when console not attached.";
    return false;
  }
  // IsWindowVisible is unreliable for the conhost pseudo-window on
  // Win10/11 Terminal (it's the wrong HWND), so we skip the early-out
  // and just call ShowWindow - it's a no-op when already in the
  // requested state.
  ShowWindow(console, showstate);
  return true;
}

bool logging::HideConsole() {
  const HWND console = GetCurrentConsole();
  if (console == nullptr) {
    LOG(WARN) << L"Console not attached.";
    return false;
  }
  ShowWindow(console, SW_HIDE);
  return true;
}

bool logging::ToggleShowConsole(const bool activate) {
  const int showstate = activate ? SW_SHOW : SW_SHOWNOACTIVATE;
  const HWND console  = GetCurrentConsole();
  if (console == nullptr) {
    LOG(WARN) << L"Console not attached.";
    return false;
  }
  // ShowWindow's BOOL return is "was previously visible", not success;
  // see ShowConsole / HideConsole. Use IsWindowVisible to pick which
  // direction to flip, then trust the call.
  const bool visible = IsWindowVisible(console);
  ShowWindow(console, visible ? SW_HIDE : showstate);
  return true;
}

bool logging::RouteStdioToConsole(bool create_console_if_not_found) {
  if (console_attached) {
    std::wcerr << __func__ << L" console_attached = true" << std::endl;
    return true;
  }
  // We don't use GetStdHandle() to check stdout/stderr here because
  // it can return dangling IDs of handles that were never inherited
  // by this process.  These IDs could have been reused by the time
  // this function is called.  The CRT checks the validity of
  // stdout/stderr on startup (before the handle IDs can be reused).
  // _fileno(stdout) will return -2 (_NO_CONSOLE_FILENO) if stdout was
  // invalid.
  if (_fileno(stdout) >= 0 || _fileno(stderr) >= 0) {
    // _fileno was broken for SUBSYSTEM:WINDOWS from VS2010 to VS2012/2013. See
    // http://crbug.com/358267. Confirm that the underlying HANDLE is valid before aborting.
    intptr_t stdout_handle = _get_osfhandle(_fileno(stdout));
    intptr_t stderr_handle = _get_osfhandle(_fileno(stderr));
    if (stdout_handle >= 0 || stderr_handle >= 0) {
      // stdout or stderr already point to a valid stream. Maybe abort?
    }
  }

  pAttachConsole = reinterpret_cast<ATTACH_CONSOLE_>(
      GetProcAddress(GetModuleHandleW(L"kernel32.dll"), "AttachConsole"));
  if (!pAttachConsole) {
    // Didn't get AttachConsole, probably running on Windows 2000, in which case just AllocConsole.
    if (!AllocConsole()) {
      MessageBoxW(nullptr, L"AllocConsole failed!", L"AllocConsole Error", MB_OK | MB_ICONERROR);
      NOTREACHED();
      return false;
    }
  } else {
#if _WIN32_WINNT < 0x0500
    // Windows NT 4 doesn't have ATTACH_PARENT_PROCESS
    if (!pAttachConsole((DWORD)-1)) {
#else
    if (!pAttachConsole(ATTACH_PARENT_PROCESS)) {
#endif
      unsigned int result = GetLastError();
      // Was probably already attached.
      if (result == ERROR_ACCESS_DENIED) {
        MessageBoxW(nullptr, L"ERROR_ACCESS_DENIED", L"AttachConsole_t Error",
                    MB_OK | MB_ICONERROR);
        return false;
      }
      if (create_console_if_not_found) {
        // Make a new console if attaching to parent fails with any other error.
        // It should be ERROR_INVALID_HANDLE at this point, which means the
        // browser was likely not started from a console.
        if (!AllocConsole()) {
          MessageBoxW(nullptr, L"AllocConsole failed!", L"AllocConsole Error",
                      MB_OK | MB_ICONERROR);
          NOTREACHED();
          return false;
        }
      } else {
        MessageBoxW(nullptr, L"Not creating console", L"RouteStdioToConsole Warning",
                    MB_OK | MB_ICONWARNING);
        return false;
      }
    }
  }

  // Arbitrary byte count to use when buffering output lines.  More
  // means potential waste, less means more risk of interleaved
  // log-lines in output.
  enum { kOutputBufferSize = 32 * 1024 };

  if (freopen("CONOUT$", "w", stdout)) {
    setvbuf(stdout, nullptr, _IOLBF, kOutputBufferSize);
    // Overwrite FD 1 for the benefit of any code that uses this FD
    // directly.  This is safe because the CRT allocates FDs 0, 1 and
    // 2 at startup even if they don't have valid underlying Windows
    // handles.  This means we won't be overwriting an FD created by
    // _open() after startup.
    _dup2(_fileno(stdout), 1);
  } else {
    MessageBoxW(nullptr, L"freopen stdout failed!", L"freopen Error", MB_OK | MB_ICONERROR);
  }
  if (freopen("CONOUT$", "w", stderr)) {
    setvbuf(stderr, nullptr, _IOLBF, kOutputBufferSize);
    _dup2(_fileno(stderr), 2);
  } else {
    MessageBoxW(nullptr, L"freopen stderr failed!", L"freopen Error", MB_OK | MB_ICONERROR);
  }

  // Fix all cout, wcout, cin, wcin, cerr, wcerr, clog and wclog together.
  std::ios::sync_with_stdio();
  return true;
}
