/*------------------------------------------
   Torqalc Win32
   Copyright (c) 2026 Alex313031
  ------------------------------------------*/

#include "main.h"

#include "convert.h"
#include "globals.h"
#include "resource.h"

/* -------------------------------- Statics --------------------------------- */

// Latched at startup from the .rc's initial GRAYED flag on IDM_CONSOLE.
// When true, UpdateConsoleToggleMenu leaves the item greyed and the
// WM_COMMAND handler refuses to toggle - the .rc gets the final say on
// whether the feature is available at all.
static bool s_console_menu_user_disabled = false;

// Our own view of whether the user has toggled the console hidden. We can't
// use IsWindowVisible(GetConsoleWindow()) for this because on Win10/11 with
// Windows Terminal as the default conhost, GetConsoleWindow returns a
// permanently-hidden pseudo-window owned by conhost.exe. Track intent
// instead: wWinMain hides the console at startup unless --debug / --version
// / --help asked for it, and the flag flips on every successful Show/Hide.
static bool s_console_hidden = false;

static std::string winever = ""; // For storing Wine version if applicable

// Assumed AC line voltage, used to turn a lone Amps reading into watts and to
// back Volts/Amps out of a power value. Toggled by Settings -> Default Line
// Volts (115 / 230); 115 is the .rc default (CHECKED).
static long double s_line_volts = 115.0L;

// Control ID of the field the user most recently typed into - the "source"
// a Calculate reads from (0 = nothing edited yet). Updated on EN_CHANGE, but
// only for genuine user edits: s_programmatic_update suppresses the EN_CHANGE
// storm our own SetWindowTextW calls raise while filling results, which would
// otherwise make the tracker chase its own tail.
static int s_last_edited          = 0;
static bool s_programmatic_update = false;

// Shared font for every child control; created in CreateChildControls and
// freed in WM_DESTROY.
static HFONT s_uiFont = nullptr;

/* -------------------------------- Globals --------------------------------- */

HWND mainHwnd      = nullptr;
HWND hVoltsEdit    = nullptr;
HWND hAmpsEdit     = nullptr;
HWND hWattsEdit    = nullptr;
HWND hImpHPEdit    = nullptr;
HWND hMetHPEdit    = nullptr;
HWND hEleHPEdit    = nullptr;
HWND hFtLbsEdit    = nullptr;
HWND hCaloriesEdit = nullptr;
HWND hCalcButton   = nullptr;
HWND hClearButton  = nullptr;

HINSTANCE g_hInstance = nullptr;

int cxClient = 0;
int cyClient = 0;

bool g_debug_mode = is_debug;
// CLI flags. Set by ParseCommandLine before InitLogging runs so the log
// sink picks up --debug, and so --version / --help can short-circuit
// wWinMain before the window is created.
bool g_show_version = false;
bool g_show_help    = false;

// Store handles to main icon since commonly used
HICON kMainIcon  = nullptr;
HICON kSmallIcon = nullptr;

// Whether we have commctl32 5.82 (XP/I.E 6.0)
bool can_use_582_controls = false;

COLORREF g_bkg_color = GetSysColor(COLOR_3DFACE); // Standard grey background

bool is_on_wine = false; // Whether we are on wine

/* -------------------------- Forward declarations -------------------------- */

static bool ParseCommandLine(int argc, LPWSTR argv[]);
static int ShowVersionAndExit();
static int ShowHelpAndExit();
static void UpdateConsoleToggleMenu(HWND hWnd);
static bool CreateChildControls(HWND hWnd);
static void LayoutChildren(HWND hWnd);
static void OnCalculate(HWND hWnd);
static void ClearAllFields();
static void SetLineVolts(HWND hWnd, long double volts, UINT checkedId);

/* ------------------------------- Functions -------------------------------- */

