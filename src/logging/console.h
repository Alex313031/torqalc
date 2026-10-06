#ifndef MINI_LOGGER_CONSOLE_H_
#define MINI_LOGGER_CONSOLE_H_

#include "logging_base.h"

typedef BOOL(WINAPI* ATTACH_CONSOLE_)(DWORD dwProcessId);

namespace logging {

  extern volatile bool console_attached;

  // Gets if a console is already attached for this process.
  bool GetIsConsoleAttached();

  // Attaches console to window, only one allowed per process.
  // Returns 0 (true) if successful, 1 if already attached, 2 if other error (false)
  // static cast to a bool to use as original bool implementation
  int AttachConsoleImpl();

  // Detaches console to allow attaching a new one.
  bool DetachConsoleImpl();

  // Update the console window's title
  bool SetLogConsoleTitle(const std::wstring& title);

  // Returns the currently attached console to this process, if any.
  HWND GetCurrentConsole();

  // Shows console window if it exists.
  bool ShowConsole(const bool activate);

  // Hides (but doesn't close or empty stdout) console window if it exists.
  bool HideConsole();

  // Toggles hide/show state of console, and whether to activate the window if showing.
  bool ToggleShowConsole(const bool activate);

  // Opens a console if app wasn't launched from command line, and syncs all logging output to it
  bool RouteStdioToConsole(bool create_console_if_not_found);

} // namespace logging

#endif // MINI_LOGGER_CONSOLE_H_
