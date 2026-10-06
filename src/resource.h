// {{NO_DEPENDENCIES}}
// For #define-ing static resources for resource script file(s).
// Used by torqalc.rc
//
// ID range map (keep new entries inside their range):
//   100-119  IDI_*  Icons
//   120-129  IDR_*  Main program resource (menu / accelerators)
//   130-149  IDD_*  Dialogs
//   200-299  IDM_*  Menu commands
//   300-399  IDC_*  Child control IDs, grouped by logical role

// clang-format off

/* Icons (100-119) */
#define IDI_MAIN                101 /* 32x32 & 48x48 icon */
#define IDI_SMALL               102 /* Small 16x16 icon */
#define IDI_ABOUT               103 /* About Dialog icon */

/* Main application resource - also used to attach menu */
#define IDR_MAIN                120

/* Dialogs (130-149) */
#define IDD_ABOUTDLG            130 // About Dialog

/* Menu items (200-299) */
#define IDM_ABOUT               200 // About dialog
#define IDM_EXIT                201 // Exit immediately
#define IDM_CEXIT               202 // Exit confirmation
#define IDM_HELP                203 // Opens help
//#define IDM_SAVEAS              204 // Saves results as .txt (future)
#define IDM_CONSOLE             205 // Toggle console window
#define IDM_CLEAR               206 // Clear results
#define IDM_115VAC              207 // For changing default assumed volts (115 AC) when calculating from watts/horsepower
#define IDM_230VAC              208 // For changing default assumed volts (230V AC) when calculating from watts/horsepower

/* Child control IDs (300-399) */
#define IDC_VOLTS               300 // Voltage input/output
#define IDC_AMPS                301 // Amperage input/output
#define IDC_WATTS               302 // Watts input/output
#define IDC_IMP_HP              303 // Imperial Horsepower
#define IDC_MET_HP              304 // Metric Horsepower
#define IDC_ELE_HP              305 // Electric Horsepower
#define IDC_FT_LB               306 // Foot-pounds
#define IDC_CALORIES            307 // Calories
#define IDC_CALCULATE           308 // Calculate button
#define IDC_CLEAR               309 // Clear button

// For resources to be loaded without an ID from the system.
#ifndef IDC_STATIC
 #define IDC_STATIC             -1
#endif // IDC_STATIC

// clang-format on