bool RegisterWndClass(HINSTANCE hInstance, LPCWSTR className) {
  if (kMainIcon == nullptr || kSmallIcon == nullptr) {
    return false;
  }
  WNDCLASSEXW wndclass;
  wndclass.cbSize      = sizeof(WNDCLASSEX);
  wndclass.style       = CS_HREDRAW | CS_VREDRAW;
  wndclass.lpfnWndProc = WindowProc;
  wndclass.cbClsExtra  = 0;
  wndclass.cbWndExtra  = 0;
  wndclass.hInstance   = hInstance;
  wndclass.hIcon       = kMainIcon;
  wndclass.hCursor     = LoadCursorW(nullptr, IDC_ARROW);
  // Classic grey client area; the OS repaints it on WM_SYSCOLORCHANGE because
  // we hand it the system pseudo-brush rather than a fixed HBRUSH.
  wndclass.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_3DFACE + 1);
  wndclass.lpszMenuName  = MAKEINTRESOURCEW(IDR_MAIN);
  wndclass.lpszClassName = className;
  wndclass.hIconSm       = kSmallIcon;

  // RegisterClassEx returns an ATOM (typedef unsigned short - really a short
  // pointer left over from Win16 days), 0 on failure. The double cast spells
  // out "this is an ATOM-shaped zero" rather than relying on the implicit
  // promotion from int 0.
  if (RegisterClassExW(&wndclass) == static_cast<ATOM>(static_cast<unsigned short>(0))) {
    return false;
  }
  return true;
}

bool InitWindow(HINSTANCE hInstance, LPCWSTR className, LPCWSTR title, int iCmdShow) {
  static constexpr DWORD exStyle = WS_EX_OVERLAPPEDWINDOW;
  static constexpr DWORD style =
      WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX | WS_MAXIMIZEBOX | WS_SIZEBOX;

  // Create main window
  mainHwnd = CreateWindowExW(exStyle, className, title, style, CW_USEDEFAULT, CW_USEDEFAULT,
                             CW_WIDTH, CW_HEIGHT, nullptr, nullptr, hInstance, nullptr);

  if (mainHwnd == nullptr) {
    return false;
  }
  ShowWindow(mainHwnd, iCmdShow);
  if (!UpdateWindow(mainHwnd)) {
    return false;
  }
  return true;
}

// Walks the wchar_t argv produced by CommandLineToArgvW and flips any of
// g_debug_mode / g_show_version / g_show_help that the user passed. Each
// flag accepts the common Win32 / Unix variants (--foo, -foo, -f, /f) so
// we work the same way from PowerShell, cmd.exe, and a Unix shell under
// Wine. Returns false only when argv itself is null (i.e. the system
// failed to split the command line), so wWinMain can give up cleanly
// before we depend on log output.
static bool ParseCommandLine(int argc, LPWSTR argv[]) {
  if (argv == nullptr) {
    return false;
  }
  bool is_debug_mode   = false;
  bool is_version_mode = false;
  bool is_help_mode    = false;
  // argv[0] is the .exe path (CommandLineToArgvW convention); skip it so
  // a path containing characters that happen to match a flag literal
  // can't false-trigger one of the wcscmp checks below.
  for (int arg_idx = 1; arg_idx < argc; ++arg_idx) {
    wchar_t* arg = argv[arg_idx];
    is_debug_mode |= (wcscmp(arg, L"--debug") == 0) || (wcscmp(arg, L"-d") == 0) ||
                     (wcscmp(arg, L"-debug") == 0) || (wcscmp(arg, L"/d") == 0) ||
                     (wcscmp(arg, L"/D") == 0);
    is_version_mode |= (wcscmp(arg, L"--version") == 0) || (wcscmp(arg, L"-v") == 0) ||
                       (wcscmp(arg, L"-ver") == 0) || (wcscmp(arg, L"/v") == 0) ||
                       (wcscmp(arg, L"/V") == 0);
    is_help_mode |= (wcscmp(arg, L"--help") == 0) || (wcscmp(arg, L"-h") == 0) ||
                    (wcscmp(arg, L"-?") == 0) || (wcscmp(arg, L"/h") == 0) ||
                    (wcscmp(arg, L"/H") == 0) || (wcscmp(arg, L"/?") == 0);
  }
  if (is_version_mode && !is_help_mode) {
    g_show_version = true;
  }
  if (is_help_mode) {
    g_show_help = true;
  }
  if (is_debug_mode) {
    g_debug_mode = true;
  }
  return true;
}

// Prints the app name + semver to the attached console (InitLogging has
// already done the AttachConsole/AllocConsole dance) and returns wWinMain's
// exit code. `system("pause")` keeps the window open when launched from
// Explorer so the user can actually read the line.
static int ShowVersionAndExit() {
  std::wcout << L"\n " << GetAppName() << L" Version " << GetVersionString() << L"\n " << std::endl;
  system("pause");
  return 0;
}

