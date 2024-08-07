;.,.,.,.,.,.,.,.,.,.,.,.,.,.,.,.,.,.,.,.,.,.,.,.,.,.,.,.,.,.,.,.,.,.,.,.,
;                                                                       ;
;Copyright (c) 2002-2010 Jernej Simončič                                ;
;                                                                       ;
;This software is provided 'as-is', without any express or implied      ;
;warranty. In no event will the authors be held liable for any damages  ;
;arising from the use of this software.                                 ;
;                                                                       ;
;Permission is granted to anyone to use this software for any purpose,  ;
;including commercial applications, and to alter it and redistribute it ;
;freely, subject to the following restrictions:                         ;
;                                                                       ;
;   1. The origin of this software must not be misrepresented; you must ;
;      not claim that you wrote the original software. If you use this  ;
;      software in a product, an acknowledgment in the product          ;
;      documentation would be appreciated but is not required.          ;
;                                                                       ;
;   2. Altered source versions must be plainly marked as such, and must ;
;      not be misrepresented as being the original software.            ;
;                                                                       ;
;   3. This notice may not be removed or altered from any source        ;
;      distribution.                                                    ;
;.,.,.,.,.,.,.,.,.,.,.,.,.,.,.,.,.,.,.,.,.,.,.,.,.,.,.,.,.,.,.,.,.,.,.,.;
;
;Install script for GIMP and GTK+
;requires Inno Setup 6
;
;See base_directories.isi
;
;Changelog:
;
;2012-05-05
;- check for SSE support
;- remove obsolete 2.6 plugins when installing over 2.6.12 combined installer
;
;2011-12-18
;- display a picture on the first install screen
;- add a development version warning back
;- clean gegl's DLLs on install as some files have changed names
;
;2011-08-30
;- only uninstall previous GIMP version when installing over existing installation
;  TODO: offer the option to uninstall 32bit version when installing on x64 system
;- install 32bit plugins to same directory as 64bit plugins on x64 installs to
;  avoid problems when upgrading
;
;2010-07-08
;- clean up entire Python subdirectory (since the .pyc files are generated when
;  scripts are run)
;- use crHand instead of crHandPoint for the billboard URL
;
;2010-07-02
;- add libraries for compatibility with old 32-bit plug-ins as a component
;- remove a few unused RTF ready-memo related things
;- uninst.inf is now processed as the first step of uninstall, as otherwise the
;  uninstaller could leave behind empty directories
;
;2010-06-29
;- fix SuppressibleMsgBox calls to use proper button ID for default button
;- simplify the wizard - skip Welcome page, and make the Next button from
;  InfoBefore/License page invoke install immediately (custom install is still
;  possible by clicking the Customize button)
;
;2010-05-15
;- rewrote script mostly from scratch
;- combine 32 and 64bit GIMP versions to a single installer
;  - install enough 32bit support files even with 64bit version to allow running 32bit
;    plug-ins on 64bit version (used by Python scriptin support [as there's no 64-bit
;    PyGTK+ on Windows available yet] and TWAIN plug-in, which only works in 32-bit
;    version)
;- Python with PyGTK is included in the installer now
;- install GIMP to new directory by default ({pf}\GIMP 2 instead of {pf}\GIMP-2.0)
;- uninstall previous GIMP versions as the first step of install (both 32 and 64-bit)
;  - require reboot if installing to directory from which GIMP was just uninstalled,
;    and this directory wasn't removed by the uninstaller; the installer will continue
;    automatically after reboot
;- fixed a long standing bug where "Open with GIMP" menu entries would be left after
;  uninstalling
;
#pragma option -e+


; Optional Build-time params: DEBUG_SYMBOLS, LUA, PYTHON, NOCOMPRESSION, NOFILES
; Optional Run-time params: /configoverride= /disablecheckupdate=false|true /debugresume=0|1 /resumeinstall=0|1|2

#define X86 1
#define X64 2
#define ARM64 3

#define GIMP_DIR32 GIMP_DIR + "\" + DIR32
#define GIMP_DIR64 GIMP_DIR + "\" + DIR64
#define GIMP_DIRA64 GIMP_DIR + "\" + DIR64

#define DDIR32 DIR32
#define DDIR64 DIR64
#define DDIRA64 DIRA64

#define DEPS_DIR32 DEPS_DIR + "\" + DDIR32
#define DEPS_DIR64 DEPS_DIR + "\" + DDIR64
#define DEPS_DIRA64 DEPS_DIR + "\" + DDIRA64


#define MAJOR=Copy(GIMP_VERSION,1,Pos(".",GIMP_VERSION)-1)
#define MINOR=Copy(GIMP_VERSION,Pos(".",GIMP_VERSION)+1)
#expr MINOR=Copy(MINOR,1,Pos(".",MINOR)-1)
#if Int(MINOR) % 2 == 1
 #define GIMP_UNSTABLE="-dev"
#endif

[Setup]
MissingMessagesWarning=no
NotRecognizedMessagesWarning=no
#define ASSETS_DIR BUILD_DIR + "\build\windows\installer"

