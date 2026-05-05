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
;Install script for GIMP and its deps
;requires Inno Setup 6
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

;1 NOTE: This script do NOT work with Inno Compiler alone


;2 GLOBAL VARIABLES SET BY PARAMS
;FIXME: Meson don't support C++ style comments. See: https://github.com/mesonbuild/meson/issues/14260
#include BUILD_DIR + "\config_clean.h"

;Main GIMP versions:
;1) Get GIMP_MUTEX_VERSION (used for internal versioning control)
;Taken from config_clean.h
;2) Get FULL_GIMP_VERSION (used by ITs)
#define ORIGINAL_GIMP_VERSION GIMP_VERSION
#if Defined(GIMP_RC_VERSION)
  #define GIMP_VERSION=Copy(GIMP_VERSION,1,Pos("-",GIMP_VERSION)-1)
#endif
#if !Defined(REVISION) || REVISION=="0"
  #define FULL_GIMP_VERSION GIMP_VERSION + "." + "0"
#else
  #define FULL_GIMP_VERSION GIMP_VERSION + "." + REVISION
#endif
;3) Get CUSTOM_GIMP_VERSION (that the users see)
#if !Defined(REVISION) || REVISION=="0"
  #define CUSTOM_GIMP_VERSION ORIGINAL_GIMP_VERSION
#else
  #define CUSTOM_GIMP_VERSION ORIGINAL_GIMP_VERSION + "-" + REVISION
#endif
#define DISPLAY_NAME "GIMP " + CUSTOM_GIMP_VERSION

;This script supports creating both an installer per arch or an universal installer with all arches
#if Defined(ARM64_BUNDLE) && !Defined(X64_BUNDLE)
#define ARCHS_ALLOWED="arm64"
#define ARCHS_INSTALLED="arm64"
#define GIMP_ARCHS="gimpARM64"
#define DEPS_ARCHS="depsARM64"
#define PY_ARCHS="pyARM64"
#define LUA_ARCHS="luaARM64"
#endif
#if !Defined(ARM64_BUNDLE) && Defined(X64_BUNDLE)
#define ARCHS_ALLOWED="x64os"
#define ARCHS_INSTALLED="x64os"
#define GIMP_ARCHS="gimpX64"
#define DEPS_ARCHS="depsX64"
#define PY_ARCHS="pyX64"
#define LUA_ARCHS="luaX64"
#endif
#if Defined(ARM64_BUNDLE) && Defined(X64_BUNDLE)
#define ARCHS_ALLOWED="x64compatible"
#define ARCHS_INSTALLED="x64os arm64"
#define GIMP_ARCHS="gimpARM64 or gimpX64"
#define DEPS_ARCHS="depsARM64 or depsX64"
#define PY_ARCHS="pyARM64 or pyX64"
#define LUA_ARCHS="luaARM64 or luaX64"
#endif

;Optional Build-time params: DEBUG_SYMBOLS, LUA, PYTHON, NOCOMPRESSION, NOFILES, DEVEL_WARNING

;Optional Run-time params: /configoverride= /disablecheckupdate=false|true /debugresume=0|1 /resumeinstall=0|1|2


;3 INSTALLER SOURCE
#define ASSETS_DIR BUILD_DIR + "\build\windows\installer"

;3.1.1 Icons and other files
#include ASSETS_DIR + "\splash-dimensions.h"
#define WIZARD_SMALL_IMAGE ASSETS_DIR + "\gimp.scale-100.bmp," + ASSETS_DIR + "\gimp.scale-125.bmp," + ASSETS_DIR + "\gimp.scale-150.bmp," + ASSETS_DIR + "\gimp.scale-175.bmp," + ASSETS_DIR + "\gimp.scale-200.bmp," + ASSETS_DIR + "\gimp.scale-225.bmp," + ASSETS_DIR + "\gimp.scale-250.bmp"
#define WIZARD_IMAGE ASSETS_DIR + "\install-end.scale-100.bmp," + ASSETS_DIR + "\install-end.scale-125.bmp," + ASSETS_DIR + "\install-end.scale-150.bmp," + ASSETS_DIR + "\install-end.scale-175.bmp," + ASSETS_DIR + "\install-end.scale-200.bmp," + ASSETS_DIR + "\install-end.scale-225.bmp," + ASSETS_DIR + "\install-end.scale-250.bmp"

;3.1.2 Installer lang files
[Languages]
Name: "en"; MessagesFile: "compiler:Default.isl,{#ASSETS_DIR}\lang\en.setup.isl"
#include ASSETS_DIR + "\base_po-msg.list"
;Avoid annoying inoffensive warnings about langs that we can't do nothing about
[Setup]
MissingMessagesWarning=no
NotRecognizedMessagesWarning=no