// Same as above, but for --help. Lists the recognised flags exactly as
// ParseCommandLine spells them out so the two stay in sync.
static int ShowHelpAndExit() {
  std::wcout << L"\n " << ORIG_FILENAME << L" Usage: \n" << std::flush;
  std::wostringstream wostr;
  wostr << L"   /d | -d | --debug   : Enable debug logging\n"
        << L"   /v | -v | --version : Show version info \n"
        << L"   /? | -h | --help    : Show this Help \n"
        << std::flush;
  static const std::wstring kHelpMsg = wostr.str();
  std::wcout << kHelpMsg.c_str() << std::endl;
  system("pause");
  return 0;
}

// Syncs the Dev -> Toggle Console menu entry. Greyed when no console is
// attached (or when the .rc disabled the feature); the label itself stays
// "Toggle Console" and simply flips the console's visibility when invoked.
static void UpdateConsoleToggleMenu(HWND hWnd) {
  HMENU menu = GetMenu(hWnd);
  if (menu == nullptr) {
    return;
  }
  const bool enabled = !s_console_menu_user_disabled && logging::GetIsConsoleAttached();
  EnableMenuItem(menu, IDM_CONSOLE, MF_BYCOMMAND | (enabled ? MF_ENABLED : MF_GRAYED));
}

int APIENTRY wWinMain(HINSTANCE hInstance,
                      HINSTANCE hPrevInstance,
                      LPWSTR lpCmdLine,
                      int iCmdShow) {
  UNREFERENCED_PARAMETER(hPrevInstance);
  UNREFERENCED_PARAMETER(lpCmdLine);
  g_hInstance = hInstance;

  // Initialize common controls
  INITCOMMONCONTROLSEX icex;
  icex.dwSize = sizeof(INITCOMMONCONTROLSEX);
  icex.dwICC  = ICC_STANDARD_CLASSES | ICC_BAR_CLASSES;
  InitCommonControlsEx(&icex);
  // Probe comctl32's version once for callers that gate v5.82+ behavior.
  can_use_582_controls = IsCommCtrlAtLeast(dwComCtl32TargetVer);

  static const std::wstring name   = GetAppName();
  static const LPCWSTR appTitle    = name.c_str();
  static const LPCWSTR szClassName = MAIN_WNDCLASS;

  // Parse the command line into a real argv via CommandLineToArgvW
  // (lpCmdLine is the post-exe-path tail only; we want the full thing
  // so argv[0] is the exe path that ParseCommandLine's loop skips).
  // Failure path is "no flags set" - we can't LOG(ERROR) here because
  // logging isn't initialized yet.
  int argc     = 0;
  LPWSTR* argv = CommandLineToArgvW(GetCommandLineW(), &argc);
  if (!ParseCommandLine(argc, argv)) {
    std::wcerr << L"Failed to parse command line, aborting!" << std::endl;
    return 2;
  }
  if (argv != nullptr) {
    LocalFree(argv);
  }

  // Open a conhost window when we have anything text-y to show.
  logging::LogDest kLogSink           = logging::LOG_TO_STDERR;
  static const std::wstring file_name = std::wstring(INTERNAL_NAME);
  const std::wstring kLogFile         = file_name + L".log";
  logging::LogInitSettings LoggingSettings;
  LoggingSettings.log_sink          = kLogSink;
  LoggingSettings.logfile_name      = kLogFile;
  LoggingSettings.app_name          = appTitle;
  LoggingSettings.show_func_sigs    = false;
  LoggingSettings.show_line_numbers = false;
  LoggingSettings.show_time         = false;
  LoggingSettings.full_prefix_level = LOG_ERROR;
  if (!logging::InitLogging(g_hInstance, LoggingSettings)) {
    ErrorBox(nullptr, L"Logging Initialization Failure", L"InitLogging failed!");
    return 3;
  }
  logging::SetIsDCheck(is_dcheck);
  is_on_wine = IsRunningOnWine(&winever);
  if (g_show_version) {
    return ShowVersionAndExit();
  }
  if (g_show_help) {
    return ShowHelpAndExit();
  }
  LOG(INFO) << GetWelcomeMessage();

  kMainIcon  = LoadIconW(hInstance, MAKEINTRESOURCEW(IDI_MAIN));
  kSmallIcon = LoadIconW(hInstance, MAKEINTRESOURCEW(IDI_SMALL));

  // Register our window class.
  if (!RegisterWndClass(g_hInstance, szClassName)) {
    ErrorBox(nullptr, L"RegisterClassEx Error", L"This program requires Windows NT!");
    return 1;
  }

  // We always attach a console (so the Dev -> Console menu is useful even in
  // release builds), but unless a mode wants it visible now (--debug for live
  // logging, --version / --help for one-shot output), hide it immediately so
  // a normal launch doesn't pop a console the user didn't ask for. Logs still
  // flow to the hidden stream; "Toggle Console" reveals it any time.
  if (!g_debug_mode && !g_show_version && !g_show_help) {
    if (logging::HideConsole()) {
      s_console_hidden = true;
    }
  }

  // Open our window now
  if (!InitWindow(g_hInstance, szClassName, appTitle, iCmdShow)) {
    return 4;
  }
  UpdateConsoleToggleMenu(mainHwnd);

  HACCEL hAccel = LoadAcceleratorsW(hInstance, MAKEINTRESOURCEW(IDR_MAIN));
  if (hAccel == nullptr) {
    return 5;
  }

  MSG msg;
  while (GetMessageW(&msg, nullptr, 0, 0)) {
    // Accelerator first so menu shortcuts win over dialog navigation.
    if (TranslateAcceleratorW(mainHwnd, hAccel, &msg)) {
      continue;
    }
    // Enter runs the calculation. On a non-dialog window IsDialogMessageW can't
    // route Enter to the default button (there's no registered default ID), so
    // when focus is in one of the edits we fire Calculate here. When a button
    // has focus we fall through and let IsDialogMessageW click that button.
    if (msg.message == WM_KEYDOWN && msg.wParam == VK_RETURN) {
      const HWND focused = GetFocus();
      if (focused != hCalcButton && focused != hClearButton) {
        SendMessageW(mainHwnd, WM_COMMAND, MAKEWPARAM(IDC_CALCULATE, BN_CLICKED),
                     reinterpret_cast<LPARAM>(hCalcButton));
        continue;
      }
    }
    // Dialog-style navigation: TAB / Shift+TAB between the WS_TABSTOP controls,
    // and Enter on a focused button clicks it.
    if (IsDialogMessageW(mainHwnd, &msg)) {
      continue;
    }
    TranslateMessage(&msg);
    DispatchMessageW(&msg);
  }
  if (hAccel != nullptr) {
    DestroyAcceleratorTable(hAccel);
  }
  return static_cast<int>(msg.wParam);
}