;INSTALLER AND APP INFO
AppName=GIMP
#if Defined(GIMP_UNSTABLE) && GIMP_UNSTABLE != ""
	;Inno installer identifier: https://github.com/jrsoftware/issrc/pull/461
	SetupMutex=GIMP-{#GIMP_APP_VERSION}
	;Inno installer (default) install location
	DefaultDirName={autopf}\GIMP {#GIMP_APP_VERSION}
	;Inno uninstaller identifier
	AppID=GIMP-{#GIMP_APP_VERSION}
#else
	SetupMutex=GIMP-{#MAJOR}
	DefaultDirName={autopf}\GIMP {#MAJOR}
	AppID=GIMP-{#MAJOR}
#endif

#if !defined(REVISION) || REVISION == "0"
	;Inno installer file version
	VersionInfoVersion={#GIMP_VERSION}
	;Inno installer product version and ImmersiveControlPanel 'DisplayVersion'
	AppVersion={#GIMP_VERSION}.0
	;ImmersiveControlPanel 'DisplayName'
	AppVerName=GIMP {#GIMP_VERSION}
#else
	VersionInfoVersion={#GIMP_VERSION}.{#REVISION}
	AppVersion={#GIMP_VERSION}.{#REVISION}
	AppVerName=GIMP {#GIMP_VERSION}.{#REVISION}
#endif

;ImmersiveControlPanel 'Publisher'
AppPublisher=The GIMP Team
;ControlPanel 'URLInfoAbout'
AppPublisherURL=https://www.gimp.org/
;ControlPanel 'HelpLink'
AppSupportURL=https://www.gimp.org/docs/
#if Defined(GIMP_UNSTABLE) && GIMP_UNSTABLE != ""
	;ControlPanel 'URLUpdateInfo'
	AppUpdatesURL=https://www.gimp.org/downloads/devel/
#else
	AppUpdatesURL=https://www.gimp.org/downloads/
#endif

;INSTALLER PAGES
PrivilegesRequiredOverridesAllowed=dialog
ShowLanguageDialog=auto
DisableWelcomePage=no
InfoBeforeFile=gpl+python.rtf
DisableDirPage=auto
FlatComponentsList=yes
DisableProgramGroupPage=yes
;AllowNoIcons=true
ChangesAssociations=true
ChangesEnvironment=yes
AlwaysShowDirOnReadyPage=yes

WizardStyle=modern
WizardSizePercent=100
WizardResizable=no
WizardSmallImageFile={#ASSETS_DIR}\gimp.scale-100.bmp,{#ASSETS_DIR}\gimp.scale-125.bmp,{#ASSETS_DIR}\gimp.scale-150.bmp,{#ASSETS_DIR}\gimp.scale-175.bmp,{#ASSETS_DIR}\gimp.scale-200.bmp,{#ASSETS_DIR}\gimp.scale-225.bmp,{#ASSETS_DIR}\gimp.scale-250.bmp
WizardImageFile={#ASSETS_DIR}\install-end.scale-100.bmp,{#ASSETS_DIR}\install-end.scale-125.bmp,{#ASSETS_DIR}\install-end.scale-150.bmp,{#ASSETS_DIR}\install-end.scale-175.bmp,{#ASSETS_DIR}\install-end.scale-200.bmp,{#ASSETS_DIR}\install-end.scale-225.bmp,{#ASSETS_DIR}\install-end.scale-250.bmp
WizardImageStretch=yes


;INSTALLER .EXE FILE
SetupIconFile={#ASSETS_DIR}\setup.ico
ArchitecturesInstallIn64BitMode=x64 arm64
MinVersion=10.0

#if Defined(GIMP_UNSTABLE) && GIMP_UNSTABLE != ""
LZMANumBlockThreads=4
LZMABlockSize=76800
#endif
#ifdef NOCOMPRESSION
;UseSetupLdr=no
OutputDir={#GIMP_DIR}\unc
Compression=none
;InternalCompressLevel=0
DiskSpanning=yes
DiskSliceSize=max
#else
OutputDir={#GIMP_DIR}
Compression=lzma2/ultra64
InternalCompressLevel=ultra
SolidCompression=yes
LZMAUseSeparateProcess=yes
LZMANumFastBytes=273
; Increasing dictionary size may improve compression ratio at the
; expense of memory requirement. We run into "Out of memory" error in
; the CI.
;LZMADictionarySize=524288

;SignTool=Default
;SignedUninstaller=yes
;SignedUninstallerDir=_Uninst
#endif //NOCOMPRESSION

#if !defined(REVISION) || REVISION == "0"
OutputBaseFileName=gimp-{#GIMP_VERSION}-setup
#else
OutputBaseFileName=gimp-{#GIMP_VERSION}.{#REVISION}-setup
#endif
OutputManifestFile=inno.log


;UNINSTALLER
;ImmersiveControlPanel 'DisplayIcon'
UninstallDisplayIcon={app}\bin\gimp-{#GIMP_APP_VERSION}.exe
UninstallFilesDir={app}\uninst


[LangOptions]
;Win32 Vista design (despite the name it is still used in Win32 Win 11 apps)
DialogFontName=Segoe UI
DialogFontSize=9
WelcomeFontName=Segoe UI
WelcomeFontSize=12

[Languages]
Name: "en"; MessagesFile: "compiler:Default.isl,{#ASSETS_DIR}\lang\en.setup.isl"
#include ASSETS_DIR + "\base_po-msg.list"


[Types]
;Name: normal; Description: "{cm:TypeTypical}"
Name: full; Description: "{cm:TypeFull}"
Name: compact; Description: "{cm:TypeCompact}"
Name: custom; Description: "{cm:TypeCustom}"; Flags: iscustom

[Components]
;Required components (minimal install)
Name: gimp32; Description: "{cm:ComponentsGimp,{#GIMP_VERSION}}"; Types: full compact custom; Flags: fixed; Check: Check3264('32')
Name: gimp64; Description: "{cm:ComponentsGimp,{#GIMP_VERSION}}"; Types: full compact custom; Flags: fixed; Check: Check3264('x64')
Name: gimpARM64; Description: "{cm:ComponentsGimp,{#GIMP_VERSION}}"; Types: full compact custom; Flags: fixed; Check: Check3264('arm64')

Name: deps32; Description: "{cm:ComponentsDeps}"; Types: full compact custom; Flags: fixed; Check: Check3264('32')
Name: deps64; Description: "{cm:ComponentsDeps}"; Types: full compact custom; Flags: fixed; Check: Check3264('x64')
Name: depsARM64; Description: "{cm:ComponentsDeps}"; Types: full compact custom; Flags: fixed; Check: Check3264('arm64')

;Optional components (complete install)
#ifdef DEBUG_SYMBOLS
Name: debug32; Description: "{cm:ComponentsDebug}"; Types: full custom; Flags: disablenouninstallwarning; Check: Check3264('32')
Name: debug64; Description: "{cm:ComponentsDebug}"; Types: full custom; Flags: disablenouninstallwarning; Check: Check3264('x64')
Name: debugARM64; Description: "{cm:ComponentsDebug}"; Types: full custom; Flags: disablenouninstallwarning; Check: Check3264('arm64')
#endif

;Ghostscript
Name: gs32; Description: "{cm:ComponentsGhostscript}"; Types: full custom; Check: Check3264('32')
Name: gs64; Description: "{cm:ComponentsGhostscript}"; Types: full custom; Check: Check3264('x64')
Name: gsARM64; Description: "{cm:ComponentsGhostscript}"; Types: full custom; Check: Check3264('arm64')

#ifdef LUA
Name: lua32; Description: "{cm:ComponentsLua}"; Types: full custom; Check: Check3264('32')
Name: lua64; Description: "{cm:ComponentsLua}"; Types: full custom; Check: Check3264('x64')
Name: luaARM64; Description: "{cm:ComponentsLua}"; Types: full custom; Check: Check3264('arm64')
#endif

#ifdef PYTHON
Name: py32; Description: "{cm:ComponentsPython}"; Types: full custom; Check: Check3264('32')
Name: py64; Description: "{cm:ComponentsPython}"; Types: full custom; Check: Check3264('x64')
Name: pyARM64; Description: "{cm:ComponentsPython}"; Types: full custom; Check: Check3264('arm64')
#endif

;Locales
Name: loc; Description: "{cm:ComponentsTranslations}"; Types: full custom
#include ASSETS_DIR + "\base_po-cmp.list"

;MyPaint Brushes
Name: mypaint; Description: "{cm:ComponentsMyPaint}"; Types: full custom

;32-bit TWAIN support
Name: gimp32on64; Description: "{cm:ComponentsGimp32}"; Types: full custom; Flags: checkablealone; Check: Check3264('64')


[Tasks]
Name: desktopicon; Description: "{cm:AdditionalIconsDesktop}"; GroupDescription: "{cm:AdditionalIcons}"

[Icons]
Name: "{autoprograms}\GIMP {#GIMP_VERSION}"; Filename: "{app}\bin\gimp-{#GIMP_APP_VERSION}.exe"; WorkingDir: "%USERPROFILE%"; Comment: "GIMP {#GIMP_VERSION}"
Name: "{autodesktop}\GIMP {#GIMP_VERSION}"; Filename: "{app}\bin\gimp-{#GIMP_APP_VERSION}.exe"; WorkingDir: "%USERPROFILE%"; Comment: "GIMP {#GIMP_VERSION}"; Tasks: desktopicon

[Run]
Filename: "{app}\bin\gimp-{#GIMP_APP_VERSION}.exe"; Description: "{cm:LaunchGimp}"; Flags: unchecked postinstall nowait skipifsilent


[Files]
;setup files
Source: "{#ASSETS_DIR}\install-end.scale-100.bmp"; Flags: dontcopy
Source: "{#ASSETS_DIR}\installsplash.bmp"; Flags: dontcopy
Source: "{#ASSETS_DIR}\installsplash_small.bmp"; Flags: dontcopy

#ifndef NOFILES
#define COMMON_FLAGS="recursesubdirs restartreplace uninsrestartdelete ignoreversion"

;Required arch-neutral components (compact install)
#define GIMP_ARCHS="gimp32 or gimp64 or gimpARM64"
#define OPTIONAL_EXT="*.debug,*.lua,*.py"
Source: "{#GIMP_DIR32}\etc\gimp\*"; DestDir: "{app}\etc\gimp"; Components: {#GIMP_ARCHS}; Flags: {#COMMON_FLAGS}
Source: "{#GIMP_DIR32}\lib\gimp\{#GIMP_API_VERSION}\environ\default.env"; DestDir: "{app}\lib\gimp\{#GIMP_API_VERSION}\environ"; Components: {#GIMP_ARCHS}; Flags: {#COMMON_FLAGS}
Source: "{#GIMP_DIR32}\lib\gimp\{#GIMP_API_VERSION}\interpreters\gimp-script-fu-interpreter.interp"; DestDir: "{app}\lib\gimp\{#GIMP_API_VERSION}\interpreters"; Components: {#GIMP_ARCHS}; Flags: {#COMMON_FLAGS}
Source: "{#GIMP_DIR32}\lib\gimp\{#GIMP_API_VERSION}\extensions\*"; DestDir: "{app}\lib\gimp\{#GIMP_API_VERSION}\extensions"; Excludes: "*.dll,*.exe,{#OPTIONAL_EXT}"; Components: {#GIMP_ARCHS}; Flags: {#COMMON_FLAGS}
Source: "{#GIMP_DIR32}\lib\gimp\{#GIMP_API_VERSION}\plug-ins\*"; DestDir: "{app}\lib\gimp\{#GIMP_API_VERSION}\plug-ins"; Excludes: "*.dll,*.exe,{#OPTIONAL_EXT}"; Components: {#GIMP_ARCHS}; Flags: {#COMMON_FLAGS}
Source: "{#GIMP_DIR32}\share\gimp\*"; DestDir: "{app}\share\gimp"; Excludes: "{#OPTIONAL_EXT}"; Components: {#GIMP_ARCHS}; Flags: {#COMMON_FLAGS} createallsubdirs
Source: "{#GIMP_DIR32}\share\icons\hicolor\*"; DestDir: "{app}\share\icons\hicolor"; Components: {#GIMP_ARCHS}; Flags: {#COMMON_FLAGS}
Source: "{#GIMP_DIR32}\share\metainfo\*"; DestDir: "{app}\share\metainfo"; Components: {#GIMP_ARCHS}; Flags: {#COMMON_FLAGS}

#define DEPS_ARCHS="deps32 or deps64 or depsARM64"
Source: "{#DEPS_DIR32}\etc\*"; DestDir: "{app}\etc"; Excludes: "gimp"; Components: {#DEPS_ARCHS}; Flags: {#COMMON_FLAGS}
Source: "{#DEPS_DIR32}\share\*"; DestDir: "{app}\share"; Excludes: "gimp,icons\hicolor,metainfo,locale\*,mypaint-data"; Components: {#DEPS_ARCHS}; Flags: {#COMMON_FLAGS} createallsubdirs

;Optional arch-neutral components (full install)
#ifdef LUA
Source: "{#GIMP_DIR32}\lib\gimp\{#GIMP_API_VERSION}\interpreters\lua.interp"; DestDir: "{app}\lib\gimp\{#GIMP_API_VERSION}\interpreters"; Components: (lua32 or lua64 or luaARM64); Flags: {#COMMON_FLAGS}
Source: "{#GIMP_DIR32}\*.lua"; DestDir: "{app}"; Components: (lua32 or lua64 or luaARM64); Flags: {#COMMON_FLAGS}
#endif
#ifdef PYTHON
Source: "{#GIMP_DIR32}\lib\gimp\{#GIMP_API_VERSION}\environ\py*.env"; DestDir: "{app}\lib\gimp\{#GIMP_API_VERSION}\environ"; Components: (py32 or py64 or pyARM64); Flags: {#COMMON_FLAGS}
Source: "{#GIMP_DIR32}\lib\gimp\{#GIMP_API_VERSION}\interpreters\pygimp.interp"; DestDir: "{app}\lib\gimp\{#GIMP_API_VERSION}\interpreters"; Components: (py32 or py64 or pyARM64); Flags: {#COMMON_FLAGS}
Source: "{#GIMP_DIR32}\*.py"; DestDir: "{app}"; Components: (py32 or py64 or pyARM64); Flags: {#COMMON_FLAGS}
#endif
Source: "{#GIMP_DIR32}\share\locale\*"; DestDir: "{app}\share\locale"; Components: loc; Flags: dontcopy {#COMMON_FLAGS}
#include ASSETS_DIR + "\base_po-files.list"
Source: "{#DEPS_DIR32}\share\mypaint-data\*"; DestDir: "{app}\share\mypaint-data"; Components: mypaint; Flags: {#COMMON_FLAGS}

;files arch specific
;i686
#define PLATFORM X86
#include "base_executables.isi"
;special case, since 64bit version doesn't work, and is excluded in base_executables.isi
Source: "{#GIMP_DIR32}\lib\gimp\{#GIMP_API_VERSION}\plug-ins\twain.exe"; DestDir: "{app}\lib\gimp\{#GIMP_API_VERSION}\plug-ins"; Components: gimp32; Flags: {#COMMON_FLAGS}

;x86_64
#define PLATFORM X64
#include "base_executables.isi"

;AArch64
#define PLATFORM ARM64
#include "base_executables.isi"

;32-on-64bit
#include "base_twain32on64.isi"
;prefer 32bit twain plugin over 64bit because 64bit twain drivers are rare
Source: "{#GIMP_DIR32}\lib\gimp\{#GIMP_API_VERSION}\plug-ins\twain\twain.exe"; DestDir: "{app}\lib\gimp\{#GIMP_API_VERSION}\plug-ins\twain"; Components: gimp32on64; Flags: {#COMMON_FLAGS}

;upgrade zlib1.dll in System32 if it's present there to avoid breaking plugins
;sharedfile flag will ensure that the upgraded file is left behind on uninstall to avoid breaking other programs that use the file
Source: "{#DEPS_DIR32}\bin\zlib1.dll"; DestDir: "{sys}"; Components: {#GIMP_ARCHS}; Flags: restartreplace sharedfile 32bit uninsrestartdelete comparetimestamp; Check: BadSysDLL('zlib1.dll',32)
Source: "{#DEPS_DIR64}\bin\zlib1.dll"; DestDir: "{sys}"; Components: gimp64; Flags: restartreplace sharedfile uninsrestartdelete comparetimestamp; Check: BadSysDLL('zlib1.dll',64)

;allow specific configuration files to be overridden by files in a specific directory
#define FindHandle

#sub ProcessConfigFile
  #define FileName FindGetFileName(FindHandle)
Source: "{code:GetExternalConfDir}\{#FileName}"; DestDir: "{app}\{#ConfigDir}"; Flags: external restartreplace; Check: CheckExternalConf('{#FileName}')
  #if BaseDir != GIMP_DIR32
Source: "{code:GetExternalConfDir}\{#FileName}"; DestDir: "{app}\32\{#ConfigDir}"; Components: gimp32on64; Flags: external restartreplace; Check: CheckExternalConf('{#FileName}')
  #endif
#endsub

#define FindResult
#sub ProcessConfigDir
  #emit ';; ' + ConfigDir
  #emit ';; ' + BaseDir
  #for {FindHandle = FindResult = FindFirst(AddBackslash(BaseDir) + AddBackSlash(ConfigDir) + "*", 0); \
      FindResult; FindResult = FindNext(FindHandle)} ProcessConfigFile
  #if FindHandle
    #expr FindClose(FindHandle)
  #endif
#endsub

#define public BaseDir GIMP_DIR32
#define public ConfigDir "etc\gimp\" + GIMP_API_VERSION
#expr ProcessConfigDir

#define public ConfigDir "etc\fonts"
#expr ProcessConfigDir

#endif //NOFILES

;We need at least an empty folder to avoid GIMP*_LOCALEDIR warnings
[Dirs]
Name: "{app}\32\share\locale"; Components: gimp32on64; Flags: uninsalwaysuninstall


[InstallDelete]
Type: files; Name: "{app}\bin\gimp-?.?.exe"
Type: files; Name: "{app}\bin\gimp-?.??.exe"
Type: files; Name: "{app}\bin\gimp-console-?.?.exe"
Type: files; Name: "{app}\bin\gimp-console-?.??.exe"
;old ghostscript
Type: filesandordirs; Name: "{app}\share\ghostscript\*"
;<2.99.14 plug-ins and modules
Type: files; Name: "{app}\lib\gimp\2.99\plug-ins\py-slice\py-slice.py"
Type: files; Name: "{app}\lib\gimp\2.99\plug-ins\benchmark-foreground-extract\benchmark-foreground-extract.py"
;Some typo in meson which we used for GIMP 2.99.12 installer.
Type: files; Name: "{app}\lib\gimp\2.99\modules\libcontroller-dx-input.dll"
;old icons
#ifndef GIMP_UNSTABLE
Type: files; Name: "{autoprograms}\GIMP {#MAJOR}.lnk"
Type: files; Name: "{autodesktop}\GIMP {#MAJOR}.lnk"
#endif
;get previous GIMP icon name from uninstall name in Registry
#if Defined(GIMP_UNSTABLE) && GIMP_UNSTABLE != ""
Type: files; Name: "{autoprograms}\GIMP {reg:HKA\SOFTWARE\Microsoft\Windows\CurrentVersion\Uninstall\GIMP-{#GIMP_APP_VERSION}_is1,DisplayVersion|GIMP {#GIMP_APP_VERSION}}.lnk"; Check: CheckRegValueExists('SOFTWARE\Microsoft\Windows\CurrentVersion\Uninstall\GIMP-{#GIMP_APP_VERSION}_is1','DisplayVersion')
Type: files; Name: "{autodesktop}\GIMP {reg:HKA\SOFTWARE\Microsoft\Windows\CurrentVersion\Uninstall\GIMP-{#GIMP_APP_VERSION}_is1,DisplayVersion|GIMP {#GIMP_APP_VERSION}}.lnk"; Check: CheckRegValueExists('SOFTWARE\Microsoft\Windows\CurrentVersion\Uninstall\GIMP-{#GIMP_APP_VERSION}_is1','DisplayVersion')
#else
Type: files; Name: "{autoprograms}\GIMP {reg:HKA\SOFTWARE\Microsoft\Windows\CurrentVersion\Uninstall\GIMP-{#MAJOR}_is1,DisplayVersion|GIMP {#MAJOR}}.lnk"; Check: CheckRegValueExists('SOFTWARE\Microsoft\Windows\CurrentVersion\Uninstall\GIMP-{#MAJOR}_is1','DisplayVersion')
Type: files; Name: "{autodesktop}\GIMP {reg:HKA\SOFTWARE\Microsoft\Windows\CurrentVersion\Uninstall\GIMP-{#MAJOR}_is1,DisplayVersion|GIMP {#MAJOR}}.lnk"; Check: CheckRegValueExists('SOFTWARE\Microsoft\Windows\CurrentVersion\Uninstall\GIMP-{#MAJOR}_is1','DisplayVersion')
#endif
;remove old babl and gegl plugins
Type: filesandordirs; Name: "{app}\lib\babl-0.1"
Type: filesandordirs; Name: "{app}\lib\gegl-0.4"


[Registry]
;Shell "Open with"
Root: HKA; Subkey: "Software\Classes\Applications\gimp-{#GIMP_APP_VERSION}.exe"; Flags: uninsdeletekey
Root: HKA; Subkey: "Software\Classes\Applications\gimp-{#GIMP_APP_VERSION}.exe"; ValueType: string; ValueName: "FriendlyAppName"; ValueData: "GIMP"
Root: HKA; Subkey: "Software\Classes\Applications\gimp-{#GIMP_APP_VERSION}.exe\DefaultIcon"; ValueType: string; ValueName: ""; ValueData: "{app}\bin\gimp-{#GIMP_APP_VERSION}.exe,1"
Root: HKA; Subkey: "Software\Classes\Applications\gimp-{#GIMP_APP_VERSION}.exe\shell\open\command"; ValueType: string; ValueName: ""; ValueData: """{app}\bin\gimp-{#GIMP_APP_VERSION}.exe"" ""%1"""

Root: HKA; Subkey: "Software\GIMP {#GIMP_APP_VERSION}"; Flags: uninsdeletekey
Root: HKA; Subkey: "Software\GIMP {#GIMP_APP_VERSION}\Capabilities"; ValueType: string; ValueName: "ApplicationName"; ValueData: "GIMP"
Root: HKA; Subkey: "Software\GIMP {#GIMP_APP_VERSION}\Capabilities"; ValueType: string; ValueName: "ApplicationIcon"; ValueData: "{app}\bin\gimp-{#GIMP_APP_VERSION}.exe,0"
Root: HKA; Subkey: "Software\GIMP {#GIMP_APP_VERSION}\Capabilities"; ValueType: string; ValueName: "ApplicationDescription"; ValueData: "GIMP is a free raster graphics editor used for image retouching and editing, free-form drawing, converting between different image formats, and more specialized tasks."

Root: HKA; Subkey: "Software\RegisteredApplications"; ValueType: string; ValueName: "GIMP {#GIMP_APP_VERSION}"; ValueData: "Software\GIMP {#GIMP_APP_VERSION}\Capabilities"; Flags: uninsdeletevalue

;Associations
#pragma option -e-
#define protected
#define Line=0
#define FileLine
#sub ProcessAssociation
	#if !defined(Finished)
		#if Copy(FileLine,1,1)=="#" || FileLine==""
			//skip comments and empty lines
		#else
			#pragma message "Processing data_associations.list: " + FileLine
Root: HKA; Subkey: "Software\Classes\.{#FileLine}\OpenWithProgids"; ValueType: string; ValueName: "GIMP{#MAJOR}.{#FileLine}"; ValueData: ""; Flags: uninsdeletevalue
Root: HKA; Subkey: "Software\Classes\GIMP{#MAJOR}.{#FileLine}"; ValueType: string; ValueName: ""; ValueData: "GIMP {#GIMP_VERSION} {#UpperCase(FileLine)}"; Flags: uninsdeletekey
Root: HKA; Subkey: "Software\Classes\GIMP{#MAJOR}.{#FileLine}\DefaultIcon"; ValueType: string; ValueName: ""; ValueData: "{app}\bin\gimp-{#GIMP_APP_VERSION}.exe,1"
Root: HKA; Subkey: "Software\Classes\GIMP{#MAJOR}.{#FileLine}\shell\open\command"; ValueType: string; ValueName: ""; ValueData: """{app}\bin\gimp-{#GIMP_APP_VERSION}.exe"" ""%1"""
Root: HKA; Subkey: "Software\Classes\Applications\gimp-{#GIMP_APP_VERSION}.exe\SupportedTypes"; ValueType: string; ValueName: ".{#FileLine}"; ValueData: ""
Root: HKA; Subkey: "Software\GIMP {#GIMP_APP_VERSION}\Capabilities\FileAssociations"; ValueType: string; ValueName: ".{#FileLine}"; ValueData: "GIMP{#MAJOR}.{#FileLine}"
		#endif
	#endif
#endsub

#define FileHandle
#for {FileHandle = FileOpen(AddBackslash(SourcePath)+"data_associations.list"); \
  FileHandle && !FileEof(FileHandle); FileLine = FileRead(FileHandle)} \
  ProcessAssociation
#if FileHandle
  #expr FileClose(FileHandle)
#endif

;special case for .ico files
Root: HKA; Subkey: "Software\Classes\.ico\OpenWithProgids"; ValueType: string; ValueName: "GIMP{#MAJOR}.ico"; ValueData: ""; Flags: uninsdeletevalue
Root: HKA; Subkey: "Software\Classes\GIMP{#MAJOR}.ico"; ValueType: string; ValueName: ""; ValueData: "GIMP {#GIMP_VERSION}"; Flags: uninsdeletekey
Root: HKA; Subkey: "Software\Classes\GIMP{#MAJOR}.ico\DefaultIcon"; ValueType: string; ValueName: ""; ValueData: "%1"
Root: HKA; Subkey: "Software\Classes\GIMP{#MAJOR}.ico\shell\open\command"; ValueType: string; ValueName: ""; ValueData: """{app}\bin\gimp-{#GIMP_APP_VERSION}.exe"" ""%1"""
Root: HKA; Subkey: "Software\Classes\Applications\gimp-{#GIMP_APP_VERSION}.exe\SupportedTypes"; ValueType: string; ValueName: ".ico"; ValueData: ""
Root: HKA; Subkey: "Software\GIMP {#GIMP_APP_VERSION}\Capabilities\FileAssociations"; ValueType: string; ValueName: ".ico"; ValueData: "GIMP{#MAJOR}.{#FileLine}"


[UninstallDelete]
Type: files; Name: "{app}\uninst\uninst.inf"
Type: files; Name: "{app}\lib\gimp\{#GIMP_API_VERSION}\interpreters\lua.interp"
Type: files; Name: "{app}\lib\gimp\{#GIMP_API_VERSION}\environ\pygimp.env"


[Code]
//GENERAL VARS AND UTILS
const
	CP_ACP = 0;
	CP_UTF8 = 65001;
  COLOR_HOTLIGHT = 26;

var
	//pgSimple: TWizardPage;
  InstallMode: (imNone, imSimple, imCustom, imRebootContinue);
  ConfigOverride: (coUndefined, coOverride, coDontOverride);
	Force32bitInstall: Boolean;

function WideCharToMultiByte(CodePage: Cardinal; dwFlags: DWORD; lpWideCharStr: String; cchWideCharStr: Integer;
                             lpMultiByteStr: PAnsiChar; cbMultiByte: Integer; lpDefaultChar: Integer;
                             lpUsedDefaultChar: Integer): Integer; external 'WideCharToMultiByte@Kernel32 stdcall';

function MultiByteToWideChar(CodePage: Cardinal; dwFlags: DWORD; lpMultiByteStr: PAnsiChar; cbMultiByte: Integer;
                             lpWideCharStr: String; cchWideChar: Integer): Integer;
                             external 'MultiByteToWideChar@Kernel32 stdcall';

function GetLastError(): DWORD; external 'GetLastError@Kernel32 stdcall';

function GetSysColor(nIndex: Integer): DWORD; external 'GetSysColor@user32.dll stdcall';

function GetButtonWidth(const Texts: TArrayOfString; const Minimum: Integer): Integer;
var MeasureLabel: TNewStaticText;
	i: Integer;
begin
	MeasureLabel := TNewStaticText.Create(WizardForm);
	with MeasureLabel do
	begin
		Parent := WizardForm;
		Left := 0;
		Top := 0;
		AutoSize := True;
	end;

	Result := Minimum;

	for i := 0 to GetArrayLength(Texts) - 1 do
	begin
		MeasureLabel.Caption := Texts[i];
		if (MeasureLabel.Width + ScaleX(16)) > Result then
			Result := (MeasureLabel.Width + ScaleX(16));
	end;

	MeasureLabel.Free;
end;

#include "util_general.isi"

#include "util_MessageWithURL.isi"


//0. PRELIMINARY SETUP CODE

//SSE instructions
const
  PF_XMMI_INSTRUCTIONS_AVAILABLE = 6;

function IsProcessorFeaturePresent(ProcessorFeature: DWORD): LongBool; external 'IsProcessorFeaturePresent@kernel32 stdcall';

//Existing 32-bit
procedure Check32bitOverride;
var i: Integer;
	old: String;
begin
	Force32bitInstall := False;

	old := LowerCase(GetPreviousData('32bitMode',''));

	if old = 'true' then //ignore command line if previous install is already present
		Force32bitInstall := True
	else if old = 'false' then
		Force32bitInstall := False
	else
		for i := 0 to ParamCount do //not a bug (in script anyway) - ParamCount returns the index of last ParamStr element, not the actual count
			if ParamStr(i) = '/32' then
			begin
				Force32bitInstall := True;
				break;
			end;

	DebugMsg('Check32bitOverride',BoolToStr(Force32bitInstall) + '[' + old + ']');
end;

procedure RegisterPreviousData(PreviousDataKey: Integer);
begin
	if Is64BitInstallMode() then
		SetPreviousData(PreviousDataKey,'32BitMode',BoolToStr(Force32bitInstall));
end;

//Resume after reboot (if needed)
function RestartSetupAfterReboot(): Boolean; forward;

//Screen bit depth
function GetDC(hWnd: Integer): Integer; external 'GetDC@User32 stdcall';
function ReleaseDC(hWnd, hDC: Integer): Integer; external 'ReleaseDC@User32 stdcall';
function GetDeviceCaps(hDC, nIndex: Integer): Integer; external 'GetDeviceCaps@GDI32 stdcall';

const
	BITSPIXEL = 12;
  PLANES = 14;
function BPPTooLowWarning(): Boolean;
var hDC, bpp, pl: Integer;
	Message,Buttons: TArrayOfString;
begin
	hDC := GetDC(0);
	pl := GetDeviceCaps(hDC, PLANES);
	bpp := GetDeviceCaps(hDC, BITSPIXEL);
	ReleaseDC(0,hDC);

	bpp := pl * bpp;

	if bpp < 32 then
	begin
		SetArrayLength(Message,1);
		SetArrayLength(Buttons,2);
		Message[0] := CustomMessage('Require32BPP');
		Buttons[0] := CustomMessage('Require32BPPContinue');
		Buttons[1] := CustomMessage('Require32BPPExit');
		if (not WizardSilent) and
		   (MessageWithURL(Message, CustomMessage('Require32BPPTitle'), Buttons, mbError, 2, 2) = 2) then
			Result := False
		else
			Result := True;
	end
	else
		Result := True;
end;

function InitializeSetup(): Boolean;
#if (Defined(GIMP_UNSTABLE) && GIMP_UNSTABLE != "") || Defined(DEVEL_WARNING)
var Message,Buttons: TArrayOfString;
#endif
begin
	ConfigOverride := coUndefined;

	if not IsProcessorFeaturePresent(PF_XMMI_INSTRUCTIONS_AVAILABLE) then
	begin
		SuppressibleMsgBox(CustomMessage('SSERequired'), mbCriticalError, MB_OK, 0);
		Result := false;
		exit;
	end;

	Check32bitOverride;

	Result := RestartSetupAfterReboot(); //resume install after reboot - skip all setting pages, and install directly

	if Result then
		Result := BPPTooLowWarning();

	if not Result then //no need to do anything else
		exit;

//Unstable version warning
#if (Defined(GIMP_UNSTABLE) && GIMP_UNSTABLE != "") || Defined(DEVEL_WARNING)
	Explode(Message, CustomMessage('DevelopmentWarning'), #13#10);
	SetArrayLength(Buttons,2);
	Buttons[0] := CustomMessage('DevelopmentButtonContinue');
	Buttons[1] := CustomMessage('DevelopmentButtonExit');
	if (not WizardSilent) and
	   (MessageWithURL(Message, CustomMessage('DevelopmentWarningTitle'), Buttons, mbError, 2, 2) = 2) then
	begin
		Result := False;
		Exit;
	end;
#endif

	try
		ExtractTemporaryFile('install-end.scale-100.bmp');
		ExtractTemporaryFile('installsplash.bmp');
		ExtractTemporaryFile('installsplash_small.bmp');
	except
		DebugMsg('InitializeSetup','Error extracting temporary file: ' + GetExceptionMessage);
		MsgBox(CustomMessage('ErrorExtractingTemp') + #13#13 + GetExceptionMessage,mbError,MB_OK);
		Result := False;
		exit;
	end;

	//if InstallMode <> imRebootContinue then
	//	SuppressibleMsgBox(CustomMessage('UninstallWarning'),mbError,MB_OK,IDOK);
end;


//1. WELCOME: add splash image with buttons in non-default positions
var
  WelcomeBitmapBottom: TBitmapImage;
  btnInstall, btnCustomize: TNewButton;

procedure UpdateWizardImages();
var NewBitmap1,NewBitmap2: TFileStream;
begin
	WelcomeBitmapBottom := TBitmapImage.Create(WizardForm);
	with WelcomeBitmapBottom do
	begin
		Left := 0;
		Top := 0;
		Parent := WizardForm;
		Width := WizardForm.ClientWidth;
		Height := WizardForm.ClientHeight;
		Stretch := True;
	end;

	DebugMsg('UpdateWizardImages','Height: ' + IntToStr(WizardForm.WizardBitmapImage.Height));

	if WizardForm.WizardBitmapImage.Height < 386 then //use smaller image when not using Large Fonts
	begin
		try
			NewBitmap1 := TFileStream.Create(ExpandConstant('{tmp}\installsplash_small.bmp'),fmOpenRead);
			WizardForm.WizardBitmapImage.Bitmap.LoadFromStream(NewBitmap1);
			WelcomeBitmapBottom.Bitmap := WizardForm.WizardBitmapImage.Bitmap;
			try
				NewBitmap2 := TFileStream.Create(ExpandConstant('{tmp}\install-end.scale-100.bmp'),fmOpenRead);
				WizardForm.WizardBitmapImage2.Bitmap.LoadFromStream(NewBitmap2);
			except
				DebugMsg('UpdateWizardImages','Error loading image: ' + GetExceptionMessage);
			finally
				if NewBitmap2 <> nil then
					NewBitmap2.Free;
			end;
		except
			DebugMsg('UpdateWizardImages','Error loading image: ' + GetExceptionMessage);
		finally
			if NewBitmap1 <> nil then
				NewBitmap1.Free;
		end;
	end
	else
	begin
		try
			NewBitmap1 := TFileStream.Create(ExpandConstant('{tmp}\installsplash.bmp'),fmOpenRead);
			WizardForm.WizardBitmapImage.Bitmap.LoadFromStream(NewBitmap1);
			WelcomeBitmapBottom.Bitmap := WizardForm.WizardBitmapImage.Bitmap;
		except
			DebugMsg('UpdateWizardImages','Error loading image: ' + GetExceptionMessage);
		finally
			if NewBitmap1 <> nil then
				NewBitmap1.Free;
		end;
	end;
	WizardForm.WizardBitmapImage.Width := WizardForm.ClientWidth;
	WizardForm.WizardBitmapImage.Height := WizardForm.ClientHeight;
end;

procedure PrepareWelcomePage();
begin
	if not WizardSilent then
	begin
		WizardForm.NextButton.Visible := False;

		btnInstall.Visible := True;
		btnInstall.TabOrder := 1;
		btnCustomize.Visible := True;

		WizardForm.Bevel.Visible := False;
		WizardForm.WelcomeLabel1.Visible := False;
		WizardForm.WelcomeLabel2.Visible := False;

		WelcomeBitmapBottom.Visible := True;
	end;
end;

procedure CleanUpCustomWelcome();
begin
	WizardForm.NextButton.Visible := True;
	btnInstall.Visible := False;
	btnCustomize.Visible := False;

	WizardForm.Bevel.Visible := True;
	WelcomeBitmapBottom.Visible := False;
end;

procedure InstallOnClick(Sender: TObject);
begin
	DebugMsg('Install mode','Simple');
	InstallMode := imSimple;

	CleanUpCustomWelcome();

	WizardForm.NextButton.OnClick(TNewButton(Sender).Parent);
end;

procedure CustomizeOnClick(Sender: TObject);
begin
	DebugMsg('Install mode','Custom');
	InstallMode := imCustom;

	CleanUpCustomWelcome();

	WizardForm.NextButton.OnClick(TNewButton(Sender).Parent);
end;

procedure InitCustomPages();
var	i,ButtonWidth: Integer;
	ButtonText: TArrayOfString;
	MeasureLabel: TNewStaticText;
	//lblInfo: TNewStaticText;
begin
	DebugMsg('InitCustomPages','wpLicense');

	btnInstall := TNewButton.Create(WizardForm);
	with btnInstall do
	begin
		Parent := WizardForm;
		Width := WizardForm.NextButton.Width;
		Height := WizardForm.NextButton.Height;
		Left := WizardForm.NextButton.Left;
		Top := WizardForm.NextButton.Top;
		Caption := CustomMessage('Install');
		Default := True;
		Visible := False;

		OnClick := @InstallOnClick;
	end;

	//used to measure text width
	MeasureLabel := TNewStaticText.Create(WizardForm);
	with MeasureLabel do
	begin
		Parent := WizardForm;
		Left := 0;
		Top := 0;
		AutoSize := True;
		Caption := CustomMessage('Customize');
	end;

	btnCustomize := TNewButton.Create(WizardForm);
	with btnCustomize do
	begin
		Parent := WizardForm;
		Width := WizardForm.NextButton.Width;

		if Width < (MeasureLabel.Width + ScaleX(8)) then
			Width := MeasureLabel.Width + ScaleX(8);

		Height := WizardForm.NextButton.Height;
		Left := WizardForm.ClientWidth - (WizardForm.CancelButton.Left + WizardForm.CancelButton.Width);
		//Left := WizardForm.BackButton.Left;
		Top := WizardForm.NextButton.Top;
		Visible := False;

		Caption := CustomMessage('Customize');

		OnClick := @CustomizeOnClick;
	end;

	MeasureLabel.Free;
end;


//2. LICENSE
procedure InfoBeforeLikeLicense();
begin
	WizardForm.InfoBeforeClickLabel.Visible := False;
	WizardForm.InfoBeforeMemo.Height := WizardForm.InfoBeforeMemo.Height + WizardForm.InfoBeforeMemo.Top - WizardForm.InfoBeforeClickLabel.Top;
	WizardForm.InfoBeforeMemo.Top := WizardForm.InfoBeforeClickLabel.Top;
end;


//3. INSTALL DIR (no customizations)


//4. COMPONENTS: Add panel with description on click, to the right of the list
var
	lblComponentDescription: TNewStaticText;

procedure ComponentsListOnClick(pSender: TObject); forward;

procedure SelectComponentsFaceLift();
var pnlDescription: TPanel;
	lblDescription: TNewStaticText;
begin
	DebugMsg('SelectComponentsFaceLift','');

	if WizardForm.ComponentsList.Width = WizardForm.SelectComponentsPage.Width then
		WizardForm.ComponentsList.Width := Round(WizardForm.ComponentsList.Width * 0.6)
	else
		exit;
	DebugMsg('SelectComponentsFaceLift','2');

	WizardForm.ComponentsList.OnClick := @ComponentsListOnClick;

	lblDescription := TNewStaticText.Create(WizardForm.ComponentsList.Parent)
	with lblDescription do
	begin
		Left := WizardForm.ComponentsList.Left + WizardForm.ComponentsList.Width + ScaleX(16);
		Top := WizardForm.ComponentsList.Top;
		AutoSize := True;
		Caption := CustomMessage('ComponentsDescription');
	end;

	pnlDescription := TPanel.Create(WizardForm.ComponentsList.Parent);
	with pnlDescription do
	begin
		Parent := WizardForm.ComponentsList.Parent;
		Left := WizardForm.ComponentsList.Left + WizardForm.ComponentsList.Width + ScaleX(8);
		Width := WizardForm.TypesCombo.Width - WizardForm.ComponentsList.Width - ScaleX(8);
		ParentColor := True;
		BevelOuter := bvLowered;
		BevelInner := bvRaised;
		Top := WizardForm.ComponentsList.Top + Round(lblDescription.Height * 0.4);
		Height := WizardForm.ComponentsList.Height - Round(lblDescription.Height * 0.4);
	end;

	lblDescription.Parent := WizardForm.ComponentsList.Parent; //place lblDescription above pnlDescription

	lblComponentDescription := TNewStaticText.Create(pnlDescription);
	with lblComponentDescription do
	begin
		Parent := pnlDescription;
		Left := ScaleX(8);
		WordWrap := True;
		AutoSize := False;
		Width := Parent.Width - ScaleX(16);
		Height := Parent.Height - ScaleY(20);
		Top := ScaleY(12);
	end;
end;

procedure ComponentsListOnClick(pSender: TObject);
var i,j: Integer;
	Components: TArrayOfString;
	ComponentDesc: String;
begin
	DebugMsg('ComponentsListOnClick','');

	Components := ['Gimp','Deps','Debug', 'Ghostscript','Lua','Python','Translations','MyPaint','Gimp32'];
	ComponentDesc := '';

	for i := 0 to TNewCheckListBox(pSender).Items.Count - 1 do
		if TNewCheckListBox(pSender).Selected[i] then
		begin
			for j := 0 to Length(Components) - 1 do
			begin
				if TNewCheckListBox(pSender).Items.Strings[i] = CustomMessage('Components' + Components[j]) then
					ComponentDesc := CustomMessage('Components' + Components[j] + 'Description');
			end;

			if ComponentDesc <> '' then
				break;
		end;

	lblComponentDescription.Caption := ComponentDesc;
end;


//5. TAKS (no customizations)


//6. READY: Add formatting support to text box on ready page
var
	ReadyMemoRichText: String;

procedure ReadyFaceLift();
var rtfNewReadyMemo: TRichEditViewer;
begin
	DebugMsg('ReadyFaceLift','');
	WizardForm.ReadyMemo.Visible := False;

	rtfNewReadyMemo := TRichEditViewer.Create(WizardForm.ReadyMemo.Parent);
	with rtfNewReadyMemo do
	begin
		Parent := WizardForm.ReadyMemo.Parent;
		Scrollbars := ssVertical;
		Color := WizardForm.Color;
		BevelKind := bkFlat;
		BorderStyle := bsNone;
		UseRichEdit := True;
		RTFText := ReadyMemoRichText;
		ReadOnly := True;
		Left := WizardForm.ReadyMemo.Left;
		Top := WizardForm.ReadyMemo.Top;
		Width := WizardForm.ReadyMemo.Width;
		Height := WizardForm.ReadyMemo.Height;
		Visible := True;
	end;
end;

function CopyW(const S: String; const Start, Len: Integer): String; //work-around for unicode-related bug in Copy
begin
	Result := Copy(S, Start, Len);
end;

function Unicode2RTF(const pIn: String): String; //convert to RTF-compatible unicode
var	i: Integer;
	c: SmallInt;
begin
	Result := '';
	for i := 1 to Length(pIn) do
		if Ord(pIn[i]) <= 127 then
		begin
			Result := Result + pIn[i];
		end else
		begin
			c := Ord(pIn[i]); //code points above 7FFF must be expressed as negative numbers
			Result := Result + '\u' + IntToStr(c) + '?';
		end;
end;

const
  RTFPara	  = '\par ';
function ParseReadyMemoText(pSpaces,pText: String): String;
var sTemp: String;
begin
	sTemp := CopyW(pText,Pos(#10,pText)+1,Length(pText));
	sTemp := Replace('{','\{',sTemp);
	sTemp := Replace('\','\\',sTemp);
	sTemp := Replace(#13#10,RTFpara,sTemp);
	sTemp := Replace(pSpaces,'',sTemp);
	sTemp := '\b ' + CopyW(pText,1,Pos(#13,pText)-1) + '\par\sb0' +
						'\li284\b0 ' + sTemp + '\par \pard';

	Result := Unicode2RTF(sTemp);
end;

const
  RTFHeader = '{\rtf1\deff0{\fonttbl{\f0\fswiss\fprq2\fcharset0 Segoe UI;}{\f1\fnil\fcharset2 Segoe UI Symbol;}}\viewkind4\uc1\fs18';
	//RTFBullet = '{\pntext\f1\''B7\tab}';
function UpdateReadyMemo(pSpace, pNewLine, pMemoUserInfo, pMemoDirInfo, pMemoTypeInfo, pMemoComponentsInfo, pMemoGroupInfo, pMemoTasksInfo: String): String;
var sText: String;
	bShowAssoc: Boolean;
	i,j: Integer;
begin
	DebugMsg('UpdateReadyMemo','');
	(* Prepare the text for new Ready Memo *)

	sText := RTFHeader;
	if pMemoDirInfo <> '' then
		sText := sText + ParseReadyMemoText(pSpace,pMemoDirInfo) + '\sb100';
	sText := sText + ParseReadyMemoText(pSpace,pMemoTypeInfo);
	sText := sText + '\sb100' + ParseReadyMemoText(pSpace,pMemoComponentsInfo);

	If pMemoTasksInfo<>'' then
		sText := sText + '\sb100' + ParseReadyMemoText(pSpace,pMemoTasksInfo);

	ReadyMemoRichText := Copy(sText,1,Length(sText)-6) + '}';

	Result := 'If you see this, something went wrong';
end;


//7.1 BEFORE INSTALL

//Unistall old version of GIMP (only if needed)
const
  UNINSTALL_MAX_WAIT_TIME = 10000;
	UNINSTALL_CHECK_TIME    =   250;

type
	TRemoveOldGIMPResult = (rogContinue, rogRestartRequired, rogUninstallFailed, rogCantUninstall);
procedure DoUninstall(const UninstStr, InstallDir: String; const pInfoLabel: TNewStaticText; var oResult: TRemoveOldGIMPResult);
var InResult: TRemoveOldGIMPResult;
	ResultCode, i: Integer;
begin
	InResult := oResult;

	DebugMsg('DoUninstall','Uninstall string: ' + UninstStr);

	//when installing to same directory, assume that restart is required by default ...
	if LowerCase(RemoveBackslashUnlessRoot(InstallDir)) = LowerCase(RemoveBackslashUnlessRoot(ExpandConstant('{app}'))) then
		oResult := rogRestartRequired
	else
		oResult := InResult;

	pInfoLabel.Caption := InstallDir;

	if not Exec('>',UninstStr,'',SW_SHOW,ewWaitUntilTerminated,ResultCode) then
	begin
		DebugMsg('DoUninstall','Exec('+UninstStr+') failed: ' + IntToStr(ResultCode));

		if not DirExists(InstallDir) then //old install directory doesn't exist, assume it was deleted, and Registry info is orphaned
		begin
			DebugMsg('DoUninstall','Install directory doesn'#39't exist: ' + InstallDir + ', resuming install');
			oResult := InResult
		end else
		begin
			oResult := rogUninstallFailed;
		end;

		exit;
	end;

	DebugMsg('DoUninstall','Exec succeeded, uninstaller result: ' + IntToStr(ResultCode));

	//... unless the complete installation directory was removed on uninstall
	i := 0;
	while i < (UNINSTALL_MAX_WAIT_TIME / UNINSTALL_CHECK_TIME) do
	begin
		if not DirExists(ExpandConstant('{app}')) then
		begin
			DebugMsg('DoUninstall','Existing GIMP directory removed, restoring previous restart requirement');
			oResult := InResult; //restore previous result
			break;
		end;
		DebugMsg('DoUninstall','Waiting for ' + ExpandConstant('{app}') + ' to disappear [' + IntToStr(i) + ']');
		Sleep(UNINSTALL_CHECK_TIME); //it may take a few seconds for the uninstaller to remove the directory after it's exited
		Inc(i);
	end;
end;

function RemoveOldGIMPVersions(): TRemoveOldGIMPResult;
var lblInfo1,lblInfo2: TNewStaticText;
	RootKey: Integer;
	OldPath, UninstallString, WhichStr: String;
begin
	Result := rogContinue;

	lblInfo1 := TNewStaticText.Create(WizardForm.PreparingPage);
	with lblInfo1 do
	begin
		Parent := WizardForm.PreparingPage;
		Left := 0;
		Top := 0;
		AutoSize := True;
		WordWrap := True;
		Width := WizardForm.PreparingPage.ClientWidth;

		Caption := CustomMessage('RemovingOldVersion');
	end;

	lblInfo2 := TNewStaticText.Create(WizardForm.PreparingPage);
	with lblInfo2 do
	begin
		Parent := WizardForm.PreparingPage;
		Left := 0;
		AutoSize := True;
		WordWrap := True;
		Width := WizardForm.PreparingPage.ClientWidth;
		Top := lblInfo1.Height + ScaleY(8);
	end;

	if ExpandConstant('{param:debugresume|0}') = '1' then
		Result := rogRestartRequired; //for testing

	if Is64BitInstallMode() then
		RootKey := HKLM32
	else
		RootKey := HKLM;

	if RegValueExists(RootKey,'Software\Microsoft\Windows\CurrentVersion\Uninstall\WinGimp-2.0_is1',
	                  'Inno Setup: App Path') then
	begin
		if RegQueryStringValue(RootKey,'Software\Microsoft\Windows\CurrentVersion\Uninstall\WinGimp-2.0_is1',
		                       'Inno Setup: App Path',OldPath) then
		begin
			DebugMsg('RemoveOldGIMPVersions','Found legacy GIMP install, removing');

			if RegValueExists(RootKey,'Software\Microsoft\Windows\CurrentVersion\Uninstall\WinGimp-2.0_is1',
			                  'QuietUninstallString') then
				WhichStr := 'QuietUninstallString'
			else if RegValueExists(RootKey,'Software\Microsoft\Windows\CurrentVersion\Uninstall\WinGimp-2.0_is1',
			                       'UninstallString') then
				WhichStr := 'UninstallString'
			else
			begin
				Result := rogCantUninstall;
				exit;
			end;

			if not RegQueryStringValue(RootKey,'Software\Microsoft\Windows\CurrentVersion\Uninstall\WinGimp-2.0_is1',
			                           WhichStr,UninstallString) then
			begin
				Result := rogCantUninstall;
				exit;
			end;

			if WhichStr = 'UninstallString' then
				UninstallString := UninstallString + ' /SILENT';

			UninstallString := UninstallString + ' /NORESTART';

			DoUninstall(UninstallString, OldPath, lblInfo2, Result);
		end;
	end;

	lblInfo1.Free;
	lblInfo2.Free;
end;

procedure CreateRunOnceEntry; forward;

function PrepareToInstall(var pNeedsRestart: Boolean): String;
var	ChecksumBefore, ChecksumAfter: String;
	RemoveResult: TRemoveOldGIMPResult;
begin
	ChecksumBefore := MakePendingFileRenameOperationsChecksum;

  RemoveResult := RemoveOldGIMPVersions;

	if RemoveResult = rogRestartRequired then //old version was uninstalled, but something was left behind, so to be safe a reboot
	begin                                     //is enforced - this can only happen when reusing install directory

		DebugMsg('PrepareToInstall','RemoveOldGIMPVersions requires restart');

		ChecksumAfter := MakePendingFileRenameOperationsChecksum;
		if (ChecksumBefore <> ChecksumAfter) or (ExpandConstant('{param:debugresume|0}') = '1') then
		begin //this check is most likely redundant, since the uninstaller will be added to pending rename operations
			CreateRunOnceEntry;
			pNeedsRestart := True;
			Result := CustomMessage('RebootRequiredFirst');
		end;
	end else
	if RemoveResult = rogContinue then
	begin
		DebugMsg('PrepareToInstall','RemoveOldGIMPVersions finished successfully');
		Result := ''; //old version was uninstalled successfully, nothing was left behind, so install can continue immediately
	end else
	if RemoveResult = rogUninstallFailed then
	begin
		DebugMsg('PrepareToInstall','RemoveOldGIMPVersions failed to uninstall old GIMP version');
		Result := FmtMessage(CustomMessage('RemovingOldVersionFailed'),['{#GIMP_VERSION}',ExpandConstant('{app}')]);
	end else
	if RemoveResult = rogCantUninstall then
	begin
		DebugMsg('PrepareToInstall','RemoveOldGIMPVersions failed to uninstall old GIMP version [1]');
		Result := FmtMessage(CustomMessage('RemovingOldVersionCantUninstall'),['{#GIMP_VERSION}',ExpandConstant('{app}')]);
	end else
	begin
		DebugMsg('PrepareToInstall','Internal error 11');
		Result := FmtMessage(CustomMessage('InternalError'),['11']); //should never happen
	end;
end;

//remove .debug files from previous installs
//there's no built-in way in Inno to recursively delete files with wildcard+extension
procedure RemoveDebugFilesFromDir(pDir: String; var pDirectories: TArrayOfString);
var FindRec: TFindRec;
begin
	DebugMsg('RemoveDebugFilesFromDir', pDir);
	WizardForm.FilenameLabel.Caption := pDir;
	if FindFirst(AddBackSlash(pDir) + '*', FindRec) then
	begin
		try
			repeat
				if FindRec.Attributes and FILE_ATTRIBUTE_DIRECTORY = 0 then
				begin
					if (Length(FindRec.Name) > 6) and (LowerCase(Copy(FindRec.Name, Length(FindRec.Name) - 5, 6)) = '.debug') then
					begin
						DebugMsg('RemoveDebugFilesFromDir', '> ' + FindRec.Name);
						DeleteFile(AddBackSlash(pDir) + FindRec.Name);
					end;
				end else
				begin
					if (FindRec.Name <> '.') and (FindRec.Name <> '..') then
					begin
						SetArrayLength(pDirectories, GetArrayLength(pDirectories) + 1);
						pDirectories[GetArrayLength(pDirectories) - 1] := AddBackSlash(pDir) + FindRec.Name;
					end;
				end;
			until not FindNext(FindRec);
		finally
			FindClose(FindRec);
		end;
	end;
end;

procedure RemoveDebugFiles();
var Directories: TArrayOfString;
	Index: Integer;
begin
	SetArrayLength(Directories, 1);
	Directories[0] := ExpandConstant('{app}');
	Index := 0;

	WizardForm.StatusLabel.Caption := CustomMessage('RemovingOldFiles');

	repeat
		RemoveDebugFilesFromDir(Directories[Index], Directories);
		Inc(Index);
	until Index = GetArrayLength(Directories);
end;

procedure AssociationsCleanUp(); forward;

//Check if icon exists in registry
function CheckRegValueExists(const SubKeyName, ValueName: String): Boolean;
begin
	Result := RegValueExists(HKEY_AUTO, SubKeyName, ValueName);
	DebugMsg('CheckRegValueExists',SubKeyName + ', ' + ValueName + ': ' + BoolToStr(Result));
end;

//Legacy arch check
function Check3264(const pWhich: String): Boolean;
begin
	if pWhich = '64' then //x64 or arm64
		Result := Is64BitInstallMode() and (not Force32bitInstall)
	else if pWhich = '32' then
		Result := (not Is64BitInstallMode()) or Force32bitInstall
	else if pWhich = 'x64' then
		Result := Is64BitInstallMode() and IsX64 and (not Force32bitInstall)
	else if pWhich = 'arm64' then
		Result := Is64BitInstallMode() and IsARM64 and (not Force32bitInstall)
	else
		RaiseException('Unknown check');
end;

//some programs improperly install libraries to the System32 directory, which then causes problems with plugins
//this function checks if such file exists in System32, and lets setup update the file when it exists
function BadSysDLL(const pFile: String; const pPlatform: Integer): Boolean;
var	OldRedir: Boolean;
begin
	Result := False;

	if pPlatform = 64 then
	begin
		if Is64BitInstallMode() then //only check when installing in 64bit mode
		begin
			OldRedir := EnableFsRedirection(False);
			DebugMsg('BadSysDLL','64: ' + ExpandConstant('{sys}\' + pFile));
			Result := FileExists(ExpandConstant('{sys}\' + pFile));
			EnableFsRedirection(OldRedir);
		end;
	end
	else if pPlatform = 32 then
	begin
		if Is64BitInstallMode() then //check 32bit system directory on x64 windows
		begin
			DebugMsg('BadSysDLL','32on64: ' + ExpandConstant('{syswow64}\' + pFile));
			Result := FileExists(ExpandConstant('{syswow64}\' + pFile));
		end
		else
		begin
			DebugMsg('BadSysDLL','32: ' + ExpandConstant('{sys}\' + pFile));
			Result := FileExists(ExpandConstant('{sys}\' + pFile));
		end;
	end
	else
	begin
		RaiseException('Unsupported platform');
	end;

	DebugMsg('BadSysDLL','Result: ' + BoolToStr(Result));
end;

//Override some 'etc' configs (if requested)
const
  CONFIG_OVERRIDE_PARAM = 'configoverride';

function DoConfigOverride: Boolean;
var i: Integer;
begin
	if ConfigOverride = coUndefined then
	begin
		DebugMsg('DoConfigOverride', 'First call');

		Result := False;
		ConfigOverride := coDontOverride;

		for i := 0 to ParamCount() do //use ParamCount/ParamStr to allow specifying /configoverride without any parameters
			if LowerCase(Copy(ParamStr(i),1,15)) = '/' + CONFIG_OVERRIDE_PARAM then
			begin
				Result := True;
				ConfigOverride := coOverride;
				break;
			end;
	end
	else if ConfigOverride = coOverride then
		Result := True
  else
		Result := False;

	DebugMsg('DoConfigOverride', BoolToStr(Result));
end;

function GetExternalConfDir(Unused: String): String;
begin
	if ExpandConstant('{param:' + CONFIG_OVERRIDE_PARAM + '|<>}') = '<>' then
		Result := ExpandConstant('{src}\')
	else
		Result := ExpandConstant('{param:' + CONFIG_OVERRIDE_PARAM + '|<>}\');

  DebugMsg('GetExternalConfDir', Result);
end;

function CheckExternalConf(const pFile: String): Boolean;
begin
	if not DoConfigOverride then //no config override
		Result := False
	else
	begin
		if FileExists(GetExternalConfDir('') + pFile) then //config file override only applies when that file exists
			Result := True
		else
			Result := False;
	end;

	DebugMsg('CheckExternalConf', pFile + ': ' + BoolToStr(Result));
end;


//7.2 INSTALL: show GIMP text (aka billboard) above progress bar
#if Defined(GIMP_UNSTABLE) && GIMP_UNSTABLE != ""
	const
		GIMP_URL = 'https://gimp.org/downloads/devel/';
#else
	const
		GIMP_URL = 'https://gimp.org/downloads/';
#endif

procedure lblURL_OnClick(Sender: TObject);
var ErrorCode: Integer;
begin
	ShellExecAsOriginalUser('',GIMP_URL,'','',SW_SHOW,ewNoWait,ErrorCode);
end;

function MeasureLabel(const pText: String): Integer; //WordWrap + AutoSize works better with TNewStaticText than with TLabel,
var lblMeasure: TNewStaticText;                      //abuse this
begin
	lblMeasure := TNewStaticText.Create(WizardForm.InstallingPage);

  with lblMeasure do
	begin
		Parent := WizardForm.InstallingPage;

		AutoSize := True;
		WordWrap := True;
		Width := Parent.ClientWidth;

		Caption := pText;

		Result := Height;
	end;

  lblMeasure.Free;
end;

procedure InstallingFaceLift();
var lblMessage1,lblURL,lblMessage2: TLabel; //TNewStaticText doesn't support alignment
begin
	with WizardForm.ProgressGauge do
	begin
		Height := ScaleY(21);
		Top := WizardForm.InstallingPage.ClientHeight - Top - Height;

		WizardForm.StatusLabel.Top := Top - WizardForm.FilenameLabel.Height - ScaleY(4);
		WizardForm.FilenameLabel.Top := Top + Height + ScaleY(4);
	end;

	lblMessage1 := TLabel.Create(WizardForm.InstallingPage);
	with lblMessage1 do
	begin
		Parent := WizardForm.InstallingPage;

		Alignment := taCenter;
		WordWrap := True;
		AutoSize := False;
		Width := WizardForm.InstallingPage.ClientWidth;
		Height := MeasureLabel(CustomMessage('Billboard1'));

		Caption := CustomMessage('Billboard1');
	end;

	lblURL := TLabel.Create(WizardForm.InstallingPage);
	with lblURL do
	begin
		Parent := WizardForm.InstallingPage;

		AutoSize := True;
		WordWrap := False;

		Font.Color := GetSysColor(COLOR_HOTLIGHT);
		Font.Style := [fsUnderline];
		Font.Size := 12;

		Cursor := crHand;

		OnClick := @lblURL_OnClick;

		Caption := GIMP_URL;

		Left := Integer(WizardForm.InstallingPage.ClientWidth / 2 - Width / 2);
	end;

	lblMessage2 := TLabel.Create(WizardForm.InstallingPage);
	with lblMessage2 do
	begin
		Parent := WizardForm.InstallingPage;

		Alignment := taCenter;
		WordWrap := True;
		AutoSize := False;
		Width := WizardForm.InstallingPage.ClientWidth;
		Height := MeasureLabel(CustomMessage('Billboard2'));

		Caption := CustomMessage('Billboard2');
	end;

	lblMessage1.Top := Integer(WizardForm.StatusLabel.Top / 2 -
	                           (lblMessage1.Height + ScaleY(4) + lblURL.Height + ScaleY(4) + lblMessage2.Height) / 2);
	lblURL.Top := lblMessage1.Top + lblMessage1.Height + ScaleY(4);
	lblMessage2.Top := lblURL.Top + lblURL.Height + ScaleY(4);
end;


//7.3 AFTER INSTALL

//Create .interp files
procedure PrepareInterp();
var InterpFile,InterpContent,LuaBin: String;
begin
#ifdef PYTHON
	if IsComponentSelected('py32') or IsComponentSelected('py64') or IsComponentSelected('pyARM64') then
	begin
		StatusLabel(CustomMessage('SettingUpPyGimp'),'');

		InterpFile := ExpandConstant('{app}\lib\gimp\{#GIMP_API_VERSION}\interpreters\pygimp.interp');
    DebugMsg('PrepareInterp','Writing interpreter file for gimp-python: ' + InterpFile);

#ifdef GIMP_UNSTABLE
	  #define PYTHON="python.exe"
#else
	  #define PYTHON="pythonw.exe"
#endif

		InterpContent := 'python=' + ExpandConstant('{app}\bin\{#PYTHON}') + #10 +
		                 'python3=' + ExpandConstant('{app}\bin\{#PYTHON}') + #10 +
		                 '/usr/bin/python=' + ExpandConstant('{app}\bin\{#PYTHON}') + #10 +
		                 '/usr/bin/python3=' + ExpandConstant('{app}\bin\{#PYTHON}') + #10 +
		                 ':Python:E::py::python:'#10;

		if not SaveStringToUTF8File(InterpFile,InterpContent,False) then
		begin
			DebugMsg('PrepareInterp','Problem writing the file. [' + InterpContent + ']');
			SuppressibleMsgBox(CustomMessage('ErrorUpdatingPython') + ' (2)',mbInformation,mb_ok,IDOK);
		end;
	end;
#endif

#ifdef LUA
	if IsComponentSelected('lua32') or IsComponentSelected('lua64') or IsComponentSelected('luaARM64') then
	begin
		InterpFile := ExpandConstant('{app}\lib\gimp\{#GIMP_API_VERSION}\interpreters\lua.interp');
    DebugMsg('PrepareInterp','Writing interpreter file for lua: ' + InterpFile);

		LuaBin := 'luajit.exe'

		InterpContent := 'lua=' + ExpandConstant('{app}\bin\') + LuaBin + #10 +
		                 'luajit=' + ExpandConstant('{app}\bin\') + LuaBin + #10 +
		                 '/usr/bin/luajit=' + ExpandConstant('{app}\bin\') + LuaBin + #10 +
		                 '/usr/bin/lua=' + ExpandConstant('{app}\bin\') + LuaBin + #10 +
		                 ':Lua:E::lua::' + LuaBin + ':'#10;

		if not SaveStringToUTF8File(InterpFile,InterpContent,False) then
		begin
			DebugMsg('PrepareInterp','Problem writing the file. [' + InterpContent + ']');
			SuppressibleMsgBox(CustomMessage('ErrorUpdatingPython') + ' (2)',mbInformation,mb_ok,IDOK);
		end;
	end;
#endif

// !!! use comma for binfmt delimiter and full Windows path in interpreter field of binfmt
begin
	InterpFile := ExpandConstant('{app}\lib\gimp\{#GIMP_API_VERSION}\interpreters\gimp-script-fu-interpreter.interp');
	DebugMsg('PrepareInterp','Writing interpreter file for gimp-script-fu-interpreter: ' + InterpFile);

	InterpContent := 'gimp-script-fu-interpreter=' + ExpandConstant('{app}\bin\gimp-script-fu-interpreter-{#GIMP_API_VERSION}.exe') + #10 +
						       'gimp-script-fu-interpreter-{#GIMP_API_VERSION}=' + ExpandConstant('{app}\bin\gimp-script-fu-interpreter-{#GIMP_API_VERSION}.exe') + #10 +
						       '/usr/bin/gimp-script-fu-interpreter=' + ExpandConstant('{app}\bin\gimp-script-fu-interpreter-{#GIMP_API_VERSION}.exe') + #10 +
						       ',ScriptFu,E,,scm,,' + ExpandConstant('{app}\bin\gimp-script-fu-interpreter-{#GIMP_API_VERSION}.exe') + ','#10;

	if not SaveStringToUTF8File(InterpFile,InterpContent,False) then
	begin
		DebugMsg('PrepareInterp','Problem writing the file. [' + InterpContent + ']');
		SuppressibleMsgBox(CustomMessage('ErrorUpdatingScriptFu') + ' (2)',mbInformation,mb_ok,IDOK);
	end;
end;
end; //PrepareInterp

//Create .env files
procedure PrepareGimpEnvironment();
var EnvFile,Env: String;
begin
	StatusLabel(CustomMessage('SettingUpEnvironment'),'');

	//set PATH to be used by plug-ins
	EnvFile := ExpandConstant('{app}\lib\gimp\{#GIMP_API_VERSION}\environ\default.env');
	DebugMsg('PrepareGimpEnvironment','Setting environment in ' + EnvFile);

	Env := #10'PATH=${gimp_installation_dir}\bin';

	if IsComponentSelected('gimp32on64') then
	begin
		Env := Env + ';${gimp_installation_dir}\32\bin' + #10;
	end else
	begin
		Env := Env + #10;
	end;

	DebugMsg('PrepareGimpEnvironment','Appending ' + Env);

	if not SaveStringToUTF8File(EnvFile,Env,True) then
	begin
		DebugMsg('PrepareGimpEnvironment','Problem appending');
		SuppressibleMsgBox(FmtMessage(CustomMessage('ErrorChangingEnviron'),[EnvFile]),mbInformation,mb_ok,IDOK);
	end;

	// Disable check-update when run with specific option
  if ExpandConstant('{param:disablecheckupdate|false}') = 'true' then
	begin
		EnvFile := ExpandConstant('{app}\share\gimp\{#GIMP_API_VERSION}\gimp-release');
		DebugMsg('DisableCheckUpdate','Disabling check-update in ' + EnvFile);

    Env := 'check-update=false'

		if not SaveStringToUTF8File(EnvFile,Env,True) then
		begin
			DebugMsg('PrepareGimpEnvironment','Problem appending');
			SuppressibleMsgBox(FmtMessage(CustomMessage('ErrorChangingEnviron'),[EnvFile]),mbInformation,mb_ok,IDOK);
		end;
	end;
end;//PrepareGimpEnvironment

//Unistaller info
procedure SaveToUninstInf(const pText: AnsiString); forward;

procedure SaveToUninstInf(const pText: AnsiString);
var sUnInf: String;
	sOldContent: String;
begin
	sUnInf := ExpandConstant('{app}\uninst\uninst.inf');

	if not FileExists(sUnInf) then //save small header
		SaveStringToUTF8File(sUnInf,#$feff+'#Additional uninstall tasks'#13#10+ //#$feff BOM is required for LoadStringsFromFile
		                            '#This file uses UTF-8 encoding'#13#10+
		                            '#'#13#10+
		                            '#Empty lines and lines beginning with # are ignored'#13#10+
		                            '#'#13#10+
		                            '#Add uninstallers for GIMP add-ons that should be removed together with GIMP like this:'#13#10+
		                            '#Run:<description>/<full path to uninstaller>/<parameters for automatic uninstall>'#13#10+
		                            '#'#13#10+
		                            '#The file is parsed in reverse order' + #13#10 +
		                            '' + #13#10 //needs '' in front, otherwise preprocessor complains
		                            ,False)
	else
	begin
		LoadStringFromUTF8File(sUnInf,sOldContent);
		if Pos(#13#10+pText+#13#10,sOldContent) > 0 then //don't write duplicate lines
			exit;
	end;

	SaveStringToUTF8File(sUnInf,pText+#13#10,True);
end;

#include "util_uninst.isi"


//8. DONE/FINAL PAGE (no customizations)


//INITIALIZE AND ORDER INSTALLER PAGES
procedure InitializeWizard();
begin
	UpdateWizardImages();
	InitCustomPages();
end;

function ShouldSkipPage(pPageID: Integer): Boolean;
begin
	DebugMsg('ShouldSkipPage','ID: '+IntToStr(pPageID));

	Result := ((InstallMode = imSimple) or (InstallMode = imRebootContinue)) and (pPageID <> wpFinished);
	if Result then
		DebugMsg('ShouldSkipPage','Yes')
	else
		DebugMsg('ShouldSkipPage','No');
end;

procedure CurPageChanged(pCurPageID: Integer);
begin
	DebugMsg('CurPageChanged','ID: '+IntToStr(pCurPageID));
	case pCurPageID of
		wpWelcome:
			PrepareWelcomePage();
		wpInfoBefore:
			InfoBeforeLikeLicense();
		wpSelectComponents:
			SelectComponentsFaceLift();
		wpReady:
			ReadyFaceLift();
		wpInstalling:
			InstallingFaceLift();
	end;
end;

procedure CurStepChanged(pCurStep: TSetupStep);
begin
	case pCurStep of
		ssInstall:
		begin
			RemoveDebugFiles();
			AssociationsCleanup();
		end;
		ssPostInstall:
		begin
			PrepareInterp();
			PrepareGimpEnvironment();
		end;
	end;
end;

//Reboot if needed
#include "util_rebootcontinue.isi"


#expr SaveToFile(AddBackslash(SourcePath) + "Preprocessed.iss")