;3.2.1 INNO INTERNAL VERSIONING (used to rule how different versions are installed)
;GIMP process identifier for Inno
AppMutex=GIMP-{#GIMP_MUTEX_VERSION}
;Inno installer identifier
SetupMutex=GIMP-{#GIMP_MUTEX_VERSION}-setup
;Inno installer (default) install location
DefaultDirName={autopf}\GIMP {#GIMP_MUTEX_VERSION}
;Inno uninstaller identifier
AppID=GIMP-{#GIMP_MUTEX_VERSION}
;Inno unninstaller identifier logs location
UninstallFilesDir={app}\uninst

;3.2.2 WINDOWS INTERNAL VERSIONING (used by ITs for deploying GIMP)
;Inno installer file version
VersionInfoVersion={#FULL_GIMP_VERSION}
;Inno installer product version and ImmersiveControlPanel 'DisplayVersion'
AppVersion={#FULL_GIMP_VERSION}

;3.2.3 THAT'S WHAT THE FINAL USER ACTUALLY SEE
;ImmersiveControlPanel 'DisplayName'
AppVerName={#DISPLAY_NAME}
;ImmersiveControlPanel 'DisplayIcon'
UninstallDisplayIcon={app}\bin\gimp-{#GIMP_MUTEX_VERSION}.exe
;ImmersiveControlPanel 'Publisher'
AppPublisher=The GIMP Team
;ControlPanel 'URLInfoAbout'
AppPublisherURL=https://www.gimp.org/
;ControlPanel 'HelpLink'
AppSupportURL=https://www.gimp.org/docs/
#if Defined(GIMP_UNSTABLE)
  ;ControlPanel 'URLUpdateInfo'
  AppUpdatesURL=https://www.gimp.org/downloads/devel/
#else
  AppUpdatesURL=https://www.gimp.org/downloads/
#endif


;3.3.1 EXE COMPRESSION
LZMANumBlockThreads=4
LZMABlockSize=76800
#ifdef NOCOMPRESSION
OutputDir=..\..\..\unc
Compression=none
DiskSpanning=yes
DiskSliceSize=max
#else
Compression=lzma2/ultra64
InternalCompressLevel=ultra
SolidCompression=yes
LZMAUseSeparateProcess=yes
LZMANumFastBytes=273
; Increasing dictionary size may improve compression ratio at the
; expense of memory requirement. We run into "Out of memory" error in
; the CI.
;LZMADictionarySize=524288
#endif //NOCOMPRESSION

;3.3.2 EXE SIGNING
;SignTool=Default
;SignedUninstaller=yes
;SignedUninstallerDir=_Uninst

;3.3.3 EXE FILE DETAILS
AppName=GIMP
SetupIconFile={#ASSETS_DIR}\setup.ico
OutputDir=..\..\..
OutputBaseFileName=gimp-{#CUSTOM_GIMP_VERSION}-setup
OutputManifestFile=inno.log
ArchitecturesAllowed={#ARCHS_ALLOWED}
ArchitecturesInstallIn64BitMode={#ARCHS_INSTALLED}
MinVersion=10.0
//we can't enforce the version from devel-docs\os-support.txt because Windows Server don't always have free upgrades


;3.4.1 INSTALLER PAGES
PrivilegesRequired=lowest
PrivilegesRequiredOverridesAllowed=dialog
ShowLanguageDialog=auto
DisableWelcomePage=no
InfoBeforeFile=gpl+python.rtf
DisableDirPage=no
FlatComponentsList=yes
DisableProgramGroupPage=yes
AllowNoIcons=no
ChangesAssociations=true
ChangesEnvironment=yes
AlwaysShowDirOnReadyPage=yes

;3.4.2 INSTALLER UI: uses modern Win32 "Vista" (still used today) design
WizardStyle=modern dynamic
WizardSizePercent=100
WizardImageAlphaFormat=defined
WizardSmallImageFile={#WIZARD_SMALL_IMAGE}
WizardSmallImageFileDynamicDark={#WIZARD_SMALL_IMAGE}
WizardImageFile={#WIZARD_IMAGE}
WizardImageFileDynamicDark={#WIZARD_IMAGE}
WizardImageStretch=yes
WizardKeepAspectRatio=no
[LangOptions]
DialogFontBaseScaleHeight=13
DialogFontBaseScaleWidth=6
WelcomeFontSize=12

;3.4.1 INSTALLER PAGES AGAIN
[Tasks]
Name: desktopicon; Description: "{cm:AdditionalIconsDesktop}"; GroupDescription: "{cm:AdditionalIcons}"
[Icons]
Name: "{autoprograms}\{#DISPLAY_NAME}"; Filename: "{app}\bin\gimp-{#GIMP_MUTEX_VERSION}.exe"; WorkingDir: "%USERPROFILE%"; Comment: "{#DISPLAY_NAME}"
Name: "{autodesktop}\{#DISPLAY_NAME}"; Filename: "{app}\bin\gimp-{#GIMP_MUTEX_VERSION}.exe"; WorkingDir: "%USERPROFILE%"; Comment: "{#DISPLAY_NAME}"; Tasks: desktopicon
[Run]
Filename: "{app}\bin\gimp-{#GIMP_MUTEX_VERSION}.exe"; Description: "{cm:LaunchGimp}"; Flags: unchecked postinstall nowait skipifsilent


;4.1 GIMP FILES (make sure that the resulting .exe installer content is identical to the .msix and vice-versa)
[Types]
Name: full; Description: "{cm:TypeFull}"
Name: compact; Description: "{cm:TypeCompact}"
Name: custom; Description: "{cm:TypeCustom}"; Flags: iscustom

[Components]
;Required components (minimal install)
;GIMP files
#ifdef ARM64_BUNDLE
Name: gimpARM64; Description: "{cm:ComponentsGimp,{#GIMP_VERSION}}"; Types: full compact custom; Flags: fixed; Check: CheckArch('arm64')
#endif
#ifdef X64_BUNDLE
Name: gimpX64; Description: "{cm:ComponentsGimp,{#GIMP_VERSION}}"; Types: full compact custom; Flags: fixed; Check: CheckArch('x64')
#endif
;Deps files
#ifdef ARM64_BUNDLE
Name: depsARM64; Description: "{cm:ComponentsDeps,{#FULL_GIMP_VERSION}}"; Types: full compact custom; Flags: fixed; Check: CheckArch('arm64')
#endif
#ifdef X64_BUNDLE
Name: depsX64; Description: "{cm:ComponentsDeps,{#FULL_GIMP_VERSION}}"; Types: full compact custom; Flags: fixed; Check: CheckArch('x64')
#endif

;Optional components (complete install)
#ifdef DEBUG_SYMBOLS
#ifdef ARM64_BUNDLE
Name: debugARM64; Description: "{cm:ComponentsDebug}"; Types: full custom; Flags: disablenouninstallwarning; Check: CheckArch('arm64')
#endif
#ifdef X64_BUNDLE
Name: debugX64; Description: "{cm:ComponentsDebug}"; Types: full custom; Flags: disablenouninstallwarning; Check: CheckArch('x64')
#endif
#endif
;Development files
#ifdef ARM64_BUNDLE
Name: devARM64; Description: "{cm:ComponentsDev}"; Types: full custom; Flags: disablenouninstallwarning; Check: CheckArch('arm64')
#endif
#ifdef X64_BUNDLE
Name: devX64; Description: "{cm:ComponentsDev}"; Types: full custom; Flags: disablenouninstallwarning; Check: CheckArch('x64')
#endif
;PostScript support
#ifdef ARM64_BUNDLE
Name: gsARM64; Description: "{cm:ComponentsGhostscript}"; Types: full custom; Check: CheckArch('arm64')
#endif
#ifdef X64_BUNDLE
Name: gsX64; Description: "{cm:ComponentsGhostscript}"; Types: full custom; Check: CheckArch('x64')
#endif
;Lua plug-ins support
#ifdef LUA
#ifdef ARM64_BUNDLE
Name: luaARM64; Description: "{cm:ComponentsLua}"; Types: full custom; Check: CheckArch('arm64')
#endif
#ifdef X64_BUNDLE
Name: luaX64; Description: "{cm:ComponentsLua}"; Types: full custom; Check: CheckArch('x64')
#endif
#endif
;Python plug-ins support
#ifdef PYTHON
#ifdef ARM64_BUNDLE
Name: pyARM64; Description: "{cm:ComponentsPython}"; Types: full custom; Check: CheckArch('arm64')
#endif
#ifdef X64_BUNDLE
Name: pyX64; Description: "{cm:ComponentsPython}"; Types: full custom; Check: CheckArch('x64')
#endif
#endif
;Locales
Name: loc; Description: "{cm:ComponentsTranslations}"; Types: full custom
#include ASSETS_DIR + "\base_po-cmp.list"
;MyPaint Brushes
Name: mypaint; Description: "{cm:ComponentsMyPaint}"; Types: full custom

[Files]
;setup files
Source: "{#ASSETS_DIR}\installsplash_top.scale-100.bmp"; Flags: dontcopy
Source: "{#ASSETS_DIR}\installsplash_top.scale-125.bmp"; Flags: dontcopy
Source: "{#ASSETS_DIR}\installsplash_top.scale-150.bmp"; Flags: dontcopy
Source: "{#ASSETS_DIR}\installsplash_top.scale-175.bmp"; Flags: dontcopy
Source: "{#ASSETS_DIR}\installsplash_top.scale-200.bmp"; Flags: dontcopy
Source: "{#ASSETS_DIR}\installsplash_top.scale-225.bmp"; Flags: dontcopy
Source: "{#ASSETS_DIR}\installsplash_top.scale-250.bmp"; Flags: dontcopy
Source: "{#ASSETS_DIR}\installsplash_bottom.bmp"; Flags: dontcopy

#ifndef NOFILES
#ifdef ARM64_BUNDLE
#define MAIN_BUNDLE ARM64_BUNDLE
#endif
#ifdef X64_BUNDLE
#define MAIN_BUNDLE X64_BUNDLE
#endif
#define COMMON_FLAGS="recursesubdirs restartreplace uninsrestartdelete ignoreversion"

;Required arch-neutral files (compact install)
#define OPTIONAL_EXT="*.pdb,*.lua,*.py"
Source: "{#MAIN_BUNDLE}\etc\gimp\*"; DestDir: "{app}\etc\gimp"; Components: {#GIMP_ARCHS}; Flags: {#COMMON_FLAGS}
Source: "{#MAIN_BUNDLE}\lib\gimp\{#GIMP_PKGCONFIG_VERSION}\environ\default.env"; DestDir: "{app}\lib\gimp\{#GIMP_PKGCONFIG_VERSION}\environ"; Components: {#GIMP_ARCHS}; Flags: {#COMMON_FLAGS}
Source: "{#MAIN_BUNDLE}\lib\gimp\{#GIMP_PKGCONFIG_VERSION}\interpreters\gimp-script-fu-interpreter*.interp"; DestDir: "{app}\lib\gimp\{#GIMP_PKGCONFIG_VERSION}\interpreters"; Components: {#GIMP_ARCHS}; Flags: {#COMMON_FLAGS}
Source: "{#MAIN_BUNDLE}\lib\gimp\{#GIMP_PKGCONFIG_VERSION}\extensions\*"; DestDir: "{app}\lib\gimp\{#GIMP_PKGCONFIG_VERSION}\extensions"; Excludes: "*.dll,*.exe,{#OPTIONAL_EXT}"; Components: {#GIMP_ARCHS}; Flags: {#COMMON_FLAGS}
Source: "{#MAIN_BUNDLE}\lib\gimp\{#GIMP_PKGCONFIG_VERSION}\plug-ins\*"; DestDir: "{app}\lib\gimp\{#GIMP_PKGCONFIG_VERSION}\plug-ins"; Excludes: "*.dll,*.exe,{#OPTIONAL_EXT}"; Components: {#GIMP_ARCHS}; Flags: {#COMMON_FLAGS}
Source: "{#MAIN_BUNDLE}\share\gimp\*"; DestDir: "{app}\share\gimp"; Components: {#GIMP_ARCHS}; Flags: {#COMMON_FLAGS} createallsubdirs
Source: "{#MAIN_BUNDLE}\share\icons\hicolor\*"; DestDir: "{app}\share\icons\hicolor"; Components: {#GIMP_ARCHS}; Flags: {#COMMON_FLAGS}
Source: "{#MAIN_BUNDLE}\etc\*"; DestDir: "{app}\etc"; Excludes: "gimp,ssl"; Components: {#DEPS_ARCHS}; Flags: {#COMMON_FLAGS}
Source: "{#MAIN_BUNDLE}\share\*"; DestDir: "{app}\share"; Excludes: "gimp,icons\hicolor,locale\*,mypaint-data"; Components: {#DEPS_ARCHS}; Flags: {#COMMON_FLAGS} createallsubdirs

;Optional arch-neutral files (full install)
#ifdef LUA
Source: "{#MAIN_BUNDLE}\lib\gimp\{#GIMP_PKGCONFIG_VERSION}\interpreters\lua.interp"; DestDir: "{app}\lib\gimp\{#GIMP_PKGCONFIG_VERSION}\interpreters"; Components: {#LUA_ARCHS}; Flags: {#COMMON_FLAGS}
Source: "{#MAIN_BUNDLE}\*.lua"; DestDir: "{app}"; Excludes: "share\gimp\*.lua"; Components: {#LUA_ARCHS}; Flags: {#COMMON_FLAGS}
#endif
#ifdef PYTHON
Source: "{#MAIN_BUNDLE}\etc\ssl\*"; DestDir: "{app}\etc\ssl"; Components: {#PY_ARCHS}; Flags: {#COMMON_FLAGS}
Source: "{#MAIN_BUNDLE}\lib\gimp\{#GIMP_PKGCONFIG_VERSION}\environ\py*.env"; DestDir: "{app}\lib\gimp\{#GIMP_PKGCONFIG_VERSION}\environ"; Components: {#PY_ARCHS}; Flags: {#COMMON_FLAGS}
Source: "{#MAIN_BUNDLE}\lib\gimp\{#GIMP_PKGCONFIG_VERSION}\interpreters\pygimp*.interp"; DestDir: "{app}\lib\gimp\{#GIMP_PKGCONFIG_VERSION}\interpreters"; Components: {#PY_ARCHS}; Flags: {#COMMON_FLAGS}
Source: "{#MAIN_BUNDLE}\*.py"; DestDir: "{app}"; Components: {#PY_ARCHS}; Flags: {#COMMON_FLAGS}
#endif
Source: "{#MAIN_BUNDLE}\share\locale\*"; DestDir: "{app}\share\locale"; Components: loc; Flags: dontcopy {#COMMON_FLAGS}
#include ASSETS_DIR + "\base_po-files.list"
Source: "{#MAIN_BUNDLE}\share\mypaint-data\*"; DestDir: "{app}\share\mypaint-data"; Components: mypaint; Flags: {#COMMON_FLAGS}


#define OPTIONAL_DLL="libgs*.dll,lua*.dll,corelgilua*.dll,gluas.dll,libpython*.dll"
#define OPTIONAL_EXE="file-ps.exe,lua*.exe,python*.exe"

#define FindHandle
#define FindResult

#define i

#sub EmitBaseExecutables
#if i == 1
  #ifdef ARM64_BUNDLE
  #define private BUNDLE ARM64_BUNDLE
  #define private COMPONENT "ARM64"
  #endif
#elif i == 2
  #ifdef X64_BUNDLE
  #define private BUNDLE X64_BUNDLE
  #define private COMPONENT "X64"
  #endif
#endif

#ifdef BUNDLE
; Required arch-specific components (compact installation)
Source: "{#BUNDLE}\bin\libgimp*.dll"; DestDir: "{app}\bin"; Components: gimp{#COMPONENT}; Flags: {#COMMON_FLAGS}
Source: "{#BUNDLE}\bin\gimp*.exe"; DestDir: "{app}\bin"; Components: gimp{#COMPONENT}; Flags: {#COMMON_FLAGS}
Source: "{#BUNDLE}\lib\gimp\*.dll"; DestDir: "{app}\lib\gimp"; Components: gimp{#COMPONENT}; Flags: {#COMMON_FLAGS}
Source: "{#BUNDLE}\lib\gimp\*.exe"; DestDir: "{app}\lib\gimp"; Excludes: "{#OPTIONAL_EXE}"; Components: gimp{#COMPONENT}; Flags: {#COMMON_FLAGS}
Source: "{#BUNDLE}\lib\girepository-1.0\Gimp*.typelib"; DestDir: "{app}\lib\girepository-1.0"; Components: gimp{#COMPONENT}; Flags: {#COMMON_FLAGS}

Source: "{#BUNDLE}\bin\*"; DestDir: "{app}\bin"; Excludes: "libgimp*.dll,gimp*.exe,*.pdb,{#OPTIONAL_DLL},{#OPTIONAL_EXE}"; Components: deps{#COMPONENT}; Flags: {#COMMON_FLAGS}
Source: "{#BUNDLE}\lib\*"; DestDir: "{app}\lib"; Excludes: "gimp,Gimp*.typelib,*.pdb,*.a,*.pc,lua,gluas.dll,python{#PYTHON_VERSION}"; Components: deps{#COMPONENT}; Flags: {#COMMON_FLAGS}

; Optional arch-specific components (full installation)
#if defined(DEBUG_SYMBOLS)
Source: "{#BUNDLE}\*.pdb"; DestDir: "{app}"; Components: debug{#COMPONENT}; Flags: {#COMMON_FLAGS}
#endif

Source: "{#BUNDLE}\include\*.h**"; DestDir: "{app}\include"; Components: dev{#COMPONENT}; Flags: {#COMMON_FLAGS}
Source: "{#BUNDLE}\lib\*.a"; DestDir: "{app}\lib"; Components: dev{#COMPONENT}; Flags: {#COMMON_FLAGS}
Source: "{#BUNDLE}\lib\pkgconfig\*"; DestDir: "{app}\lib\pkgconfig"; Components: dev{#COMPONENT}; Flags: {#COMMON_FLAGS}

Source: "{#BUNDLE}\bin\libgs*.dll"; DestDir: "{app}\bin"; Components: gs{#COMPONENT}; Flags: {#COMMON_FLAGS}
Source: "{#BUNDLE}\lib\file-ps.exe"; DestDir: "{app}\lib"; Components: gs{#COMPONENT}; Flags: {#COMMON_FLAGS}

#ifdef LUA
Source: "{#BUNDLE}\bin\lua*.dll"; DestDir: "{app}\bin"; Components: lua{#COMPONENT}; Flags: {#COMMON_FLAGS}
Source: "{#BUNDLE}\bin\lua*.exe"; DestDir: "{app}\bin"; Components: lua{#COMPONENT}; Flags: {#COMMON_FLAGS}
Source: "{#BUNDLE}\lib\corelgilua*.dll"; DestDir: "{app}\lib"; Components: lua{#COMPONENT}; Flags: {#COMMON_FLAGS}
Source: "{#BUNDLE}\lib\gluas.dll"; DestDir: "{app}\lib"; Components: lua{#COMPONENT}; Flags: {#COMMON_FLAGS}
#endif

#ifdef PYTHON
Source: "{#BUNDLE}\bin\libpython*.dll"; DestDir: "{app}\bin"; Components: py{#COMPONENT}; Flags: {#COMMON_FLAGS}
Source: "{#BUNDLE}\bin\python*.exe"; DestDir: "{app}\bin"; Components: py{#COMPONENT}; Flags: {#COMMON_FLAGS}
Source: "{#BUNDLE}\lib\python{#PYTHON_VERSION}\*"; DestDir: "{app}\lib\python{#PYTHON_VERSION}"; Excludes: "*.pdb,*.py"; Components: py{#COMPONENT}; Flags: {#COMMON_FLAGS} createallsubdirs
#endif

#undef BUNDLE
#undef COMPONENT
#endif
#endsub

#for {i = 1; i <= 2; i++} EmitBaseExecutables

;upgrade zlib1.dll in System32 if it's present there to avoid breaking plugins
;sharedfile flag will ensure that the upgraded file is left behind on uninstall to avoid breaking other programs that use the file
#ifdef ARM64_BUNDLE
Source: "{#ARM64_BUNDLE}\bin\zlib1.dll"; DestDir: "{sys}"; Components: gimpARM64; Flags: restartreplace sharedfile uninsrestartdelete comparetimestamp; Check: BadSysDLL('zlib1.dll',64)
#endif
#ifdef X64_BUNDLE
Source: "{#X64_BUNDLE}\bin\zlib1.dll"; DestDir: "{sys}"; Components: gimpX64; Flags: restartreplace sharedfile uninsrestartdelete comparetimestamp; Check: BadSysDLL('zlib1.dll',64)
#endif

;allow specific config files to be overridden if '/configoverride=' is set at run time
#sub ProcessConfigFile
  #define FileName FindGetFileName(FindHandle)
Source: "{code:GetExternalConfDir}\{#FileName}"; DestDir: "{app}\{#ConfigDir}"; Flags: external restartreplace; Check: CheckExternalConf('{#FileName}')
#endsub
#sub ProcessConfigDir
  #emit ';; ' + ConfigDir
  #emit ';; ' + BaseDir
  #for {FindHandle = FindResult = FindFirst(AddBackslash(BaseDir) + AddBackSlash(ConfigDir) + "*", 0); \
      FindResult; FindResult = FindNext(FindHandle)} ProcessConfigFile
  #if FindHandle
    #expr FindClose(FindHandle)
  #endif
#endsub
#define public BaseDir MAIN_BUNDLE
#define public ConfigDir "etc\gimp\" + GIMP_PKGCONFIG_VERSION
#expr ProcessConfigDir
#define public ConfigDir "etc\fonts"
#expr ProcessConfigDir

#endif //NOFILES

;4.2 SPECIAL-CASE VERSIONED DEP FILES TO BE WIPED SINCE INNO IS FEBLE (#16087)
[InstallDelete]
Type: files; Name: "{app}\bin\*"
Type: files; Name: "{app}\lib\babl-0.1\*"
Type: files; Name: "{app}\lib\gegl-0.4\*"
Type: files; Name: "{app}\lib\girepository-1.0\*.typelib"
Type: filesandordirs; Name: "{app}\lib\python{#PYTHON_VERSION}\*"
#define DotPos Pos(".", PYTHON_VERSION)
#define PyMajor Copy(PYTHON_VERSION, 1, DotPos - 1)
#define PyMinor Int(Copy(PYTHON_VERSION, DotPos + 1, Len(PYTHON_VERSION) - DotPos))
#define PREVIOUS_PYTHON_VERSION PyMajor + "." + Str(PyMinor - 1)
Type: filesandordirs; Name: "{app}\lib\python{#PREVIOUS_PYTHON_VERSION}\*"
;Wintel support was dropped in 3.2.2
Type: filesandordirs; Name: "{app}\32\*"
Type: filesandordirs; Name: "{app}\lib\gimp\{#GIMP_PKGCONFIG_VERSION}\plug-ins\twain\*"
;get previous GIMP icon name from uninstall name in Registry
Type: files; Name: "{autoprograms}\{reg:HKA\SOFTWARE\Microsoft\Windows\CurrentVersion\Uninstall\GIMP-{#GIMP_MUTEX_VERSION}_is1,DisplayName|GIMP {#GIMP_MUTEX_VERSION}}.lnk"; Check: CheckRegValueExists('SOFTWARE\Microsoft\Windows\CurrentVersion\Uninstall\GIMP-{#GIMP_MUTEX_VERSION}_is1','DisplayName')
Type: files; Name: "{autodesktop}\{reg:HKA\SOFTWARE\Microsoft\Windows\CurrentVersion\Uninstall\GIMP-{#GIMP_MUTEX_VERSION}_is1,DisplayName|GIMP {#GIMP_MUTEX_VERSION}}.lnk"; Check: CheckRegValueExists('SOFTWARE\Microsoft\Windows\CurrentVersion\Uninstall\GIMP-{#GIMP_MUTEX_VERSION}_is1','DisplayName')
;.pdb files are taken care by RemoveDebugFilesFromDir procedure

[UninstallDelete]
Type: files; Name: "{app}\uninst\uninst.inf"
Type: files; Name: "{app}\lib\gimp\{#GIMP_PKGCONFIG_VERSION}\interpreters\lua.interp"
Type: files; Name: "{app}\lib\gimp\{#GIMP_PKGCONFIG_VERSION}\environ\pygimp.env"
Type: files; Name: "{app}\lib\gimp\{#GIMP_PKGCONFIG_VERSION}\environ\pygimp_win.env"


;4.3 KEYS TO BE REGISTERED
[Registry]
;'ms-settings:appsfeatures' page (using info from 3.2.3 [Setup] section above)
;Root: HKA; Subkey: "Software\Microsoft\Windows\CurrentVersion\Uninstall\GIMP-{#GIMP_MUTEX_VERSION}_is1"; //(auto registered by Inno)

;Add GIMP .exe to PATH/'Win + R' (conflicts with MSIX. See: ..\store\AppxManifest.xml)
;Root: HKA; Subkey: "Software\Microsoft\Windows\CurrentVersion\App Paths\gimp-{#GIMP_MUTEX_VERSION}.exe"; Flags: uninsdeletekey
;Root: HKA; Subkey: "Software\Microsoft\Windows\CurrentVersion\App Paths\gimp-{#GIMP_MUTEX_VERSION}.exe"; ValueType: string; ValueName: ""; ValueData: "{app}\bin\gimp-{#GIMP_MUTEX_VERSION}.exe"
;Root: HKA; Subkey: "Software\Microsoft\Windows\CurrentVersion\App Paths\gimp-{#GIMP_MUTEX_VERSION}.exe"; ValueType: string; ValueName: "Path"; ValueData: "{app}\bin"

;Shell "Open with"
Root: HKA; Subkey: "Software\Classes\Applications\gimp-{#GIMP_MUTEX_VERSION}.exe"; Flags: uninsdeletekey
Root: HKA; Subkey: "Software\Classes\Applications\gimp-{#GIMP_MUTEX_VERSION}.exe"; ValueType: string; ValueName: "FriendlyAppName"; ValueData: "{#DISPLAY_NAME}"
Root: HKA; Subkey: "Software\Classes\Applications\gimp-{#GIMP_MUTEX_VERSION}.exe\DefaultIcon"; ValueType: string; ValueName: ""; ValueData: "{app}\bin\gimp-{#GIMP_MUTEX_VERSION}.exe,1"
Root: HKA; Subkey: "Software\Classes\Applications\gimp-{#GIMP_MUTEX_VERSION}.exe\shell\open"; ValueType: string; ValueName: "MultiSelectModel"; ValueData: "Player"; Flags: uninsdeletevalue
Root: HKA; Subkey: "Software\Classes\Applications\gimp-{#GIMP_MUTEX_VERSION}.exe\shell\open\command"; ValueType: string; ValueName: ""; ValueData: """{app}\bin\gimp-{#GIMP_MUTEX_VERSION}.exe"" %*"
;'ms-settings:defaultapps' page
Root: HKA; Subkey: "Software\GIMP {#GIMP_MUTEX_VERSION}"; Flags: uninsdeletekey
Root: HKA; Subkey: "Software\GIMP {#GIMP_MUTEX_VERSION}\Capabilities"; ValueType: string; ValueName: "ApplicationName"; ValueData: "{#DISPLAY_NAME}"
Root: HKA; Subkey: "Software\GIMP {#GIMP_MUTEX_VERSION}\Capabilities"; ValueType: string; ValueName: "ApplicationIcon"; ValueData: "{app}\bin\gimp-{#GIMP_MUTEX_VERSION}.exe,0"
Root: HKA; Subkey: "Software\GIMP {#GIMP_MUTEX_VERSION}\Capabilities"; ValueType: string; ValueName: "ApplicationDescription"; ValueData: "GIMP is a free raster graphics editor used for image retouching and editing, free-form drawing, converting between different image formats, and more specialized tasks."
Root: HKA; Subkey: "Software\RegisteredApplications"; ValueType: string; ValueName: "GIMP {#GIMP_MUTEX_VERSION}"; ValueData: "Software\GIMP {#GIMP_MUTEX_VERSION}\Capabilities"; Flags: uninsdeletevalue
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
      #pragma message "Processing file_associations.list: " + FileLine
Root: HKA; Subkey: "Software\Classes\.{#FileLine}\OpenWithProgids"; ValueType: string; ValueName: "GIMP{#GIMP_MUTEX_VERSION}.{#FileLine}"; ValueData: ""; Flags: uninsdeletevalue
Root: HKA; Subkey: "Software\Classes\GIMP{#GIMP_MUTEX_VERSION}.{#FileLine}"; ValueType: string; ValueName: ""; ValueData: "{#DISPLAY_NAME} {#UpperCase(FileLine)}"; Flags: uninsdeletekey
Root: HKA; Subkey: "Software\Classes\GIMP{#GIMP_MUTEX_VERSION}.{#FileLine}\DefaultIcon"; ValueType: string; ValueName: ""; ValueData: "{app}\bin\gimp-{#GIMP_MUTEX_VERSION}.exe,2"
Root: HKA; Subkey: "Software\Classes\GIMP{#GIMP_MUTEX_VERSION}.{#FileLine}\shell\open"; ValueType: string; ValueName: "MultiSelectModel"; ValueData: "Player"; Flags: uninsdeletevalue
Root: HKA; Subkey: "Software\Classes\GIMP{#GIMP_MUTEX_VERSION}.{#FileLine}\shell\open\command"; ValueType: string; ValueName: ""; ValueData: """{app}\bin\gimp-{#GIMP_MUTEX_VERSION}.exe"" %*"
Root: HKA; Subkey: "Software\Classes\Applications\gimp-{#GIMP_MUTEX_VERSION}.exe\SupportedTypes"; ValueType: string; ValueName: ".{#FileLine}"; ValueData: ""
Root: HKA; Subkey: "Software\GIMP {#GIMP_MUTEX_VERSION}\Capabilities\FileAssociations"; ValueType: string; ValueName: ".{#FileLine}"; ValueData: "GIMP{#GIMP_MUTEX_VERSION}.{#FileLine}"
    #endif
  #endif
#endsub
#define FileHandle
#for {FileHandle = FileOpen(AddBackslash(BUILD_DIR)+"plug-ins\file_associations.list"); \
  FileHandle && !FileEof(FileHandle); FileLine = FileRead(FileHandle)} \
  ProcessAssociation
#if FileHandle
  #expr FileClose(FileHandle)
#endif
;Associations (special case for .xcf files)
Root: HKA; Subkey: "Software\Classes\.xcf\OpenWithProgids"; ValueType: string; ValueName: "GIMP{#GIMP_MUTEX_VERSION}.xcf"; ValueData: ""; Flags: uninsdeletevalue
Root: HKA; Subkey: "Software\Classes\GIMP{#GIMP_MUTEX_VERSION}.xcf"; ValueType: string; ValueName: ""; ValueData: "{#DISPLAY_NAME} XCF"; Flags: uninsdeletekey
Root: HKA; Subkey: "Software\Classes\GIMP{#GIMP_MUTEX_VERSION}.xcf\DefaultIcon"; ValueType: string; ValueName: ""; ValueData: "{app}\bin\gimp-{#GIMP_MUTEX_VERSION}.exe,1"
Root: HKA; Subkey: "Software\Classes\GIMP{#GIMP_MUTEX_VERSION}.xcf\shell\open"; ValueType: string; ValueName: "MultiSelectModel"; ValueData: "Player"; Flags: uninsdeletevalue
Root: HKA; Subkey: "Software\Classes\GIMP{#GIMP_MUTEX_VERSION}.xcf\shell\open\command"; ValueType: string; ValueName: ""; ValueData: """{app}\bin\gimp-{#GIMP_MUTEX_VERSION}.exe"" %*"
Root: HKA; Subkey: "Software\Classes\Applications\gimp-{#GIMP_MUTEX_VERSION}.exe\SupportedTypes"; ValueType: string; ValueName: ".xcf"; ValueData: ""
Root: HKA; Subkey: "Software\GIMP {#GIMP_MUTEX_VERSION}\Capabilities\FileAssociations"; ValueType: string; ValueName: ".xcf"; ValueData: "GIMP{#GIMP_MUTEX_VERSION}.xcf"
;Associations (make association for .ico files but do not set DefaultIcon since their content is the DefaultIcon)
Root: HKA; Subkey: "Software\Classes\.ico\OpenWithProgids"; ValueType: string; ValueName: "GIMP{#GIMP_MUTEX_VERSION}.ico"; ValueData: ""; Flags: uninsdeletevalue
Root: HKA; Subkey: "Software\Classes\GIMP{#GIMP_MUTEX_VERSION}.ico"; ValueType: string; ValueName: ""; ValueData: "{#DISPLAY_NAME} ICO"; Flags: uninsdeletekey
Root: HKA; Subkey: "Software\Classes\GIMP{#GIMP_MUTEX_VERSION}.ico\DefaultIcon"; ValueType: string; ValueName: ""; ValueData: "%1"
Root: HKA; Subkey: "Software\Classes\GIMP{#GIMP_MUTEX_VERSION}.ico\shell\open"; ValueType: string; ValueName: "MultiSelectModel"; ValueData: "Player"; Flags: uninsdeletevalue
Root: HKA; Subkey: "Software\Classes\GIMP{#GIMP_MUTEX_VERSION}.ico\shell\open\command"; ValueType: string; ValueName: ""; ValueData: """{app}\bin\gimp-{#GIMP_MUTEX_VERSION}.exe"" %*"
Root: HKA; Subkey: "Software\Classes\Applications\gimp-{#GIMP_MUTEX_VERSION}.exe\SupportedTypes"; ValueType: string; ValueName: ".ico"; ValueData: ""
Root: HKA; Subkey: "Software\GIMP {#GIMP_MUTEX_VERSION}\Capabilities\FileAssociations"; ValueType: string; ValueName: ".ico"; ValueData: "GIMP{#GIMP_MUTEX_VERSION}.ico"


;5 INSTALLER CUSTOM CODE
[Code]
//GENERAL VARS AND UTILS
const
  CP_ACP = 0;
  CP_UTF8 = 65001;
  COLOR_HOTLIGHT = 26;
  UNINSTALL_MAX_WAIT_TIME = 10000;
  UNINSTALL_CHECK_TIME    =   250;

var
  //pgSimple: TWizardPage;
  InstallType: String;
  InstallMode: (imNone, imSimple, imCustom);
  ConfigOverride: (coUndefined, coOverride, coDontOverride);

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


procedure DebugMsg(Const pProc,pMsg: String);
begin
  Log('[Code] ' + pProc + #9 + pMsg);
end;


function BoolToStr(const b: Boolean): String;
begin
  if b then
    Result := 'True'
  else
    Result := 'False';
end;


function RevPos(const SearchStr, Str: string): Integer;
var i: Integer;
begin

  if Length(SearchStr) < Length(Str) then
    for i := (Length(Str) - Length(SearchStr) + 1) downto 1 do
    begin

      if Copy(Str, i, Length(SearchStr)) = SearchStr then
      begin
        Result := i;
        exit;
      end;

    end;

  Result := 0;
end;


function Replace(pSearchFor, pReplaceWith, pText: String): String;
begin
    StringChangeEx(pText,pSearchFor,pReplaceWith,True);

  Result := pText;
end;


function Count(What, Where: String): Integer;
begin
  Result := 0;
  if Length(What) = 0 then
    exit;

  while Pos(What,Where)>0 do
  begin
    Where := Copy(Where,Pos(What,Where)+Length(What),Length(Where));
    Result := Result + 1;
  end;
end;


//split text to array
procedure Explode(var ADest: TArrayOfString; aText, aSeparator: String);
var tmp: Integer;
begin
  if aSeparator='' then
    exit;

  SetArrayLength(ADest,Count(aSeparator,aText)+1)

  tmp := 0;
  repeat
    if Pos(aSeparator,aText)>0 then
    begin

      ADest[tmp] := Copy(aText,1,Pos(aSeparator,aText)-1);
      aText := Copy(aText,Pos(aSeparator,aText)+Length(aSeparator),Length(aText));
      tmp := tmp + 1;

    end else
    begin

       ADest[tmp] := aText;
       aText := '';

    end;
  until Length(aText)=0;
end;


function String2Utf8(const pInput: String): AnsiString;
var Output: AnsiString;
  ret, outLen, nulPos: Integer;
begin
  outLen := WideCharToMultiByte(CP_UTF8, 0, pInput, -1, Output, 0, 0, 0);
  Output := StringOfChar(#0,outLen);
  ret := WideCharToMultiByte(CP_UTF8, 0, pInput, -1, Output, outLen, 0, 0);

  if ret = 0 then
    RaiseException('WideCharToMultiByte failed: ' + IntToStr(GetLastError));

  nulPos := Pos(#0,Output) - 1;
  if nulPos = -1 then
    nulPos := Length(Output);

    Result := Copy(Output,1,nulPos);
end;


function Utf82String(const pInput: AnsiString): String;
var Output: AnsiString;
  ret, outLen, nulPos: Integer;
begin
  outLen := MultiByteToWideChar(CP_UTF8, 0, pInput, -1, Output, 0);
  Output := StringOfChar(#0,outLen);
  ret := MultiByteToWideChar(CP_UTF8, 0, pInput, -1, Output, outLen);

  if ret = 0 then
    RaiseException('MultiByteToWideChar failed: ' + IntToStr(GetLastError));

  nulPos := Pos(#0,Output) - 1;
  if nulPos = -1 then
    nulPos := Length(Output);

    Result := Copy(Output,1,nulPos);
end;


function SaveStringToUTF8File(const pFileName, pS: String; const pAppend: Boolean): Boolean;
begin
  Result := SaveStringToFile(pFileName, String2Utf8(pS), pAppend);
end;


function LoadStringFromUTF8File(const pFileName: String; var oS: String): Boolean;
var Utf8String: AnsiString;
begin
  Result := LoadStringFromFile(pFileName, Utf8String);
  oS := Utf82String(Utf8String);
end;


procedure StatusLabel(const Status1,Status2: string);
begin
  WizardForm.StatusLabel.Caption := Status1;
  WizardForm.FilenameLabel.Caption := Status2;
  WizardForm.Refresh();
end;


//reverse encoding done by Encode
function Decode(pText: String): String;
var p: Integer;
  tmp: String;
begin
  if Pos('%',pText) = 0 then
    Result := pText
  else
  begin
    Result := '';
    while Length(pText) > 0 do
    begin
      p := Pos('%',pText);
      if p = 0 then
      begin
        Result := Result + pText;
        break;
      end;
      Result := Result + Copy(pText,1,p-1);
      tmp := '$' + Copy(pText,p+1,2);
      Result := Result + Chr(StrToIntDef(tmp,32));
      pText := Copy(pText,p+3,Length(pText));
    end;
  end;
end;

function GetThemedBgColor: TColor;
begin
  if HighContrastActive then
  begin
    Result := clWindow;
  end else
  begin
    if IsDarkInstallMode then
      Result := $2b2b2b
    else
      Result := $ffffff;
  end;
end;

function MessageWithURL(Message: TArrayOfString; const Title: String; ButtonText: TArrayOfString; const Typ: TMsgBoxType;
                        const DefaultButton, CancelButton: Integer): Integer; forward;

function GetSystemMetrics(nIndex: Integer): Integer; external 'GetSystemMetrics@User32 stdcall';
function GetDialogBaseUnits(): Integer; external 'GetDialogBaseUnits@User32 stdcall';
function ExtractIcon(hInstance: Integer; lpszExeFileName: String; nIconIndex: Integer): Integer; external 'ExtractIconW@shell32.dll stdcall';
function DrawIcon(hdc: HBitmap; x,y: Integer; hIcon: Integer): Integer; external 'DrawIcon@user32 stdcall';
function DrawIconEx(hdc: HBitmap; xLeft,yTop: Integer; hIcon: Integer; cxWidth, cyWidth: Integer; istepIfAniCur: Cardinal; hbrFlickerFreeDraw: Integer; diFlags: Cardinal): Integer; external 'DrawIconEx@user32 stdcall';
function DrawFocusRect(hDC: Integer; var lprc: TRect): BOOL; external 'DrawFocusRect@user32 stdcall';

type
  TArrayOfButton = Array of TNewButton;

const
  //borders around message
  MWU_LEFTBORDER = 25;
  MWU_RIGHTBORDER = MWU_LEFTBORDER;
  MWU_TOPBORDER = 26;
  MWU_BOTTOMBORDER = MWU_TOPBORDER;
    //space between elements (icon-text and between buttons)
  MWU_HORZSPACING = 8;
    //space between labels
  MWU_VERTSPACING = 4;
    //button sizes
  MWU_BUTTONHEIGHT = 24;
  MWU_MINBUTTONWIDTH = 86;
    //height of area where buttons are placed
  MWU_BUTTONAREAHEIGHT = 45;

  SM_CXSCREEN = 0;
  SM_CXICON = 11;
  SM_CYICON = 12;
  SM_CXICONSPACING = 38;
  SM_CYICONSPACING = 39;

  //COLOR_HOTLIGHT = 26;

  LR_DEFAULTSIZE = $00000040;
  LR_SHARED = $00008000;

  IMAGE_BITMAP = 0;
  IMAGE_ICON = 1;
  IMAGE_CURSOR = 2;

  DI_IMAGE = 1;
  DI_MASK = 2;
  DI_NORMAL = DI_IMAGE or DI_MASK;
  DI_DEFAULTSIZE = 8;

var
  URLList: TArrayOfString;
  TextLabel: Array of TNewStaticText;
  URLFocusImg: Array of TBitmapImage;
  SingleLineHeight: Integer;


procedure UrlClick(Sender: TObject);
var ErrorCode: Integer;
begin
  ShellExecAsOriginalUser('open',URLList[TNewStaticText(Sender).Tag],'','',SW_SHOWNORMAL,ewNoWait,ErrorCode);
end;


// calculates maximum width of text labels
// also counts URLs, and sets the length of URLList accordingly
function Message_CalcLabelWidth(var Message: TArrayOfString; MessageForm: TSetupForm): Integer;
var MeasureLabel: TNewStaticText;
  i,URLCount,DlgUnit,ScreenWidth: Integer;
begin
  MeasureLabel := TNewStaticText.Create(MessageForm);
  with MeasureLabel do
  begin
    Parent := MessageForm;
    Left := 0;
    Top := 0;
    AutoSize := True;
  end;

  MeasureLabel.Caption := 'X';
  SingleLineHeight := MeasureLabel.Height;

  Result := 0; //minimum width
  URLCount := 0;
  for i := 0 to GetArrayLength(Message) - 1 do
  begin
    if Length(Message[i]) < 1 then //simplifies things
      Message[i] := ' ';

    if Message[i][1] <> '_' then
      MeasureLabel.Caption := Message[i] //not an URL
    else
    begin //URL - check only the displayed text
      if Pos(' ',Message[i]) > 0 then
        MeasureLabel.Caption := Copy(Message[i],Pos(' ',Message[i])+1,Length(Message[i]))
      else
        MeasureLabel.Caption := Copy(Message[i],2,Length(Message[i]));

      URLCount := URLCount + 1;
    end;

    if MeasureLabel.Width > Result then
      Result := MeasureLabel.Width;
  end;
  MeasureLabel.Free;

  SetArrayLength(URLList,URLCount); //needed later - no need to do a special loop just for this
  SetArrayLength(URLFocusImg,URLCount);

  DlgUnit := GetDialogBaseUnits() and $FFFF;  //ensure the dialog isn't too wide
  ScreenWidth := GetSystemMetrics(SM_CXSCREEN);
  if Result > ((278 * DlgUnit) div 4) then //278 is from http://blogs.msdn.com/b/oldnewthing/archive/2011/06/24/10178386.aspx
    Result := ((278 * DlgUnit) div 4);
  if Result > (ScreenWidth * 3) div 4 then
    Result := (ScreenWidth * 3) div 4;

end;


//find the longest button
function Message_CalcButtonWidth(const ButtonText: TArrayOfString; MessageForm: TSetupForm): Integer;
var MeasureLabel: TNewStaticText;
  i: Integer;
begin
  MeasureLabel := TNewStaticText.Create(MessageForm);
  with MeasureLabel do
  begin
    Parent := MessageForm;
    Left := 0;
    Top := 0;
    AutoSize := True;
  end;

  Result := ScaleX(MWU_MINBUTTONWIDTH - MWU_HORZSPACING * 2); //minimum width
  for i := 0 to GetArrayLength(ButtonText) - 1 do
  begin
    MeasureLabel.Caption := ButtonText[i]

    if MeasureLabel.Width > Result then
      Result := MeasureLabel.Width;
  end;
  MeasureLabel.Free;

  Result := Result + ScaleX(MWU_HORZSPACING * 2); //account for borders
end;


procedure Message_Icon(const Typ: TMsgBoxType; TypImg: TBitmapImage);
var TypRect: TRect;
  Icon: THandle;
  TypIcon: Integer;
begin
  TypRect.Left := 0;
  TypRect.Top := 0;
  TypRect.Right := GetSystemMetrics(SM_CXICON);
  TypRect.Bottom := GetSystemMetrics(SM_CYICON);

  //Icon index from imageres.dll
  case Typ of
  mbInformation:
    TypIcon := 76;
  mbConfirmation:
    TypIcon := 94;
  mbError:
    TypIcon := 79;
  else
    TypIcon := 93;
  end;

  //TODO: icon loads with wrong size when using Large Fonts (SM_CXICON/CYICON is 40, but 32x32 icon loads - find out how to get the right size)
  Icon := ExtractIcon(0,'imageres.dll',TypIcon)
  //Icon := LoadIcon(0,TypIcon);
  //Icon := LoadImage(0,TypIcon,IMAGE_ICON,0,0,LR_SHARED or LR_DEFAULTSIZE);
  with TypImg do
  begin
    Left := ScaleX(MWU_LEFTBORDER);
    Top := ScaleY(MWU_TOPBORDER);
    Center := False;
    Stretch := False;
    AutoSize := True;
    Bitmap.Width := GetSystemMetrics(SM_CXICON);
    Bitmap.Height := GetSystemMetrics(SM_CYICON);
    Bitmap.Canvas.Brush.Color := GetThemedBgColor;
    Bitmap.Canvas.FillRect(TypRect);
    DrawIcon(Bitmap.Canvas.Handle,0,0,Icon); //draws icon scaled
    //DrawIconEx(Bitmap.Canvas.Handle,0,0,Icon,0,0,0,0,DI_NORMAL {or DI_DEFAULTSIZE}); //draws icon without scaling
  end;
  //DestroyIcon(Icon); //not needed with LR_SHARED or with LoadIcon
end;


procedure Message_SetUpURLLabel(URLLabel: TNewStaticText; const Msg: String; const URLNum: Integer);
var Blank: TRect;
begin
  with URLLabel do
  begin
    if Pos(' ',Msg) > 0 then
    begin
      Caption := Copy(Msg,Pos(' ',Msg)+1,Length(Msg));
      URLList[URLNum] := Copy(Msg, 2, Pos(' ',Msg)-1);
    end
    else
    begin //no text after URL - display just URL
      URLList[URLNum] := Copy(Msg, 2, Length(Msg));
      Caption := URLList[URLNum];
    end;

    Hint := URLList[URLNum];
    ShowHint := True;

    Font.Color := GetSysColor(COLOR_HOTLIGHT);
    Font.Style := [fsUnderline];
    Cursor := crHand;
    OnClick := @UrlClick;

    Tag := URLNum; //used to find the URL to open and bitmap to draw focus rectangle on

    if Height = SingleLineHeight then //shrink label to actual text width
      WordWrap := False;

    TabStop := True; //keyboard accessibility
    TabOrder := URLNum;
  end;

    URLFocusImg[URLNum] := TBitmapImage.Create(URLLabel.Parent); //focus rectangle needs a bitmap - prepare it here
  with URLFocusImg[URLNum] do
  begin
    Left := URLLabel.Left - 1;
    Top := URLLabel.Top - 1;
    Stretch := False;
    AutoSize := True;
    Parent := URLLabel.Parent;
    Bitmap.Width := URLLabel.Width + 2;
    Bitmap.Height := URLLabel.Height + 2;

    SendToBack;

    Blank.Left := 0;
    Blank.Top := 0;
    Blank.Right := Width;
    Blank.Bottom := Height;
    Bitmap.Canvas.Brush.Color := GetThemedBgColor;
    Bitmap.Canvas.FillRect(Blank);
  end;
end;


procedure Message_SetUpLabels(Message: TArrayOfString; TypImg: TBitmapImage;
                              const DialogTextWidth: Integer; MessagePanel: TPanel);
var i,URLNum,dy: Integer;
begin
  SetArrayLength(TextLabel,GetArrayLength(Message));
  URLNum := 0;
  for i := 0 to GetArrayLength(TextLabel) - 1 do
  begin
    TextLabel[i] := TNewStaticText.Create(MessagePanel);
    with TextLabel[i] do
    begin
      Parent := MessagePanel;
      Left := TypImg.Left + TypImg.Width + ScaleX(MWU_HORZSPACING);
      if i = 0 then
        Top := TypImg.Top
      else
        Top := TextLabel[i-1].Top + TextLabel[i-1].Height + ScaleY(MWU_VERTSPACING);

      WordWrap := True;
      AutoSize := True;
      Width := DialogTextWidth;

      if Message[i][1] <> '_' then
        Caption := Message[i]
      else
      begin // apply URL formatting
        Message_SetUpURLLabel(TextLabel[i], Message[i], URLNum);
        URLNum := URLNum + 1;
      end;

    end;
  end;

  i := GetArrayLength(TextLabel) - 1;
  if TextLabel[i].Top + TextLabel[i].Height < TypImg.Top + TypImg.Height then //center labels vertically
  begin
    dy := (TypImg.Top + TypImg.Height - TextLabel[i].Top - TextLabel[i].Height) div 2;
    for i := 0 to GetArrayLength(TextLabel) - 1 do
      TextLabel[i].Top := TextLabel[i].Top + dy;
  end;
end;


procedure Message_SetUpButtons(var Button: TArrayOfButton; ButtonText: TArrayOfString;
                              const ButtonWidth, DefaultButton, CancelButton: Integer; MessageForm: TSetupForm);
var i: Integer;
begin
  SetArrayLength(Button,GetArrayLength(ButtonText));
  for i := 0 to GetArrayLength(Button) - 1 do
  begin
    Button[i] := TNewButton.Create(MessageForm);
    with Button[i] do
    begin
      Parent := MessageForm;
      Width := ButtonWidth;
      Height := ScaleY(MWU_BUTTONHEIGHT);

      if i = 0 then
      begin
        Left := MessageForm.ClientWidth - (ScaleX(MWU_HORZSPACING) + ButtonWidth) * GetArrayLength(ButtonText);
        Top := MessageForm.ClientHeight - ScaleY(MWU_BUTTONAREAHEIGHT) +
               ScaleY(MWU_BUTTONAREAHEIGHT - MWU_BUTTONHEIGHT) div 2;
      end else
      begin
        Left := Button[i-1].Left + ScaleX(MWU_HORZSPACING) + ButtonWidth;
        Top := Button[i-1].Top;
      end;

      Caption := ButtonText[i];
      ModalResult := i + 1;

      //set the initial focus to the default button
      TabOrder := ((i - (DefaultButton - 1)) + GetArrayLength(Button)) mod (GetArrayLength(Button));

      if DefaultButton = i + 1 then
        Default := True;

      if CancelButton = i + 1 then
        Cancel := True;

    end;
  end;
end;


//find out if URL label has focus
//draw focus rectangle around it if so and return index of focused label
function Message_FocusLabel(): Integer;
var i: Integer;
  FocusRect: TRect;
begin
  Result := -1;

  for i := 0 to GetArrayLength(URLFocusImg) - 1 do //clear existing focus rectangle
  begin
    FocusRect.Left := 0;
    FocusRect.Top := 0;
    FocusRect.Right := URLFocusImg[i].Bitmap.Width;
    FocusRect.Bottom := URLFocusImg[i].Bitmap.Height;
    URLFocusImg[i].Bitmap.Canvas.FillRect(FocusRect);
  end;

  for i := 0 to GetArrayLength(TextLabel) - 1 do
  begin
    if TextLabel[i].Focused then
    begin
      Result := i;

      FocusRect.Left := 0;
      FocusRect.Top := 0;
      FocusRect.Right := URLFocusImg[TextLabel[i].Tag].Bitmap.Width;
      FocusRect.Bottom := URLFocusImg[TextLabel[i].Tag].Bitmap.Height;

      DrawFocusRect(URLFocusImg[TextLabel[i].Tag].Bitmap.Canvas.Handle, FocusRect);
    end;
  end;
end;


//TNewStaticText doesn't have OnFocus - handle that here
//(not perfect - if you focus label with keyboard, then focus a button with mouse, the label keeps it's underline)
procedure Message_KeyUp(Sender: TObject; var Key: Word; Shift: TShiftState);
var URLIdx: Integer;
begin
  case Key of
  9,37..40: //tab, arrow keys
    begin
      Message_FocusLabel();
    end;
  13,32: //enter, spacebar
    begin
      URLIdx := Message_FocusLabel(); //get focused label
      if URLIdx > -1 then
        UrlClick(TextLabel[URLIdx]);
    end;
  end;
end;


function MessageWithURL(Message: TArrayOfString; const Title: String; ButtonText: TArrayOfString; const Typ: TMsgBoxType;
                        const DefaultButton, CancelButton: Integer): Integer;
var MessageForm: TSetupForm;
  Button: TArrayOfButton;
  DialogTextWidth, ButtonWidth: Integer;
  MessagePanel: TPanel;
  TypImg: TBitmapImage;
  i: Integer;
begin
  if (not IsUninstaller and WizardSilent) or (IsUninstaller and UninstallSilent) then
  begin
    Result := DefaultButton;
    exit;
  end;

  MessageForm := CreateCustomForm(ScaleX(256), ScaleY(128), False, True);

  MessageForm.Caption := Title;
  if (CancelButton = 0) or (CancelButton > GetArrayLength(ButtonText)) then //no cancel button - remove close button
    MessageForm.BorderIcons := MessageForm.BorderIcons - [biSystemMenu];

  MessagePanel := TPanel.Create(MessageForm); //Vista-style background
  with MessagePanel do
  begin
    Parent := MessageForm;
    BevelInner := bvNone;
    BevelOuter := bvNone;
    BevelWidth := 0;
    ParentBackground := False;
    Color := clWindow;
    Left := 0;
    Top := 0;
  end;

  DialogTextWidth := Message_CalcLabelWidth(Message, MessageForm);
  ButtonWidth := Message_CalcButtonWidth(ButtonText, MessageForm);

  TypImg := TBitmapImage.Create(MessagePanel);
  TypImg.Parent := MessagePanel;
  Message_Icon(Typ, TypImg);

  Message_SetUpLabels(Message, TypImg, DialogTextWidth, MessagePanel);

  i := GetArrayLength(TextLabel) - 1;
  MessagePanel.ClientHeight := TextLabel[i].Top + TextLabel[i].Height + ScaleY(MWU_BOTTOMBORDER);

  MessagePanel.ClientWidth := DialogTextWidth + TypImg.Width + TypImg.Left + ScaleX(MWU_HORZSPACING + MWU_RIGHTBORDER);
  if MessagePanel.ClientWidth <
     (ButtonWidth + ScaleX(MWU_HORZSPACING)) * GetArrayLength(ButtonText) + ScaleX(MWU_HORZSPACING) then //ensure buttons fit
    MessagePanel.ClientWidth := (ButtonWidth + ScaleX(MWU_HORZSPACING)) * GetArrayLength(ButtonText) + ScaleX(MWU_HORZSPACING);

  MessageForm.ClientWidth := MessagePanel.Width;
  MessageForm.ClientHeight := MessagePanel.Height + ScaleY(MWU_BUTTONAREAHEIGHT);

  Message_SetUpButtons(Button, ButtonText, ButtonWidth, DefaultButton, CancelButton, MessageForm);

  MessageForm.OnKeyUp := @Message_KeyUp; //needed for keyboard access of URL labels
  MessageForm.KeyPreView := True;

  Result := MessageForm.ShowModal;

  for i := 0 to GetArrayLength(TextLabel) - 1 do
    TextLabel[i].Free;
  SetArrayLength(TextLabel,0);
  for i := 0 to GetArrayLength(URLFocusImg) - 1 do
    URLFocusImg[i].Free;
  SetArrayLength(URLFocusImg,0);

  MessageForm.Release;
end;


//0. PRELIMINARY SETUP CODE

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

//Check what type of installation is being done
procedure CheckInstallType;
var
  isInstalled: String;
  InstallLocation: String;
  Installed_AppVersion: String;
  Installed_AppVersionInt: Int64;
  Installer_AppVersionInt: Int64;
  ErrorCode: Integer;
begin
  isInstalled := 'notInstalled';
  if RegQueryStringValue(HKCU, 'Software\Microsoft\Windows\CurrentVersion\Uninstall\GIMP-{#GIMP_MUTEX_VERSION}_is1',
                           'DisplayVersion', Installed_AppVersion) then begin
      RegQueryStringValue(HKCU, 'Software\Microsoft\Windows\CurrentVersion\Uninstall\GIMP-{#GIMP_MUTEX_VERSION}_is1',
                           'InstallLocation', InstallLocation);
    StrToVersion(Installed_AppVersion, Installed_AppVersionInt);
    isInstalled := 'Installed';
  end;
  if RegQueryStringValue(HKLM, 'Software\Microsoft\Windows\CurrentVersion\Uninstall\GIMP-{#GIMP_MUTEX_VERSION}_is1',
                           'DisplayVersion', Installed_AppVersion) then begin
      RegQueryStringValue(HKLM, 'Software\Microsoft\Windows\CurrentVersion\Uninstall\GIMP-{#GIMP_MUTEX_VERSION}_is1',
                           'InstallLocation', InstallLocation);
      StrToVersion(Installed_AppVersion, Installed_AppVersionInt);
    isInstalled := 'Installed';
  end;

  StrToVersion('{#FULL_GIMP_VERSION}', Installer_AppVersionInt);

  if (isInstalled = 'Installed') and not DirExists(ExtractFilePath(RemoveBackslashUnlessRoot(InstallLocation))) then begin
        InstallType := 'itRepair';
    end else if isInstalled = 'notInstalled' then begin
      InstallType := 'itInstall';
  end else if ComparePackedVersion(Installer_AppVersionInt, Installed_AppVersionInt) = 0 then begin
    InstallType := 'itReinstall';
    end else if ComparePackedVersion(Installer_AppVersionInt, Installed_AppVersionInt) > 0 then begin
      InstallType := 'itUpdate';
    end else begin
      InstallType := 'itDowngrade';
  end;
  DebugMsg('CheckInstallType','Installed GIMP {#GIMP_MUTEX_VERSION} is: ' + Installed_AppVersion + ', installer is: {#FULL_GIMP_VERSION}. So Install type is: ' + InstallType);

  //Inno does not support direct downgrade so let's block it to not break installs
  if (not WizardSilent) and (InstallType = 'itDowngrade') then begin
      if SuppressibleMsgBox(FmtMessage(CustomMessage('DowngradeError'), [Installed_AppVersion, '{#FULL_GIMP_VERSION}']), mbCriticalError, MB_OK, IDOK) = IDOK then begin
      ShellExecAsOriginalUser('','ms-settings:appsfeatures','','',SW_SHOW,ewNoWait,ErrorCode);
      Abort;
    end;
  end else if (WizardSilent) and (InstallType = 'itDowngrade') then begin
      DebugMsg('CheckInstallType',CustomMessage('DowngradeError'));
    Abort;
  end;
end;

function InitializeSetup(): Boolean;
#if Defined(GIMP_UNSTABLE) || Defined(GIMP_RC_VERSION) || !Defined(GIMP_RELEASE)|| Defined(DEVEL_WARNING)
var Message,Buttons: TArrayOfString;
#endif
begin
  CheckInstallType;

  ConfigOverride := coUndefined;

  Result := BPPTooLowWarning();

//Unstable version warning
#if Defined(GIMP_UNSTABLE) || Defined(GIMP_RC_VERSION) || !Defined(GIMP_RELEASE) || Defined(DEVEL_WARNING)
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
    ExtractTemporaryFile('installsplash_top.scale-100.bmp');
    ExtractTemporaryFile('installsplash_top.scale-125.bmp');
    ExtractTemporaryFile('installsplash_top.scale-150.bmp');
    ExtractTemporaryFile('installsplash_top.scale-175.bmp');
    ExtractTemporaryFile('installsplash_top.scale-200.bmp');
    ExtractTemporaryFile('installsplash_top.scale-225.bmp');
    ExtractTemporaryFile('installsplash_top.scale-250.bmp');
    ExtractTemporaryFile('installsplash_bottom.bmp');
  except
    DebugMsg('InitializeSetup','Error extracting temporary file: ' + GetExceptionMessage);
    MsgBox(CustomMessage('ErrorExtractingTemp') + #13#13 + GetExceptionMessage,mbError,MB_OK);
    Result := False;
    exit;
  end;
end;


//1. WELCOME: add splash image with buttons in non-default positions
var
  WelcomeBitmapTop: TBitmapImage;
  WelcomeBitmapBottom: TBitmapImage;
  btnInstall, btnCustomize: TNewButton;

procedure UpdateWizardImages();
var TopBitmap,BottomBitmap: TFileStream;
begin
    if not WizardSilent then begin
    //Automatically scaled splash image
    WelcomeBitmapTop := TBitmapImage.Create(WizardForm.WelcomePage);
    with WelcomeBitmapTop do
    begin
      Parent := WizardForm.WelcomePage;
      Width := WizardForm.WelcomePage.ClientWidth
      Height := {#GIMP_SPLASH_HEIGHT} * Width / {#GIMP_SPLASH_WIDTH}
      Left := 0;
      Top := (WizardForm.ClientHeight - Height) / 2;
      AutoSize := False;
      Stretch := True;
      Center := True;
    end;
    try
      if WelcomeBitmapTop.Height <= 314 then begin
        TopBitmap := TFileStream.Create(ExpandConstant('{tmp}\installsplash_top.scale-100.bmp'),fmOpenRead);
      end else if WelcomeBitmapTop.Height <= 386 then begin
        TopBitmap := TFileStream.Create(ExpandConstant('{tmp}\installsplash_top.scale-125.bmp'),fmOpenRead);
      end else if WelcomeBitmapTop.Height <= 459 then begin
        TopBitmap := TFileStream.Create(ExpandConstant('{tmp}\installsplash_top.scale-150.bmp'),fmOpenRead);
      end else if WelcomeBitmapTop.Height <= 556 then begin
        TopBitmap := TFileStream.Create(ExpandConstant('{tmp}\installsplash_top.scale-175.bmp'),fmOpenRead);
      end else if WelcomeBitmapTop.Height <= 604 then begin
        TopBitmap := TFileStream.Create(ExpandConstant('{tmp}\installsplash_top.scale-200.bmp'),fmOpenRead);
      end else if WelcomeBitmapTop.Height <= 700 then begin
        TopBitmap := TFileStream.Create(ExpandConstant('{tmp}\installsplash_top.scale-225.bmp'),fmOpenRead);
      end else begin
        TopBitmap := TFileStream.Create(ExpandConstant('{tmp}\installsplash_top.scale-250.bmp'),fmOpenRead);
      end;
      DebugMsg('UpdateWizardImages','Height: ' + IntToStr(WelcomeBitmapTop.Height));
      WizardForm.WizardBitmapImage.Bitmap.LoadFromStream(TopBitmap);
      WelcomeBitmapTop.Bitmap := WizardForm.WizardBitmapImage.Bitmap;
    except
      DebugMsg('UpdateWizardImages','Error loading image: ' + GetExceptionMessage);
    finally
      if TopBitmap <> nil then
        TopBitmap.Free;
    end;
    WizardForm.WelcomePage.Color := clNone;

    //Blurred background
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
    try
      BottomBitmap := TFileStream.Create(ExpandConstant('{tmp}\installsplash_bottom.bmp'),fmOpenRead);
      WizardForm.WizardBitmapImage.Bitmap.LoadFromStream(BottomBitmap);
      WelcomeBitmapBottom.Bitmap := WizardForm.WizardBitmapImage.Bitmap;
    except
      DebugMsg('UpdateWizardImages','Error loading image: ' + GetExceptionMessage);
    finally
      if BottomBitmap <> nil then
        BottomBitmap.Free;
    end;
    WizardForm.WizardBitmapImage.Width := WizardForm.ClientWidth;
    WizardForm.WizardBitmapImage.Height := WizardForm.ClientHeight;
    end;
end;

procedure PrepareWelcomePage();
begin
  if not WizardSilent then
  begin
    //Inno does not support "repairing" a lost install so let's show Customize button to allow to repair installs
    //Inno does not support changing components at reinstall or update so let's hide Customize to not break installs
    WizardForm.NextButton.Visible := False;
    if not (InstallType = 'itRepair') then begin
        btnInstall.Visible := True;
    end;
    btnInstall.TabOrder := 1;
    if (InstallType = 'itRepair') or (InstallType = 'itInstall') then begin
        btnCustomize.Visible := True;
    end;

    WizardForm.Bevel.Visible := False;
    WizardForm.WelcomeLabel1.Visible := False;
    WizardForm.WelcomeLabel2.Visible := False;
    WelcomeBitmapBottom.Visible := True;
  end;
end;

procedure CleanUpCustomWelcome();
begin
  WizardForm.NextButton.Visible := True;
  if not (InstallType = 'itRepair') then begin
       btnInstall.Visible := False;
  end;
  if (InstallType = 'itRepair') or (InstallType = 'itInstall') then begin
       btnCustomize.Visible := False;
    end;

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
var
  MeasureLabel: TNewStaticText;
  //lblInfo: TNewStaticText;
begin
  DebugMsg('InitCustomPages','wpLicense');

  CheckInstallType;

  //Install-Reinstall-Update button
  btnInstall := TNewButton.Create(WizardForm);
  with btnInstall do
  begin
    Parent := WizardForm;
    Width := WizardForm.NextButton.Width;
    Height := WizardForm.NextButton.Height;
    Left := WizardForm.NextButton.Left;
    Top := WizardForm.NextButton.Top;
    Default := True;
    Visible := False;
    if InstallType = 'itInstall' then begin
        Caption := CustomMessage('Install');
      end else if InstallType = 'itReinstall' then begin
        Caption := CustomMessage('Reinstall');
      end else if InstallType = 'itUpdate' then begin
        Caption := CustomMessage('Update');
    end;

    OnClick := @InstallOnClick;
  end;

  //Customize-Repair button
  MeasureLabel := TNewStaticText.Create(WizardForm);
  with MeasureLabel do
  begin
    Parent := WizardForm;
    Left := 0;
    Top := 0;
    AutoSize := True;
    if InstallType = 'itRepair' then begin
        Caption := CustomMessage('Repair');
    end else if InstallType = 'itInstall' then begin
        Caption := CustomMessage('Customize');
    end;
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
    if InstallType = 'itRepair' then begin
        Caption := CustomMessage('Repair');
    end else if InstallType = 'itInstall' then begin
        Caption := CustomMessage('Customize');
    end;

    OnClick := @CustomizeOnClick;
  end;
  MeasureLabel.Free;
end;


//2. LICENSE
procedure InfoBeforeLikeLicense();
begin
    if not IsDarkInstallMode then begin
      WizardForm.Bevel.Visible := False;
  end;

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

  Components := ['Gimp','Deps','Debug','Dev','Ghostscript','Lua','Python','Translations','MyPaint'];
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
var i: Integer;
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
  RTFPara   = '\par ';
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
procedure PreparingFaceLift();
begin
    if not IsDarkInstallMode then begin
      WizardForm.Bevel.Visible := False;
  end;
end;

//Create restore point
procedure RestorePoint();
var
  ResultCode: Integer;
begin
  StatusLabel(CustomMessage('CreatingRestorePoint'),'');
  if not ShellExec('RunAs', 'powershell', '-Command "$job = Start-Job -ScriptBlock { Checkpoint-Computer -Description "GIMP_' + ExpandConstant('{#CUSTOM_GIMP_VERSION}') + '_install" -RestorePointType APPLICATION_INSTALL }; Wait-Job $job -Timeout 24; if ($job.State -eq \"Running\") { Stop-Job $job -Confirm:$false }; Receive-Job $job"',
                   '', SW_HIDE, ewWaitUntilTerminated, ResultCode) then
  begin
    DebugMsg('RestorePoint','Failed to create restore point. Error code: ' + IntToStr(ResultCode));
  end;
end;

//remove .pdb files from previous installs
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
          //Up to GIMP 3.0.2 we shipped only DWARF .debug symbols
          if (Length(FindRec.Name) > 6) and (LowerCase(Copy(FindRec.Name, Length(FindRec.Name) - 5, 6)) = '.debug') then
          begin
            DebugMsg('RemoveDebugFilesFromDir', '> ' + FindRec.Name);
            DeleteFile(AddBackSlash(pDir) + FindRec.Name);
          end;

          //Starting with GIMP 3.0.4 we ship native CodeView .pdb symbols
          if (Length(FindRec.Name) > 4) and (LowerCase(Copy(FindRec.Name, Length(FindRec.Name) - 3, 4)) = '.pdb') then
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
function CheckArch(const pWhich: String): Boolean;
begin
  if pWhich = '64' then //x64 or arm64
    Result := Is64BitInstallMode()
  else if pWhich = 'arm64' then
    Result := Is64BitInstallMode() and IsARM64
  else if pWhich = 'x64' then
    Result := Is64BitInstallMode() and IsX64OS
  else
    RaiseException('Unknown check');
end;

//some programs improperly install libraries to the System32 directory, which then causes problems with plugins
//this function checks if such file exists in System32, and lets setup update the file when it exists
function BadSysDLL(const pFile: String; const pPlatform: Integer): Boolean;
var OldRedir: Boolean;
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
#if Defined(GIMP_UNSTABLE)
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
    if not IsDarkInstallMode then begin
      WizardForm.Bevel.Visible := False;
  end;

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
var InterpFile,InterpContent: String;
#ifdef LUA
  LuaBin: String;
#endif
begin
#ifdef PYTHON
  if WizardIsComponentSelected('pyARM64') or WizardIsComponentSelected('py64') then
  begin
    StatusLabel(CustomMessage('SettingUpPyGimp'),'');

    //python.exe is needed for plug-ins error output if `gimp*.exe` is run from console
    InterpFile := ExpandConstant('{app}\lib\gimp\{#GIMP_PKGCONFIG_VERSION}\interpreters\pygimp.interp');
        DebugMsg('PrepareInterp','Writing interpreter file for gimp-python: ' + InterpFile);
    InterpContent := 'python=' + ExpandConstant('{app}\bin\python.exe') + #10 +
                     'python3=' + ExpandConstant('{app}\bin\python.exe') + #10 +
                     '/usr/bin/python=' + ExpandConstant('{app}\bin\python.exe') + #10 +
                     '/usr/bin/python3=' + ExpandConstant('{app}\bin\python.exe') + #10 +
                     ':Python:E::py::python:'#10;
    if not SaveStringToUTF8File(InterpFile,InterpContent,False) then
    begin
      DebugMsg('PrepareInterp','Problem writing the file. [' + InterpContent + ']');
      SuppressibleMsgBox(CustomMessage('ErrorUpdatingPython') + ' (2)',mbInformation,mb_ok,IDOK);
    end;

    //pythonw.exe is needed to run plug-ins silently if `gimp*.exe` is run from shortcut
    InterpFile := ExpandConstant('{app}\lib\gimp\{#GIMP_PKGCONFIG_VERSION}\interpreters\pygimp_win.interp');
        DebugMsg('PrepareInterp','Writing interpreter file for gimp-python: ' + InterpFile);
    InterpContent := 'python=' + ExpandConstant('{app}\bin\pythonw.exe') + #10 +
                     'python3=' + ExpandConstant('{app}\bin\pythonw.exe') + #10 +
                     '/usr/bin/python=' + ExpandConstant('{app}\bin\pythonw.exe') + #10 +
                     '/usr/bin/python3=' + ExpandConstant('{app}\bin\pythonw.exe') + #10 +
                     ':Python:E::py::python:'#10;
    if not SaveStringToUTF8File(InterpFile,InterpContent,False) then
    begin
      DebugMsg('PrepareInterp','Problem writing the file. [' + InterpContent + ']');
      SuppressibleMsgBox(CustomMessage('ErrorUpdatingPython') + ' (2)',mbInformation,mb_ok,IDOK);
    end;
  end;
#endif

#ifdef LUA
  if WizardIsComponentSelected('luaARM64') or WizardIsComponentSelected('lua64') then
  begin
    InterpFile := ExpandConstant('{app}\lib\gimp\{#GIMP_PKGCONFIG_VERSION}\interpreters\lua.interp');
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
  InterpFile := ExpandConstant('{app}\lib\gimp\{#GIMP_PKGCONFIG_VERSION}\interpreters\gimp-script-fu-interpreter.interp');
  DebugMsg('PrepareInterp','Writing interpreter file for gimp-script-fu-interpreter: ' + InterpFile);
  InterpContent := 'gimp-script-fu-interpreter=' + ExpandConstant('{app}\bin\gimp-script-fu-interpreter-{#GIMP_PKGCONFIG_VERSION}.exe') + #10 +
                   'gimp-script-fu-interpreter-{#GIMP_PKGCONFIG_VERSION}=' + ExpandConstant('{app}\bin\gimp-script-fu-interpreter-{#GIMP_PKGCONFIG_VERSION}.exe') + #10 +
                   '/usr/bin/gimp-script-fu-interpreter=' + ExpandConstant('{app}\bin\gimp-script-fu-interpreter-{#GIMP_PKGCONFIG_VERSION}.exe') + #10 +
                   ',ScriptFu,E,,scm,,' + ExpandConstant('{app}\bin\gimp-script-fu-interpreter-{#GIMP_PKGCONFIG_VERSION}.exe') + ','#10;

  if not SaveStringToUTF8File(InterpFile,InterpContent,False) then
  begin
    DebugMsg('PrepareInterp','Problem writing the file. [' + InterpContent + ']');
    SuppressibleMsgBox(CustomMessage('ErrorUpdatingScriptFu') + ' (2)',mbInformation,mb_ok,IDOK);
  end;

  InterpFile := ExpandConstant('{app}\lib\gimp\{#GIMP_PKGCONFIG_VERSION}\interpreters\gimp-script-fu-interpreter_win.interp');
  DebugMsg('PrepareInterp','Writing interpreter file for gimp-script-fu-interpreter: ' + InterpFile);
  InterpContent := 'gimp-script-fu-interpreter=' + ExpandConstant('{app}\bin\gimp-script-fu-interpreter-{#GIMP_PKGCONFIG_VERSION}.exe') + #10 +
                   'gimp-script-fu-interpreter-{#GIMP_PKGCONFIG_VERSION}=' + ExpandConstant('{app}\bin\gimp-script-fu-interpreter-{#GIMP_PKGCONFIG_VERSION}.exe') + #10 +
                   '/usr/bin/gimp-script-fu-interpreter=' + ExpandConstant('{app}\bin\gimp-script-fu-interpreter-{#GIMP_PKGCONFIG_VERSION}.exe') + #10 +
                   ',ScriptFu,E,,scm,,' + ExpandConstant('{app}\bin\gimp-script-fu-interpreter-{#GIMP_PKGCONFIG_VERSION}.exe') + ','#10;

  if not SaveStringToUTF8File(InterpFile,InterpContent,False) then
  begin
    DebugMsg('PrepareInterp','Problem writing the file. [' + InterpContent + ']');
    SuppressibleMsgBox(CustomMessage('ErrorUpdatingScriptFu') + ' (2)',mbInformation,mb_ok,IDOK);
  end;
end;
end; //PrepareInterp

//Create .env files
procedure PrepareGimpEnvironment();
var EnvFile,Env,sTemp: String;
begin
  StatusLabel(CustomMessage('SettingUpEnvironment'),'');

  //set PATH to be used by plug-ins
  EnvFile := ExpandConstant('{app}\lib\gimp\{#GIMP_PKGCONFIG_VERSION}\environ\default.env');
  DebugMsg('PrepareGimpEnvironment','Setting environment in ' + EnvFile);

  Env := #10'PATH=${gimp_installation_dir}\bin';

  Env := Env + #10;

  DebugMsg('PrepareGimpEnvironment','Appending ' + Env);

  if not SaveStringToUTF8File(EnvFile,Env,True) then
  begin
    DebugMsg('PrepareGimpEnvironment','Problem appending');
    SuppressibleMsgBox(FmtMessage(CustomMessage('ErrorChangingEnviron'),[EnvFile]),mbInformation,mb_ok,IDOK);
  end;

  // Set revision
  EnvFile := ExpandConstant('{app}\share\gimp\{#GIMP_PKGCONFIG_VERSION}\gimp-release');
  DebugMsg('PrepareGimpEnvironment','Setting revision number {#REVISION} in ' + EnvFile);

  //LoadStringFromUTF8File(EnvFile,Env);
  //sTemp := Replace('=0','={#REVISION}',Env);
  sTemp := '[package]' + #10 + 'revision={#REVISION}' + #10

  if not SaveStringToUTF8File(EnvFile,sTemp,False) then
  begin
    DebugMsg('PrepareGimpEnvironment','Problem setting revision');
    SuppressibleMsgBox(FmtMessage(CustomMessage('ErrorChangingEnviron'),[EnvFile]),mbInformation,mb_ok,IDOK);
  end;

  // Disable check-update when run with specific option
  if ExpandConstant('{param:disablecheckupdate|false}') = 'true' then
  begin
    EnvFile := ExpandConstant('{app}\share\gimp\{#GIMP_PKGCONFIG_VERSION}\gimp-release');
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

var
  asUninstInf: TArrayOfString; //uninst.inf contents (loaded at start of uninstall, executed at the end)

function SplitRegParams(const pInf: String; var oRootKey: Integer; var oKey,oValue: String): Boolean;
var sRootKey: String;
  d: Integer;
begin
  Result := False;

  d := Pos('/',pInf);
  if d = 0 then
  begin
    DebugMsg('SplitRegParams','Malformed line (missing /): ' + pInf);
    exit;
  end;

  sRootKey := Copy(pInf,1,d - 1);
  oKey := Copy(pInf,d + 1,Length(pInf));

  if oValue <> 'nil' then
  begin
    d := RevPos('\',oKey);
    if d = 0 then
    begin
      DebugMsg('SplitRegParams','Malformed line (missing \): ' + pInf);
      exit;
    end;

    oValue := Decode(Copy(oKey,d+1,Length(oKey)));
    oKey := Copy(oKey,1,d-1);
  end;

  DebugMsg('SplitRegParams','Root: '+sRootKey+', Key:'+oKey + ', Value:'+oValue);

  case sRootKey of
  'HKCR': oRootKey := HKCR;
  'HKLM': oRootKey := HKLM;
  'HKU': oRootKey := HKU;
  'HKCU': oRootKey := HKCU;
  else
    begin
      DebugMsg('SplitRegParams','Unrecognised root key: ' + sRootKey);
      exit;
    end;
  end;

  Result := True;
end;


procedure UninstInfRegKey(const pInf: String; const pIfEmpty: Boolean);
var sKey,sVal: String;
  iRootKey: Integer;
begin
  sVal := 'nil';
  if not SplitRegParams(pInf,iRootKey,sKey,sVal) then
    exit;

  if pIfEmpty then
  begin
    if not RegDeleteKeyIfEmpty(iRootKey,sKey) then
      DebugMsg('UninstInfRegKey','RegDeleteKeyIfEmpty failed');
  end
  else
  begin
    if not RegDeleteKeyIncludingSubkeys(iRootKey,sKey) then
      DebugMsg('UninstInfRegKey','RegDeleteKeyIncludingSubkeys failed');
  end;
end;


procedure UninstInfRegVal(const pInf: String);
var sKey,sVal: String;
  iRootKey: Integer;
begin
  if not SplitRegParams(pInf,iRootKey,sKey,sVal) then
    exit;

  if not RegDeleteValue(iRootKey,sKey,sVal) then
    DebugMsg('UninstInfREG','RegDeleteKeyIncludingSubkeys failed');
end;


procedure UninstInfFile(const pFile: String);
begin
  DebugMsg('UninstInfFile','File: '+pFile);

  if not DeleteFile(pFile) then
    DebugMsg('UninstInfFile','DeleteFile failed');
end;


procedure UninstInfDir(const pDir: String);
begin
  DebugMsg('UninstInfDir','Dir: '+pDir);

  if not RemoveDir(pDir) then
    DebugMsg('UninstInfDir','RemoveDir failed');
end;


procedure CreateMessageForm(var frmMessage: TForm; const pMessage: String);
var lblMessage: TNewStaticText;
begin
  frmMessage := CreateCustomForm(ScaleX(256), ScaleY(128), False, True);
  with frmMessage do
  begin
    BorderStyle := bsDialog;

    ClientWidth := ScaleX(300);
    ClientHeight := ScaleY(48);

    Caption := CustomMessage('UninstallingAddOnCaption');

    Position := poScreenCenter;

    BorderIcons := [];
  end;

  lblMessage := TNewStaticText.Create(frmMessage);
  with lblMessage do
  begin
    Parent := frmMessage;
    AutoSize := True;
    Caption := pMessage;
    Top := (frmMessage.ClientHeight - Height) div 2;
    Left := (frmMessage.ClientWidth - Width) div 2;
    Visible := True;
  end;

  frmMessage.Show();

    frmMessage.Refresh();
end;


procedure UninstInfRun(const pInf: String);
var Description,Prog,Params: String;
  Split, ResultCode, Ctr: Integer;
  frmMessage: TForm;
begin
  DebugMsg('UninstInfRun',pInf);

  Split := Pos('/',pInf);
  if Split <> 0 then
  begin
    Description := Copy(pInf, 1, Split - 1);
    Prog := Copy(pInf, Split + 1, Length(pInf));
  end else
  begin
    Prog := pInf;
    Description := '';
  end;

  Split := Pos('/',Prog);
  if Split <> 0 then
  begin
    Params := Copy(Prog, Split + 1, Length(Prog));
    Prog := Copy(Prog, 1, Split - 1);
  end else
  begin
    Params := '';
  end;

  if not UninstallSilent then //can't manipulate uninstaller messages, so create a form instead
    CreateMessageForm(frmMessage,Description);

  DebugMsg('UninstInfRun','Running: ' + Prog + '; Params: ' + Params);

  if Exec(Prog,Params,'',SW_SHOW,ewWaitUntilTerminated,ResultCode) then
  begin
    DebugMsg('UninstInfRun','Exec result: ' + IntToStr(ResultCode));

    Ctr := 0;
    while FileExists(Prog) do //wait a few seconds for the uninstaller to be deleted - since this is done by a program
    begin                     //running from a temporary directory, the uninstaller we ran above will exit some time before
      Sleep(UNINSTALL_CHECK_TIME);           //it's removed from disk
      Inc(Ctr);
      if Ctr = (UNINSTALL_MAX_WAIT_TIME/UNINSTALL_CHECK_TIME) then //don't wait more than 5 seconds
        break;
    end;

  end else
    DebugMsg('UninstInfRun','Exec failed: ' + IntToStr(ResultCode) + ' (' + SysErrorMessage(ResultCode) + ')');

  if not UninstallSilent then
    frmMessage.Free();
end;

(*
uninst.inf documentation:

- Delete Registry keys (with all subkeys):
  RegKey:<RootKey>/<SubKey>
    RootKey = HKCR, HKLM, HKCU, HKU
    SubKey = subkey to delete (warning: this will delete all keys under subkey, so be very careful with it)

- Delete empty registry keys:
  RegKeyEmpty:<RootKey>/<SubKey>
    RootKey = HKCR, HKLM, HKCU, HKU
    SubKey = subkey to delete if empty

- Delete values from registry:
  RegVal:<RootKey>/<SubKey>/Value
    RootKey = HKCR, HKLM, HKCU, HKU
    SubKey = subkey to delete Value from
    Value = value to delete; \ and % must be escaped as %5c and %25

- Delete files:
  File:<Path>
    Path = full path to file

- Delete empty directories:
  Dir:<Path>

- Run program with parameters:
  Run:<description>/<path>/<params>

Directives are parsed from the end of the file backwards as the first step of uninstall.

IMPORTANT: From GIMP 2.10.12 onwards (Inno Setup 6 with support for per-user install), Registry keys referring to HKCR will be
processed by the installer as the first step of install (and the entries will be removed from uninst.inf), since file associations
code was rewritten.

*)
procedure ParseUninstInf();
var i,d: Integer;
  sWhat: String;
begin
  for i := GetArrayLength(asUninstInf) - 1 downto 0 do
  begin

    DebugMsg('ParseUninstInf',asUninstInf[i]);

    if (Length(asUninstInf[i]) = 0) or (asUninstInf[i][1] = '#') then //skip comments and empty lines
      continue;

    d := Pos(':',asUninstInf[i]);
    if d = 0 then
    begin
      DebugMsg('ParseUninstInf','Malformed line: ' + asUninstInf[i]);
      continue;
    end;

    sWhat := Copy(asUninstInf[i],d+1,Length(asUninstInf[i]));

    case Copy(asUninstInf[i],1,d) of
    'RegKey:': UninstInfRegKey(sWhat,False);
    'RegKeyEmpty:': UninstInfRegKey(sWhat,True);
    'RegVal:': UninstInfRegVal(sWhat);
    'File:': UninstInfFile(sWhat);
    'Dir:': UninstInfDir(sWhat);
    'Run:': UninstInfRun(sWhat);
    end;

  end;

end;

procedure RestorePointU();
var
  ResultCode: Integer;
begin
  UninstallProgressForm.StatusLabel.Caption := CustomMessage('CreatingRestorePoint');
  if not ShellExec('RunAs', 'powershell', '-Command "$job = Start-Job -ScriptBlock { Checkpoint-Computer -Description "GIMP_' + ExpandConstant('{#CUSTOM_GIMP_VERSION}') + '_uninstall" -RestorePointType APPLICATION_UNINSTALL }; Wait-Job $job -Timeout 24; if ($job.State -eq \"Running\") { Stop-Job $job -Confirm:$false }; Receive-Job $job"',
                   '', SW_HIDE, ewWaitUntilTerminated, ResultCode) then
  begin
    DebugMsg('RestorePointU','Failed to create restore point. Error code: ' + IntToStr(ResultCode));
  end;
end;

#define API_MAJOR=Copy(GIMP_PKGCONFIG_VERSION,1,Pos(".",GIMP_PKGCONFIG_VERSION)-1)

//Taken from https://github.com/GrayWorldFinex/InnoSetupScripts/blob/main/Src%20Code/cdx-gridberd.iss
Procedure MoveFile(SourceFile, DestFile: {#defined UNICODE ? "Ansi" : ""}String);
  Begin
   CopyFile(SourceFile, DestFile, False);
  End;

Function ProcessFiles(S: {#defined UNICODE ? "Ansi" : ""}String): array of {#defined UNICODE ? "Ansi" : ""}String;
  var
   i, k: Integer;
  Begin
   repeat
    i := Pos(',', S);
    k := GetArrayLength(Result);
    SetArrayLength(Result, k + 1);
    If (i > 0) then begin
     Result[k] := Copy(S, 1, i - 1);
     Delete(S, 1, i); end
    else begin
     Result[k] := S;
     SetLength(S, 0);
    end;
   until (Length(S) = 0);
  End;

Procedure CopyFiles(SourceDir, DestDir, ExcludeFiles: {#defined UNICODE ? "Ansi" : ""}String);
  var
   FSR, DSR: TFindRec;
   FindResult, Exclude: Boolean;
   SourceFile, DestFile:{#defined UNICODE ? "Ansi" : ""}String;
   i, k: Integer;
   SS: Array of {#defined UNICODE ? "Ansi" : ""}String;
  Begin
   SetArrayLength(SS, 0);
   SS := ProcessFiles(ExcludeFiles);
   k := GetArrayLength(SS);
   If not DirExists(DestDir) Then Begin
    CreateDir(DestDir);
   End;
   FindResult := FindFirst(AddBackslash(SourceDir) + '*.*', FSR);
   Try
    While FindResult Do Begin
     If FSR.Attributes and FILE_ATTRIBUTE_DIRECTORY = 0 Then Begin
      Exclude := False;
      For i := 0 To k - 1 Do Begin
       If AnsiLowercase(SS[i]) = AnsiLowercase(FSR.Name) Then Begin
        Exclude := True;
        Break;
       End;
      End;
      If not Exclude Then Begin
       SourceFile := AddBackslash(SourceDir) + FSR.Name;
       DestFile := AddBackslash(DestDir) + FSR.Name;
       MoveFile(SourceFile, DestFile);
      End;
     End;
     FindResult := FindNext(FSR);
    End;
    FindResult := FindFirst(AddBackslash(SourceDir) + '*.*', DSR);
    While FindResult Do Begin
     If ((DSR.Attributes and FILE_ATTRIBUTE_DIRECTORY) = FILE_ATTRIBUTE_DIRECTORY) and not ((DSR.Name = '.') or (DSR.Name = '..')) Then Begin
      CopyFiles(AddBackslash(SourceDir) + DSR.Name, AddBackslash(DestDir) + DSR.Name, ExcludeFiles);
     End;
     FindResult := FindNext(DSR);
    End;
   Finally
    FindClose(FSR);
    FindClose(DSR);
   End;
  End;

function Time(ATimerPtr: integer): integer; external '_time32@ucrtbase.dll cdecl';

procedure CurUninstallStepChanged(CurStep: TUninstallStep);
var
  gimp_directory: String;
begin
  DebugMsg('CurUninstallStepChanged','');
  case CurStep of
  usUninstall:
  begin
    //Try to make restore point before unninstalling (like before installing, see RestorePoint() on *gimp3264.iss)
    if IsAdminInstallMode() then begin
        RestorePointU();
      UninstallProgressForm.StatusLabel.Caption := FmtMessage(SetupMessage(msgStatusUninstalling),['GIMP']);
    end;

    LoadStringsFromFile(ExpandConstant('{app}\uninst\uninst.inf'),asUninstInf);
    ParseUninstInf();
  end;
  usPostUninstall:
  begin
    //Offer option to remove %AppData%/GIMP/GIMP_APP_VERSION dir so have a future clean install
    if not IsAdminInstallMode() then begin
            gimp_directory := GetEnv('GIMP{#API_MAJOR}_DIRECTORY');
      if gimp_directory = '' then begin
        gimp_directory := ExpandConstant('{userappdata}\GIMP\{#GIMP_APP_VERSION}');
      end;
      if DirExists(gimp_directory + '\') then begin
        if SuppressibleMsgBox(CustomMessage('UninstallConfig'), mbError, MB_YESNO or MB_DEFBUTTON2, IDNO) = IDYES then begin
          CopyFiles(gimp_directory, ExpandConstant('{userdesktop}\gimp_{#GIMP_APP_VERSION}_backup_' + Format('%d', [Time(0)])), '');
          DelTree(gimp_directory, True, True, True);
        end;
      end;
      end;
  end;
  end;
end;


procedure AssociationsCleanUp();
var i, d, countNew,countUI: Integer;
  asTemp, asNew: TArrayOfString;
  sKey,sVal: String;
  iRootKey: Integer;
begin
  if FileExists(ExpandConstant('{app}\uninst\uninst.inf')) then
  begin
    DebugMsg('AssociationsCleanUp','Parsing old uninst.inf');
    LoadStringsFromFile(ExpandConstant('{app}\uninst\uninst.inf'),asTemp);

    countNew := 0;
    countUI := 0;
    SetArrayLength(asNew, GetArrayLength(asTemp));
    SetArrayLength(asUninstInf, GetArrayLength(asTemp));

    for i := 0 to GetArrayLength(asTemp) - 1 do //extract associations-related entries from uninst.inf
    begin

      if (Length(asTemp[i]) = 0) or (asTemp[i][1] = '#') then //comment/empty line
      begin
        asNew[countNew] := asTemp[i];
        Inc(countNew);
        continue;
      end;

      d := Pos(':',asTemp[i]);
      if d = 0 then //something wrong, ignore
        continue;

      if Copy(asTemp[i],1,3) = 'Reg' then
      begin

        sVal := 'nil';
        if not SplitRegParams(Copy(asTemp[i], d + 1, Length(asTemp[i])),iRootKey,sKey,sVal) then
          continue; //malformed line, ignore

        if iRootKey = HKCR then //old association, prepare for cleanup
        begin
          DebugMsg('AssociationsCleanUp','Preparing for cleanup: '+asTemp[i]);
          asUninstInf[countUI] := asTemp[i];
          Inc(countUI);
          continue;
        end;

      end;

      //something else, keep for new uninst.inf
      asNew[countNew] := asTemp[i];
      Inc(countNew);

    end;

    SetArrayLength(asNew, countNew);
    SetArrayLength(asUninstInf, countUI);

    SaveStringsToUTF8File(ExpandConstant('{app}\uninst\uninst.inf'), asNew, False); //replace uninst.inf with a cleaned one

    ParseUninstInf(); //remove old associations
  end;
end;


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

  Result := (InstallMode = imSimple) and (pPageID <> wpFinished);
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
    wpPreparing:
      PreparingFaceLift();
    wpInstalling:
      InstallingFaceLift();
  end;
end;

procedure CurStepChanged(pCurStep: TSetupStep);
begin
  case pCurStep of
    ssInstall:
    begin
      //As on usUninstall, do not allow to cancel on itReinstall or itUpdate
      //since Inno does not undo [InstallDelete] and RemoveDebugFiles()
      if (InstallType = 'itReinstall') or (InstallType = 'itUpdate') then begin
        WizardForm.CancelButton.Enabled := False;
      end;

      if IsAdminInstallMode() then begin
        RestorePoint();
      end;
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


#expr SaveToFile(AddBackslash(SourcePath) + "Preprocessed.iss")