LRESULT CALLBACK WindowProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
  switch (message) {
    case WM_CREATE: {
      if (mainHwnd == nullptr) {
        mainHwnd = hWnd; // Prevent race condition in InitApp
      }
      CenterWindowOnScreen(hWnd, /*multimon=*/true);
      if (!CreateChildControls(hWnd)) {
        LOG(ERROR) << L"Failed to create child controls!";
        return -1;
      }
      LayoutChildren(hWnd);
      InitApp(hWnd);
      SetFocus(hVoltsEdit);
    } break;
    case WM_GETMINMAXINFO: {
      LPMINMAXINFO pMinMaxInfo      = reinterpret_cast<LPMINMAXINFO>(lParam);
      pMinMaxInfo->ptMinTrackSize.x = CW_MINWIDTH;
      pMinMaxInfo->ptMinTrackSize.y = CW_MINHEIGHT;
      pMinMaxInfo->ptMaxTrackSize.x = GetSystemMetrics(SM_CXMAXIMIZED);
      pMinMaxInfo->ptMaxTrackSize.y = GetSystemMetrics(SM_CYMAXIMIZED);
      break;
    }
    case WM_SIZE: {
      // cxClient / cyClient mirror the main window's client area in pixels.
      cxClient = LOWORD(lParam);
      cyClient = HIWORD(lParam);
      if (wParam != SIZE_MINIMIZED) {
        LayoutChildren(hWnd);
      }
      break;
    }
    case WM_COMMAND: {
      const int id   = LOWORD(wParam);
      const int code = HIWORD(wParam);
      // Track the last field the user typed into (Calculate's source). Ignore
      // the EN_CHANGE our own result-filling raises (s_programmatic_update),
      // and only consider the eight unit edits.
      if (code == EN_CHANGE && !s_programmatic_update && id >= IDC_VOLTS && id <= IDC_CALORIES) {
        s_last_edited = id;
        break;
      }
      switch (id) {
        case IDC_CALCULATE:
          OnCalculate(hWnd);
          break;
        case IDM_CLEAR:
        case IDC_CLEAR:
          ClearAllFields();
          break;
        case IDM_115VAC:
          SetLineVolts(hWnd, 115.0L, IDM_115VAC);
          break;
        case IDM_230VAC:
          SetLineVolts(hWnd, 230.0L, IDM_230VAC);
          break;
        case IDM_CEXIT:
          if (ConfirmExit(hWnd)) {
            ShutDownApp();
          }
          break;
        case IDM_EXIT:
          ShutDownApp();
          break;
        case IDM_ABOUT:
          DialogBoxW(g_hInstance, MAKEINTRESOURCEW(IDD_ABOUTDLG), hWnd, AboutDlgProc);
          break;
        case IDM_HELP:
          LaunchHelp(hWnd);
          break;
        case IDM_CONSOLE: {
          // Flip the console window's visibility. Greyed items shouldn't reach
          // here, but accelerators / stray WM_COMMANDs can, so re-check.
          if (s_console_menu_user_disabled) {
            break;
          }
          if (!logging::GetIsConsoleAttached()) {
            LOG(ERROR) << L"No console attached to window";
            break;
          }
          // Drive the flip off our intent bool, not IsWindowVisible (see
          // s_console_hidden's comment). Only flip the bool when the actual
          // Show/Hide call succeeds, so a failed SW_HIDE on Win11 Terminal
          // doesn't leave the state lying.
          if (s_console_hidden) {
            if (logging::ShowConsole(false)) { // false = don't steal focus
              s_console_hidden = false;
              LOG(INFO) << L"Showed console.";
            } else {
              LOG(ERROR) << L"Failed to show console!";
            }
          } else {
            if (logging::HideConsole()) {
              s_console_hidden = true;
              LOG(INFO) << L"Hid console.";
            } else {
              LOG(ERROR) << L"Failed to hide console!";
            }
          }
          UpdateConsoleToggleMenu(hWnd);
          break;
        }
        default:
          return DefWindowProcW(hWnd, message, wParam, lParam);
      }
    } break;
    case WM_HELP:
      LaunchHelp(hWnd);
      break;
    case WM_CLOSE:
      ShutDownApp();
      break;
    case WM_QUERYENDSESSION:
      LOG(DEBUG) << L"Window station is going down now!";
      return TRUE;
    case WM_DESTROY:
      if (s_uiFont != nullptr) {
        DeleteObject(s_uiFont);
        s_uiFont = nullptr;
      }
      PostQuitMessage(0); // WM_QUIT
      break;
    case WM_NCDESTROY:
      mainHwnd = nullptr;
      LOG(DEBUG) << L"Bye bye!";
      // Last message this window will receive. Close log + console cleanly
      // here, before the loop sees the WM_QUIT that WM_DESTROY queued.
      logging::DeInitLogging(g_hInstance);
      break;
    default:
      return DefWindowProcW(hWnd, message, wParam, lParam);
  }
  return 0;
}

