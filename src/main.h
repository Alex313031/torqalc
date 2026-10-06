#ifndef TORQALC_MAIN_H_
#define TORQALC_MAIN_H_

#include "framework.h"
#include "utils.h"

// Registers our window class, one of the first things to run.
bool RegisterWndClass(HINSTANCE hInstance, LPCWSTR className);

// Creates the main window and shows it
bool InitWindow(HINSTANCE hInstance, LPCWSTR className, LPCWSTR title, int iCmdShow);

// Main window procedure
LRESULT CALLBACK WindowProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);

// Initializes app state
bool InitApp(HWND hWnd);

// Closes all windows and cleans up any resources. True means a clean shutdown
bool ShutDownApp();

// Shows help
bool LaunchHelp(HWND hWnd);

// About dialog handler
INT_PTR CALLBACK AboutDlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);

#endif // TORQALC_MAIN_H_
