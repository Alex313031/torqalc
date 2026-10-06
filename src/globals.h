#ifndef TORQALC_GLOBALS_H_
#define TORQALC_GLOBALS_H_

#include "framework.h"

// Main client width/height
extern int cxClient;
extern int cyClient;

extern HINSTANCE g_hInstance; // This program instance, everything descends from this

extern HWND mainHwnd; // Our main window handle

extern HWND hVoltsEdit;     // Volts
extern HWND hAmpsEdit;      // Amps
extern HWND hWattsEdit;     // Watts
extern HWND hImpHPEdit;     // Imperial Horsepower
extern HWND hMetHPEdit;     // Metric Horsepower
extern HWND hEleHPEdit;     // Electric Horsepower
extern HWND hFtLbsEdit;     // Foot-pounds
extern HWND hCaloriesEdit;  // Calories
extern HWND hCalcButton;    // "Calculate" button
extern HWND hClearButton;   // "Clear" button

extern bool can_use_582_controls; // Whether we can use "modern" common controls from XP+

extern bool is_on_wine;

#endif // TORQALC_GLOBALS_H_