bool InitApp(HWND hWnd) {
  if (hWnd == nullptr) {
    return false;
  }
  // Normalise the Default Line Volts radio to the .rc default (115 V) so the
  // menu shows a proper radio bullet from the first open.
  SetLineVolts(hWnd, 115.0L, IDM_115VAC);
  return true;
}

bool ShutDownApp() {
  // mainHwnd is cleared in WM_NCDESTROY; guard so a duplicate exit path
  // (e.g. WM_CLOSE arriving after WM_DESTROY's tear-down began) doesn't pass
  // NULL to DestroyWindow, which is undefined per MSDN.
  DCHECK(mainHwnd != nullptr);
  if (mainHwnd == nullptr) {
    return false;
  }
  return DestroyWindow(mainHwnd); // Send WM_DESTROY
}

bool LaunchHelp(HWND hWnd) {
  bool success = false;
  if (InfoBox(hWnd, L"Help32", L"No help yet...")) {
    success = true;
  }
  return success;
}

INT_PTR CALLBACK AboutDlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam) {
  UNREFERENCED_PARAMETER(lParam);
  switch (message) {
    case WM_INITDIALOG:
      // Set icon in titlebar of about dialog
      static const HICON kAboutIcon = LoadIconW(g_hInstance, MAKEINTRESOURCEW(IDI_ABOUT));
      SendMessageW(hDlg, WM_SETICON, ICON_SMALL, reinterpret_cast<LPARAM>(kAboutIcon));
      SendMessageW(hDlg, WM_SETICON, ICON_BIG, reinterpret_cast<LPARAM>(kAboutIcon));
      LOG(INFO) << L"Showed About dialog";
      return TRUE;
    case WM_CLOSE:
      EndDialog(hDlg, TRUE);
      return TRUE;
    case WM_COMMAND:
      if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL) {
        EndDialog(hDlg, LOWORD(wParam));
        return TRUE;
      }
      break;
    default:
      break;
  }
  return FALSE;
}

