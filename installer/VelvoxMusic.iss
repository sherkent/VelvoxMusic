; Inno Setup script for Velvox Music (Windows)
; Build the Release target first, then compile this script with Inno Setup 6.

#define MyAppName "Velvox Music"
#define MyAppVersion "0.1.0"
#define MyAppPublisher "Velvox Music contributors"
#define MyAppURL "https://github.com/VeilCode/VelvoxMusic"
#define MyAppExeName "VelvoxMusic.exe"

[Setup]
AppId={{F4C1D5E2-9B3A-4E7C-8F1D-2A6B9C5E3D71}
AppName={#MyAppName}
AppVersion={#MyAppVersion}
AppPublisher={#MyAppPublisher}
AppPublisherURL={#MyAppURL}
AppSupportURL={#MyAppURL}/issues
AppUpdatesURL={#MyAppURL}/releases
DefaultDirName={autopf}\{#MyAppName}
DisableProgramGroupPage=yes
LicenseFile=..\LICENSE
OutputDir=..\dist
OutputBaseFilename=VelvoxMusic-{#MyAppVersion}-win64-setup
; To show a custom icon in the installer and the Windows shell, create a
; Windows .ico from resources/icons/app-icon.svg and point SetupIconFile to it.
; SetupIconFile=..\resources\icons\app-icon.ico
Compression=lzma2
SolidCompression=yes
WizardStyle=modern
PrivilegesRequired=lowest
PrivilegesRequiredOverridesAllowed=dialog
UninstallDisplayIcon={app}\{#MyAppExeName}

[Languages]
Name: "english"; MessagesFile: "compiler:Default.isl"

[Tasks]
Name: "desktopicon"; Description: "{cm:CreateDesktopIcon}"; GroupDescription: "{cm:AdditionalIcons}"; Flags: unchecked

[Files]
; The entire Release build output (VelvoxMusic.exe + windeployqt-deployed Qt
; libraries + TagLib DLL + Qt plugins/qml/translations if present).
Source: "..\build\Release\*"; DestDir: "{app}"; Excludes: "*.pdb,*.ilk,*.exp"; Flags: ignoreversion recursesubdirs createallsubdirs
Source: "..\LICENSE"; DestDir: "{app}"; Flags: ignoreversion
Source: "..\THIRD_PARTY_LICENSES.md"; DestDir: "{app}"; Flags: ignoreversion

[Icons]
Name: "{autoprograms}\{#MyAppName}"; Filename: "{app}\{#MyAppExeName}"
Name: "{autodesktop}\{#MyAppName}"; Filename: "{app}\{#MyAppExeName}"; Tasks: desktopicon

[Run]
Filename: "{app}\{#MyAppExeName}"; Description: "{cm:LaunchProgram,{#StringChange(MyAppName, '&', '&&')}}"; Flags: nowait postinstall skipifsilent

[UninstallDelete]
Type: filesandordirs; Name: "{app}"
