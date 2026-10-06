; Neko-ng Inno Setup Script File
; Only for Inno Setup 5.x to support Windows 2000/XP
; Tested with ISS 5.6.1

#define AppVer "0.1.0"
#define AppName "Torqalc"
#define ExeName "torqalc"
#define Developer "Alex313031"
#define CopyRightYear "© 2026"
#define GitURL "https://github.com/Alex313031/torqalc"

[Setup]
MinVersion=5.0
AppName={#AppName}
AppVersion={#AppVer}
AppVerName={#AppName} v{#AppVer}
OutputBaseFilename={#ExeName}_{#AppVer}_setup
UninstallDisplayName={#AppName} {#AppVer}
DefaultDirName={pf}\{#AppName}
DefaultGroupName={#AppName}
UninstallDisplayIcon={app}\{#ExeName}.exe
DisableWelcomePage=no
AlwaysShowDirOnReadyPage=yes
Compression=lzma
SolidCompression=yes
VersionInfoVersion={#AppVer}
AppPublisher={#Developer}
AppPublisherURL={#GitURL}
AppSupportURL={#GitURL}/#readme
AppUpdatesURL={#GitURL}/releases
VersionInfoCompany={#Developer}
AppCopyright={#CopyRightYear}
VersionInfoCopyright={#CopyRightYear} {#Developer}
VersionInfoProductName={#AppName}
InfoBeforeFile="assets\Readme.txt"
SetupIconFile="src\res\main.ico"
WizardImageFile="assets\installer_background.bmp"
;WizardStyle=modern
UseSetupLdr=yes

[Files]
Source: "release\{#ExeName}.exe"; DestDir: "{app}"

[Icons]
Name: "{group}\{#AppName}"; Filename: "{app}\{#ExeName}.exe"

[Run]
Filename: "{app}\{#ExeName}.exe"; Description: "Launch app when finished"; Flags: postinstall nowait skipifsilent