/* --------------------------- Child controls / UI -------------------------- */

// One row of the converter: a right-aligned label and the edit beside it. The
// handle slot points at the matching global HWND so CreateChildControls can
// populate it and the rest of the file can refer to the edits by name.
struct FieldRow {
  int id;
  const wchar_t* label;
  HWND* handle;
};

static const FieldRow kFieldRows[] = {
    {IDC_VOLTS,    L"Volts:",       &hVoltsEdit},
    {IDC_AMPS,     L"Amps:",        &hAmpsEdit},
    {IDC_WATTS,    L"Watts:",       &hWattsEdit},
    {IDC_IMP_HP,   L"Imperial HP:", &hImpHPEdit},
    {IDC_MET_HP,   L"Metric HP:",   &hMetHPEdit},
    {IDC_ELE_HP,   L"Electric HP:", &hEleHPEdit},
    {IDC_FT_LB,    L"Ft-Lbs/s:",    &hFtLbsEdit},
    {IDC_CALORIES, L"Calories/s:",  &hCaloriesEdit},
};
static constexpr int kNumRows = static_cast<int>(sizeof(kFieldRows) / sizeof(kFieldRows[0]));

// Label STATICs, parallel to kFieldRows; created in CreateChildControls and
// repositioned in LayoutChildren.
static HWND s_rowLabels[kNumRows] = {};

// Builds the eight label+edit rows and the centered default Calculate button.
// All edits are plain editable fields (any one can be the input); results are
// written back into the others on Calculate. Returns false if any control or
// the shared font fails to create.
static bool CreateChildControls(HWND hWnd) {
  s_uiFont = GetFont(0); // 8pt Tahoma, DPI-scaled (size <= 0 path)

  for (int i = 0; i < kNumRows; ++i) {
    s_rowLabels[i] = CreateWindowExW(0, L"STATIC", kFieldRows[i].label,
                                     WS_CHILD | WS_VISIBLE | SS_RIGHT, 0, 0, 0, 0, hWnd,
                                     reinterpret_cast<HMENU>(static_cast<UINT_PTR>(IDC_STATIC)),
                                     g_hInstance, nullptr);
    *kFieldRows[i].handle =
        CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"",
                        WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_AUTOHSCROLL, 0, 0, 0, 0, hWnd,
                        reinterpret_cast<HMENU>(static_cast<UINT_PTR>(kFieldRows[i].id)),
                        g_hInstance, nullptr);
    if (s_rowLabels[i] == nullptr || *kFieldRows[i].handle == nullptr) {
      return false;
    }
  }

  hCalcButton = CreateWindowExW(0, L"BUTTON", L"Calculate",
                                WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_DEFPUSHBUTTON, 0, 0, 0, 0,
                                hWnd, reinterpret_cast<HMENU>(static_cast<UINT_PTR>(IDC_CALCULATE)),
                                g_hInstance, nullptr);
  hClearButton = CreateWindowExW(0, L"BUTTON", L"Clear",
                                 WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_PUSHBUTTON, 0, 0, 0, 0,
                                 hWnd, reinterpret_cast<HMENU>(static_cast<UINT_PTR>(IDC_CLEAR)),
                                 g_hInstance, nullptr);
  if (hCalcButton == nullptr || hClearButton == nullptr) {
    return false;
  }

  if (s_uiFont != nullptr) {
    const WPARAM font = reinterpret_cast<WPARAM>(s_uiFont);
    for (int i = 0; i < kNumRows; ++i) {
      SendMessageW(s_rowLabels[i], WM_SETFONT, font, TRUE);
      SendMessageW(*kFieldRows[i].handle, WM_SETFONT, font, TRUE);
    }
    SendMessageW(hCalcButton, WM_SETFONT, font, TRUE);
    SendMessageW(hClearButton, WM_SETFONT, font, TRUE);
  }
  return true;
}

// Positions the rows down the top-left of the client area and pins the
// Calculate button horizontally centered near the bottom edge. Safe to call
// before the controls exist (WM_SIZE can fire early) - it no-ops until then.
static void LayoutChildren(HWND hWnd) {
  if (hCalcButton == nullptr) {
    return; // controls not created yet
  }
  RECT rc;
  GetClientRect(hWnd, &rc);
  const int clientW = rc.right - rc.left;
  const int clientH = rc.bottom - rc.top;

  const int editX = kPadLeft + kLabelWidth + kHGap;
  int editW       = clientW - editX - kPadLeft;
  if (editW < 60) {
    editW = 60;
  }

  HDWP hdwp = BeginDeferWindowPos(kNumRows * 2 + 2);
  int y     = kPadTop;
  for (int i = 0; i < kNumRows; ++i) {
    if (hdwp != nullptr) {
      // +3 nudges the label text to line up with the taller client-edge edit.
      hdwp = DeferWindowPos(hdwp, s_rowLabels[i], nullptr, kPadLeft, y + 3, kLabelWidth,
                            kControlHeight, SWP_NOZORDER);
    }
    if (hdwp != nullptr) {
      hdwp = DeferWindowPos(hdwp, *kFieldRows[i].handle, nullptr, editX, y, editW, kControlHeight,
                            SWP_NOZORDER);
    }
    y += kControlHeight + kVGap;
  }

  // Calculate + Clear side by side, centered as a group near the bottom edge.
  const int kBtnGap    = 8;
  const int groupWidth = kButtonWidth * 2 + kBtnGap;
  const int groupX     = (clientW - groupWidth) / 2;
  int btnY             = clientH - kButtonHeight - kPadTop;
  if (btnY < y + kVGap) {
    btnY = y + kVGap; // don't ride up over the last row on a short window
  }
  if (hdwp != nullptr) {
    hdwp = DeferWindowPos(hdwp, hCalcButton, nullptr, groupX, btnY, kButtonWidth, kButtonHeight,
                          SWP_NOZORDER);
  }
  if (hdwp != nullptr) {
    hdwp = DeferWindowPos(hdwp, hClearButton, nullptr, groupX + kButtonWidth + kBtnGap, btnY,
                          kButtonWidth, kButtonHeight, SWP_NOZORDER);
  }
  if (hdwp != nullptr) {
    EndDeferWindowPos(hdwp);
  }
}

// Reads an edit's text as a long double. Returns false when the field is empty
// or doesn't parse cleanly (trailing non-space garbage is rejected). Uses a
// wistringstream so we don't drag in <cwchar> just for wcstold.
static bool GetEditValue(HWND hEdit, long double* out) {
  wchar_t buf[64];
  const int len = GetWindowTextW(hEdit, buf, static_cast<int>(sizeof(buf) / sizeof(buf[0])));
  if (len <= 0) {
    return false; // empty
  }
  std::wistringstream iss(buf);
  long double value = 0.0L;
  iss >> value;
  if (iss.fail()) {
    return false; // not a number
  }
  wchar_t extra;
  if (iss >> extra) {
    return false; // trailing garbage after the number
  }
  *out = value;
  return true;
}

// Formats a value with up to 10 significant digits (defaultfloat trims
// trailing zeros, so 1.0 -> "1", 745.7 -> "745.7") and writes it to the edit
// with the programmatic-update guard raised so the EN_CHANGE it triggers isn't
// counted as a user edit.
static void SetEditValue(HWND hEdit, long double value) {
  std::wostringstream oss;
  oss << std::setprecision(10) << value;
  const std::wstring text  = oss.str();
  s_programmatic_update    = true;
  SetWindowTextW(hEdit, text.c_str());
  s_programmatic_update    = false;
}

// Settings -> Clear Results: blank every field and forget the source so the
// next Calculate has nothing stale to read. (A results .txt file is planned
// for later; for now "results" just means the on-screen fields.)
static void ClearAllFields() {
  s_programmatic_update = true;
  for (int i = 0; i < kNumRows; ++i) {
    SetWindowTextW(*kFieldRows[i].handle, L"");
  }
  s_programmatic_update = false;
  s_last_edited         = 0;
}

// Settings -> Default Line Volts: record the assumed voltage and move the
// menu radio bullet to the chosen item.
static void SetLineVolts(HWND hWnd, long double volts, UINT checkedId) {
  s_line_volts = volts;
  HMENU menu   = GetMenu(hWnd);
  if (menu != nullptr) {
    CheckMenuRadioItem(menu, IDM_115VAC, IDM_230VAC, checkedId, MF_BYCOMMAND);
  }
}

// The converter. Resolves watts (the baseline unit) from whichever field the
// user last edited, then fills every field from it. Volts/Amps are special:
// Volts+Amps -> V x A; Amps alone -> s_line_volts x A; Volts alone is
// incomplete (can't make power from voltage alone). A power value backs Amps
// out through the assumed line voltage.
static void OnCalculate(HWND hWnd) {
  if (s_last_edited == 0) {
    MessageBeep(MB_OK); // nothing typed yet
    return;
  }

  long double watts      = 0.0L;
  long double volts_used = s_line_volts;
  bool have_watts        = false;

  switch (s_last_edited) {
    case IDC_VOLTS: {
      long double volts = 0.0L, amps = 0.0L;
      if (GetEditValue(hVoltsEdit, &volts) && GetEditValue(hAmpsEdit, &amps)) {
        volts_used = volts;
        watts      = ConvWatts(volts, amps);
        have_watts = true;
      } else {
        WarnBox(hWnd, L"Incomplete Input",
                L"Volts alone can't be converted - also enter Amps, or type a power value.");
        return;
      }
      break;
    }
    case IDC_AMPS: {
      long double amps = 0.0L;
      if (GetEditValue(hAmpsEdit, &amps)) {
        long double volts = 0.0L;
        // Both V and A given -> use the typed volts; amps alone -> assume line.
        volts_used = GetEditValue(hVoltsEdit, &volts) ? volts : s_line_volts;
        watts      = ConvWatts(volts_used, amps);
        have_watts = true;
      }
      break;
    }
    case IDC_WATTS: {
      long double v = 0.0L;
      if (GetEditValue(hWattsEdit, &v)) {
        watts      = v;
        have_watts = true;
      }
      break;
    }
    case IDC_IMP_HP: {
      long double v = 0.0L;
      if (GetEditValue(hImpHPEdit, &v)) {
        watts      = ConvWattsImpHp(v);
        have_watts = true;
      }
      break;
    }
    case IDC_MET_HP: {
      long double v = 0.0L;
      if (GetEditValue(hMetHPEdit, &v)) {
        watts      = ConvWattsMetHp(v);
        have_watts = true;
      }
      break;
    }
    case IDC_ELE_HP: {
      long double v = 0.0L;
      if (GetEditValue(hEleHPEdit, &v)) {
        watts      = ConvWattsElecHp(v);
        have_watts = true;
      }
      break;
    }
    case IDC_FT_LB: {
      long double v = 0.0L;
      if (GetEditValue(hFtLbsEdit, &v)) {
        watts      = ConvWattsFtLbs(v);
        have_watts = true;
      }
      break;
    }
    case IDC_CALORIES: {
      long double v = 0.0L;
      if (GetEditValue(hCaloriesEdit, &v)) {
        watts      = ConvWattsCalories(v);
        have_watts = true;
      }
      break;
    }
    default:
      break;
  }

  if (!have_watts) {
    MessageBeep(MB_ICONWARNING); // source field empty or unparsable
    return;
  }
  // Guard the divide-by-zero when backing out amps (only an explicit 0 volts
  // could reach here; s_line_volts is always > 0).
  if (volts_used <= 0.0L) {
    volts_used = s_line_volts;
  }

  SetEditValue(hVoltsEdit, volts_used);
  SetEditValue(hAmpsEdit, watts / volts_used);
  SetEditValue(hWattsEdit, watts);
  SetEditValue(hImpHPEdit, ConvImperialHorsepower(watts));
  SetEditValue(hMetHPEdit, ConvMetricHorsepower(watts));
  SetEditValue(hEleHPEdit, ConvElectricHorsepower(watts));
  SetEditValue(hFtLbsEdit, ConvFootPounds(watts));
  SetEditValue(hCaloriesEdit, ConvCalories(watts));
}
