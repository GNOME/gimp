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
;Get GIMP_MUTEX_VERSION (used for internal versioning control)
#if Defined(GIMP_UNSTABLE)
	#define GIMP_MUTEX_VERSION GIMP_APP_VERSION
#else
	#define GIMP_MUTEX_VERSION=Copy(GIMP_APP_VERSION,1,Pos(".",GIMP_APP_VERSION)-1)
#endif
;Get FULL_GIMP_VERSION (used by ITs)
#define ORIGINAL_GIMP_VERSION GIMP_VERSION
#if Defined(GIMP_RC_VERSION)
	#define GIMP_VERSION=Copy(GIMP_VERSION,1,Pos("-",GIMP_VERSION)-1)
#endif
#if !Defined(REVISION) || REVISION=="0"
	#define FULL_GIMP_VERSION GIMP_VERSION + "." + "0"
#else
	#define FULL_GIMP_VERSION GIMP_VERSION + "." + REVISION
#endif
;Get CUSTOM_GIMP_VERSION (that the users see)
#if !Defined(REVISION) || REVISION=="0"
	#define CUSTOM_GIMP_VERSION ORIGINAL_GIMP_VERSION
#else
	#define CUSTOM_GIMP_VERSION ORIGINAL_GIMP_VERSION + "-" + REVISION
#endif

;This script supports creating both an installer per arch or an universal installer with all arches
#if Defined(ARM64_BUNDLE) && !Defined(X64_BUNDLE) && !Defined(X86_BUNDLE)
#define ARCHS_ALLOWED="arm64"
#define ARCHS_INSTALLED="arm64"
#define GIMP_ARCHS="gimpARM64"
#define DEPS_ARCHS="depsARM64"
#define PY_ARCHS="pyARM64"
#define LUA_ARCHS="luaARM64"
#endif
#if Defined(ARM64_BUNDLE) && Defined(X64_BUNDLE) && Defined(X86_BUNDLE)
#define ARCHS_ALLOWED="x86compatible"
#define ARCHS_INSTALLED="x64os arm64"
#define GIMP_ARCHS="gimpARM64 or gimpX64 or gimpX86"
#define DEPS_ARCHS="depsARM64 or depsX64 or depsX86"
#define PY_ARCHS="pyARM64 or pyX64 or pyX86"
#define LUA_ARCHS="luaARM64 or luaX64 or luaX86"
#endif
#if !Defined(ARM64_BUNDLE) && Defined(X64_BUNDLE)
#define ARCHS_ALLOWED="x64os"
#define ARCHS_INSTALLED="x64os"
#define GIMP_ARCHS="gimpX64"
#define DEPS_ARCHS="depsX64"
#define PY_ARCHS="pyX64"
#define LUA_ARCHS="luaX64"
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
AppVerName=GIMP {#CUSTOM_GIMP_VERSION}
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
WizardStyle=modern
WizardSizePercent=100
WizardResizable=no
WizardImageAlphaFormat=defined
WizardSmallImageFile={#WIZARD_SMALL_IMAGE}
WizardImageFile={#WIZARD_IMAGE}
WizardImageStretch=yes
[LangOptions]
DialogFontName=Segoe UI
DialogFontSize=9
WelcomeFontName=Segoe UI
WelcomeFontSize=12

;3.4.1 INSTALLER PAGES AGAIN
[Tasks]
Name: desktopicon; Description: "{cm:AdditionalIconsDesktop}"; GroupDescription: "{cm:AdditionalIcons}"
[Icons]
Name: "{autoprograms}\GIMP {#CUSTOM_GIMP_VERSION}"; Filename: "{app}\bin\gimp-{#GIMP_MUTEX_VERSION}.exe"; WorkingDir: "%USERPROFILE%"; Comment: "GIMP {#CUSTOM_GIMP_VERSION}"
Name: "{autodesktop}\GIMP {#CUSTOM_GIMP_VERSION}"; Filename: "{app}\bin\gimp-{#GIMP_MUTEX_VERSION}.exe"; WorkingDir: "%USERPROFILE%"; Comment: "GIMP {#CUSTOM_GIMP_VERSION}"; Tasks: desktopicon
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
Name: gimpARM64; Description: "{cm:ComponentsGimp,{#GIMP_VERSION}}"; Types: full compact custom; Flags: fixed; Check: Check3264('arm64')
#endif
#ifdef X64_BUNDLE
Name: gimpX64; Description: "{cm:ComponentsGimp,{#GIMP_VERSION}}"; Types: full compact custom; Flags: fixed; Check: Check3264('x64')
#endif
#ifdef X86_BUNDLE
Name: gimpX86; Description: "{cm:ComponentsGimp,{#GIMP_VERSION}}"; Types: full compact custom; Flags: fixed; Check: Check3264('32')
#endif
;Deps files
#ifdef ARM64_BUNDLE
Name: depsARM64; Description: "{cm:ComponentsDeps,{#FULL_GIMP_VERSION}}"; Types: full compact custom; Flags: fixed; Check: Check3264('arm64')
#endif
#ifdef X64_BUNDLE
Name: depsX64; Description: "{cm:ComponentsDeps,{#FULL_GIMP_VERSION}}"; Types: full compact custom; Flags: fixed; Check: Check3264('x64')
#endif
#ifdef X86_BUNDLE
Name: depsX86; Description: "{cm:ComponentsDeps,{#FULL_GIMP_VERSION}}"; Types: full compact custom; Flags: fixed; Check: Check3264('32')
#endif

;Optional components (complete install)
#ifdef DEBUG_SYMBOLS
#ifdef ARM64_BUNDLE
Name: debugARM64; Description: "{cm:ComponentsDebug}"; Types: full custom; Flags: disablenouninstallwarning; Check: Check3264('arm64')
#endif
#ifdef X64_BUNDLE
Name: debugX64; Description: "{cm:ComponentsDebug}"; Types: full custom; Flags: disablenouninstallwarning; Check: Check3264('x64')
#endif
#ifdef X86_BUNDLE
Name: debugX86; Description: "{cm:ComponentsDebug}"; Types: full custom; Flags: disablenouninstallwarning; Check: Check3264('32')
#endif
#endif
;Development files
#ifdef ARM64_BUNDLE
Name: devARM64; Description: "{cm:ComponentsDev}"; Types: full custom; Flags: disablenouninstallwarning; Check: Check3264('arm64')
#endif
#ifdef X64_BUNDLE
Name: devX64; Description: "{cm:ComponentsDev}"; Types: full custom; Flags: disablenouninstallwarning; Check: Check3264('x64')
#endif
#ifdef X86_BUNDLE
Name: devX86; Description: "{cm:ComponentsDev}"; Types: full custom; Flags: disablenouninstallwarning; Check: Check3264('32')
#endif
;PostScript support
#ifdef ARM64_BUNDLE
Name: gsARM64; Description: "{cm:ComponentsGhostscript}"; Types: full custom; Check: Check3264('arm64')
#endif
#ifdef X64_BUNDLE
Name: gsX64; Description: "{cm:ComponentsGhostscript}"; Types: full custom; Check: Check3264('x64')
#endif
#ifdef X86_BUNDLE
Name: gsX86; Description: "{cm:ComponentsGhostscript}"; Types: full custom; Check: Check3264('32')
#endif
;Lua plug-ins support
#ifdef LUA
#ifdef ARM64_BUNDLE
Name: luaARM64; Description: "{cm:ComponentsLua}"; Types: full custom; Check: Check3264('arm64')
#endif
#ifdef X64_BUNDLE
Name: luaX64; Description: "{cm:ComponentsLua}"; Types: full custom; Check: Check3264('x64')
#endif
#ifdef X86_BUNDLE
Name: luaX86; Description: "{cm:ComponentsLua}"; Types: full custom; Check: Check3264('32')
#endif
#endif
;Python plug-ins support
#ifdef PYTHON
#ifdef ARM64_BUNDLE
Name: pyARM64; Description: "{cm:ComponentsPython}"; Types: full custom; Check: Check3264('arm64')
#endif
#ifdef X64_BUNDLE
Name: pyX64; Description: "{cm:ComponentsPython}"; Types: full custom; Check: Check3264('x64')
#endif
#ifdef X86_BUNDLE
Name: pyX86; Description: "{cm:ComponentsPython}"; Types: full custom; Check: Check3264('32')
#endif
#endif
;Locales
Name: loc; Description: "{cm:ComponentsTranslations}"; Types: full custom
#include ASSETS_DIR + "\base_po-cmp.list"
;MyPaint Brushes
Name: mypaint; Description: "{cm:ComponentsMyPaint}"; Types: full custom
;32-bit TWAIN support
#ifdef X86_BUNDLE
Name: gimp32on64; Description: "{cm:ComponentsGimp32}"; Types: full custom; Flags: checkablealone; Check: Check3264('64')
#endif

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
#ifdef X86_BUNDLE
#define MAIN_BUNDLE X86_BUNDLE
#endif
#ifdef X64_BUNDLE
#define MAIN_BUNDLE X64_BUNDLE
#endif
#ifdef ARM64_BUNDLE
#define MAIN_BUNDLE ARM64_BUNDLE
#endif
#define COMMON_FLAGS="recursesubdirs restartreplace uninsrestartdelete ignoreversion"

;Required arch-neutral files (compact install)
#define OPTIONAL_EXT="*.pdb,*.lua,*.py"
Source: "{#MAIN_BUNDLE}\etc\gimp\*"; DestDir: "{app}\etc\gimp"; Components: {#GIMP_ARCHS}; Flags: {#COMMON_FLAGS}
Source: "{#MAIN_BUNDLE}\lib\gimp\{#GIMP_PKGCONFIG_VERSION}\environ\default.env"; DestDir: "{app}\lib\gimp\{#GIMP_PKGCONFIG_VERSION}\environ"; Components: {#GIMP_ARCHS}; Flags: {#COMMON_FLAGS}
Source: "{#MAIN_BUNDLE}\lib\gimp\{#GIMP_PKGCONFIG_VERSION}\interpreters\gimp-script-fu-interpreter.interp"; DestDir: "{app}\lib\gimp\{#GIMP_PKGCONFIG_VERSION}\interpreters"; Components: {#GIMP_ARCHS}; Flags: {#COMMON_FLAGS}
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
Source: "{#MAIN_BUNDLE}\lib\gimp\{#GIMP_PKGCONFIG_VERSION}\interpreters\pygimp.interp"; DestDir: "{app}\lib\gimp\{#GIMP_PKGCONFIG_VERSION}\interpreters"; Components: {#PY_ARCHS}; Flags: {#COMMON_FLAGS}
Source: "{#MAIN_BUNDLE}\*.py"; DestDir: "{app}"; Components: {#PY_ARCHS}; Flags: {#COMMON_FLAGS}
#endif
Source: "{#MAIN_BUNDLE}\share\locale\*"; DestDir: "{app}\share\locale"; Components: loc; Flags: dontcopy {#COMMON_FLAGS}
#include ASSETS_DIR + "\base_po-files.list"
Source: "{#MAIN_BUNDLE}\share\mypaint-data\*"; DestDir: "{app}\share\mypaint-data"; Components: mypaint; Flags: {#COMMON_FLAGS}

;Required and optional arch specific files (binaries), except TWAIN in x64 and amd64
;i686
#ifdef X86_BUNDLE
#define BUNDLE X86_BUNDLE
#define COMPONENT "X86"
;Set solid break for 32-bit binaries. See: https://gitlab.gnome.org/GNOME/gimp/-/issues/13801
#define COMMON_FLAGS="recursesubdirs restartreplace uninsrestartdelete ignoreversion solidbreak"
#include "base_executables.isi"
;TWAIN is always installed in the 32-bit version of GIMP
Source: "{#X86_BUNDLE}\lib\gimp\{#GIMP_PKGCONFIG_VERSION}\plug-ins\twain.exe"; DestDir: "{app}\lib\gimp\{#GIMP_PKGCONFIG_VERSION}\plug-ins"; Components: gimpX86; Flags: {#COMMON_FLAGS}
;Restore common flags
#define COMMON_FLAGS="recursesubdirs restartreplace uninsrestartdelete ignoreversion"
#endif

;x86_64
#ifdef X64_BUNDLE
#define BUNDLE X64_BUNDLE
#define COMPONENT "X64"
#include "base_executables.isi"
#endif

;AArch64
#ifdef ARM64_BUNDLE
#define BUNDLE ARM64_BUNDLE
#define COMPONENT "ARM64"
#include "base_executables.isi"
#endif

;Optional 32-bit specific bins for TWAIN, since x64 and arm64 twain drivers are rare
#ifdef X86_BUNDLE
#include "base_twain32on64.isi"
Source: "{#X86_BUNDLE}\lib\gimp\{#GIMP_PKGCONFIG_VERSION}\plug-ins\twain\twain.exe"; DestDir: "{app}\lib\gimp\{#GIMP_PKGCONFIG_VERSION}\plug-ins\twain"; Components: gimp32on64; Flags: {#COMMON_FLAGS}
#endif

;upgrade zlib1.dll in System32 if it's present there to avoid breaking plugins
;sharedfile flag will ensure that the upgraded file is left behind on uninstall to avoid breaking other programs that use the file
#ifdef ARM64_BUNDLE
Source: "{#ARM64_BUNDLE}\bin\zlib1.dll"; DestDir: "{sys}"; Components: gimpARM64; Flags: restartreplace sharedfile uninsrestartdelete comparetimestamp; Check: BadSysDLL('zlib1.dll',64)
#endif
#ifdef X64_BUNDLE
Source: "{#X64_BUNDLE}\bin\zlib1.dll"; DestDir: "{sys}"; Components: gimpX64; Flags: restartreplace sharedfile uninsrestartdelete comparetimestamp; Check: BadSysDLL('zlib1.dll',64)
#endif
#ifdef X86_BUNDLE
Source: "{#X86_BUNDLE}\bin\zlib1.dll"; DestDir: "{sys}"; Components: {#GIMP_ARCHS}; Flags: restartreplace sharedfile 32bit uninsrestartdelete comparetimestamp; Check: BadSysDLL('zlib1.dll',32)
#endif

;allow specific config files to be overridden if '/configoverride=' is set at run time
#define FindHandle
#sub ProcessConfigFile
  #define FileName FindGetFileName(FindHandle)
Source: "{code:GetExternalConfDir}\{#FileName}"; DestDir: "{app}\{#ConfigDir}"; Flags: external restartreplace; Check: CheckExternalConf('{#FileName}')
  #if BaseDir != MAIN_BUNDLE
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
#define public BaseDir MAIN_BUNDLE
#define public ConfigDir "etc\gimp\" + GIMP_PKGCONFIG_VERSION
#expr ProcessConfigDir
#define public ConfigDir "etc\fonts"
#expr ProcessConfigDir

#endif //NOFILES

;We need at least an empty folder to avoid GIMP*_LOCALEDIR warnings
[Dirs]
#ifdef X86_BUNDLE
Name: "{app}\32\share\locale"; Components: gimp32on64; Flags: uninsalwaysuninstall
#endif

;4.2 SPECIAL-CASE FILES TO BE WIPED
[InstallDelete]
Type: files; Name: "{app}\bin\gimp-?.?.exe"
Type: files; Name: "{app}\bin\gimp-?.??.exe"
Type: files; Name: "{app}\bin\gimp-console-?.?.exe"
Type: files; Name: "{app}\bin\gimp-console-?.??.exe"
;old ghostscript
Type: filesandordirs; Name: "{app}\share\ghostscript\*"
;get previous GIMP icon name from uninstall name in Registry
Type: files; Name: "{autoprograms}\{reg:HKA\SOFTWARE\Microsoft\Windows\CurrentVersion\Uninstall\GIMP-{#GIMP_MUTEX_VERSION}_is1,DisplayName|GIMP {#GIMP_MUTEX_VERSION}}.lnk"; Check: CheckRegValueExists('SOFTWARE\Microsoft\Windows\CurrentVersion\Uninstall\GIMP-{#GIMP_MUTEX_VERSION}_is1','DisplayName')
Type: files; Name: "{autodesktop}\{reg:HKA\SOFTWARE\Microsoft\Windows\CurrentVersion\Uninstall\GIMP-{#GIMP_MUTEX_VERSION}_is1,DisplayName|GIMP {#GIMP_MUTEX_VERSION}}.lnk"; Check: CheckRegValueExists('SOFTWARE\Microsoft\Windows\CurrentVersion\Uninstall\GIMP-{#GIMP_MUTEX_VERSION}_is1','DisplayName')
;remove old babl and gegl plugins
Type: filesandordirs; Name: "{app}\lib\babl-0.1"
Type: filesandordirs; Name: "{app}\lib\gegl-0.4"
;Uneeded Linux appdata shipped in 2.99.18
Type: filesandordirs; Name: "{app}\share\metainfo"
;This was bunbled in 3.0 RC1 but not needed since the "Debug" menu is hidden in stable releases
#if !Defined(GIMP_UNSTABLE) && Defined(GIMP_RELEASE)
	Type: files; Name: "{app}\bin\dot.exe"
#endif
;No need to all these python binaries shipped in 3.0 RC1
Type: files; Name: "{app}\bin\python3*.exe"
;Uneeded shipped headers in 3.0 RC3 (we now ship only babl, gegl and gimp)
Type: filesandordirs; Name: "{app}\include\exiv2"
Type: filesandordirs; Name: "{app}\include\gexiv2"

[UninstallDelete]
Type: files; Name: "{app}\uninst\uninst.inf"
Type: files; Name: "{app}\lib\gimp\{#GIMP_PKGCONFIG_VERSION}\interpreters\lua.interp"
Type: files; Name: "{app}\lib\gimp\{#GIMP_PKGCONFIG_VERSION}\environ\pygimp.env"


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
Root: HKA; Subkey: "Software\Classes\Applications\gimp-{#GIMP_MUTEX_VERSION}.exe"; ValueType: string; ValueName: "FriendlyAppName"; ValueData: "GIMP"
Root: HKA; Subkey: "Software\Classes\Applications\gimp-{#GIMP_MUTEX_VERSION}.exe\DefaultIcon"; ValueType: string; ValueName: ""; ValueData: "{app}\bin\gimp-{#GIMP_MUTEX_VERSION}.exe,1"
Root: HKA; Subkey: "Software\Classes\Applications\gimp-{#GIMP_MUTEX_VERSION}.exe\shell\open\command"; ValueType: string; ValueName: ""; ValueData: """{app}\bin\gimp-{#GIMP_MUTEX_VERSION}.exe"" ""%1"""
;'ms-settings:defaultapps' page
Root: HKA; Subkey: "Software\GIMP {#GIMP_MUTEX_VERSION}"; Flags: uninsdeletekey
Root: HKA; Subkey: "Software\GIMP {#GIMP_MUTEX_VERSION}\Capabilities"; ValueType: string; ValueName: "ApplicationName"; ValueData: "GIMP"
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
Root: HKA; Subkey: "Software\Classes\GIMP{#GIMP_MUTEX_VERSION}.{#FileLine}"; ValueType: string; ValueName: ""; ValueData: "GIMP {#CUSTOM_GIMP_VERSION} {#UpperCase(FileLine)}"; Flags: uninsdeletekey
Root: HKA; Subkey: "Software\Classes\GIMP{#GIMP_MUTEX_VERSION}.{#FileLine}\DefaultIcon"; ValueType: string; ValueName: ""; ValueData: "{app}\bin\gimp-{#GIMP_MUTEX_VERSION}.exe,2"
Root: HKA; Subkey: "Software\Classes\GIMP{#GIMP_MUTEX_VERSION}.{#FileLine}\shell\open\command"; ValueType: string; ValueName: ""; ValueData: """{app}\bin\gimp-{#GIMP_MUTEX_VERSION}.exe"" ""%1"""
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
Root: HKA; Subkey: "Software\Classes\GIMP{#GIMP_MUTEX_VERSION}.xcf"; ValueType: string; ValueName: ""; ValueData: "GIMP {#CUSTOM_GIMP_VERSION} XCF"; Flags: uninsdeletekey
Root: HKA; Subkey: "Software\Classes\GIMP{#GIMP_MUTEX_VERSION}.xcf\DefaultIcon"; ValueType: string; ValueName: ""; ValueData: "{app}\bin\gimp-{#GIMP_MUTEX_VERSION}.exe,1"
Root: HKA; Subkey: "Software\Classes\GIMP{#GIMP_MUTEX_VERSION}.xcf\shell\open\command"; ValueType: string; ValueName: ""; ValueData: """{app}\bin\gimp-{#GIMP_MUTEX_VERSION}.exe"" ""%1"""
Root: HKA; Subkey: "Software\Classes\Applications\gimp-{#GIMP_MUTEX_VERSION}.exe\SupportedTypes"; ValueType: string; ValueName: ".xcf"; ValueData: ""
Root: HKA; Subkey: "Software\GIMP {#GIMP_MUTEX_VERSION}\Capabilities\FileAssociations"; ValueType: string; ValueName: ".xcf"; ValueData: "GIMP{#GIMP_MUTEX_VERSION}.xcf"
;Associations (make association for .ico files but do not set DefaultIcon since their content is the DefaultIcon)
Root: HKA; Subkey: "Software\Classes\.ico\OpenWithProgids"; ValueType: string; ValueName: "GIMP{#GIMP_MUTEX_VERSION}.ico"; ValueData: ""; Flags: uninsdeletevalue
Root: HKA; Subkey: "Software\Classes\GIMP{#GIMP_MUTEX_VERSION}.ico"; ValueType: string; ValueName: ""; ValueData: "GIMP {#CUSTOM_GIMP_VERSION} ICO"; Flags: uninsdeletekey
Root: HKA; Subkey: "Software\Classes\GIMP{#GIMP_MUTEX_VERSION}.ico\DefaultIcon"; ValueType: string; ValueName: ""; ValueData: "%1"
Root: HKA; Subkey: "Software\Classes\GIMP{#GIMP_MUTEX_VERSION}.ico\shell\open\command"; ValueType: string; ValueName: ""; ValueData: """{app}\bin\gimp-{#GIMP_MUTEX_VERSION}.exe"" ""%1"""
Root: HKA; Subkey: "Software\Classes\Applications\gimp-{#GIMP_MUTEX_VERSION}.exe\SupportedTypes"; ValueType: string; ValueName: ".ico"; ValueData: ""
Root: HKA; Subkey: "Software\GIMP {#GIMP_MUTEX_VERSION}\Capabilities\FileAssociations"; ValueType: string; ValueName: ".ico"; ValueData: "GIMP{#GIMP_MUTEX_VERSION}.ico"


;5 INSTALLER CUSTOM CODE
[Code]
//GENERAL VARS AND UTILS
const
	CP_ACP = 0;
	CP_UTF8 = 65001;
  COLOR_HOTLIGHT = 26;

var
	//pgSimple: TWizardPage;
  InstallType: String;
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

	Check32bitOverride;

	Result := RestartSetupAfterReboot(); //resume install after reboot - skip all setting pages, and install directly

	if Result then
		Result := BPPTooLowWarning();

	if not Result then //no need to do anything else
		exit;

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

	//if InstallMode <> imRebootContinue then
	//	SuppressibleMsgBox(CustomMessage('UninstallWarning'),mbError,MB_OK,IDOK);
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
		WizardForm.NextButton.Visible := False;

		if not (InstallType = 'itRepair') then begin
		    btnInstall.Visible := True;
		end;
		btnInstall.TabOrder := 1;

		//Inno does not support "repairing" a lost install so let's show Customize button to allow to repair installs
		//Inno does not support changing components at reinstall or update so let's hide Customize to not break installs
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
var	i,ButtonWidth: Integer;
	ButtonText: TArrayOfString;
	MeasureLabel: TNewStaticText;
	//lblInfo: TNewStaticText;
begin
	DebugMsg('InitCustomPages','wpLicense');

	CheckInstallType;

	btnInstall := TNewButton.Create(WizardForm);
	with btnInstall do
	begin
		Parent := WizardForm;
		Width := WizardForm.NextButton.Width;
		Height := WizardForm.NextButton.Height;
		Left := WizardForm.NextButton.Left;
		Top := WizardForm.NextButton.Top;
		if InstallType = 'itInstall' then begin
		    Caption := CustomMessage('Install');
	    end else if InstallType = 'itReinstall' then begin
		    Caption := CustomMessage('Reinstall');
	    end else if InstallType = 'itUpdate' then begin
		    Caption := CustomMessage('Update');
		end;
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
	WizardForm.Bevel.Visible := False;

	WizardForm.InfoBeforeClickLabel.Visible := False;
	WizardForm.InfoBeforeMemo.Height := WizardForm.InfoBeforeMemo.Height + WizardForm.InfoBeforeMemo.Top - WizardForm.InfoBeforeClickLabel.Top;
	WizardForm.InfoBeforeMemo.Top := WizardForm.InfoBeforeClickLabel.Top;
end;


//3. INSTALL DIR: override Inno custom dir icon
procedure NativeDirIcon();
var TypRect: TRect;
    Icon: THandle;
	IconSize: Integer;
begin
    WizardForm.SelectDirBitmapImage.Visible := False;

	Icon := ExtractIcon(0,'imageres.dll',3)
    with TBitmapImage.Create(WizardForm.SelectDirPage) do begin
        Parent := WizardForm.SelectDirPage;
	   	with Bitmap do begin
            Left := 0;
	        Top := 0;
	        AutoSize := True;
			Center := True;
			Width := ScaleY(32);
            Height := ScaleY(32);
			Canvas.FillRect(TypRect);

			if WizardForm.Font.PixelsPerInch >= 168 then begin          //175% scaling
				IconSize := 64;
			end else if WizardForm.Font.PixelsPerInch >= 144 then begin //150% scaling
				IconSize := 48;
			end else if WizardForm.Font.PixelsPerInch >= 120 then begin //125% scaling
				IconSize := 32;
			end else begin                                              //100% scaling
				IconSize := 32;
			end;
			DrawIconEx(Canvas.Handle, 0, 0, Icon, IconSize, IconSize, 0, 0, DI_NORMAL);
        end;
    end;
end;

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

	Components := ['Gimp','Deps','Debug','Dev','Ghostscript','Lua','Python','Translations','MyPaint','Gimp32'];
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
procedure PreparingFaceLift();
begin
	WizardForm.Bevel.Visible := False;
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
		Result := FmtMessage(CustomMessage('RemovingOldVersionFailed'),['{#CUSTOM_GIMP_VERSION}',ExpandConstant('{app}')]);
	end else
	if RemoveResult = rogCantUninstall then
	begin
		DebugMsg('PrepareToInstall','RemoveOldGIMPVersions failed to uninstall old GIMP version [1]');
		Result := FmtMessage(CustomMessage('RemovingOldVersionCantUninstall'),['{#CUSTOM_GIMP_VERSION}',ExpandConstant('{app}')]);
	end else
	begin
		DebugMsg('PrepareToInstall','Internal error 11');
		Result := FmtMessage(CustomMessage('InternalError'),['11']); //should never happen
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
	WizardForm.Bevel.Visible := False;

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

		InterpFile := ExpandConstant('{app}\lib\gimp\{#GIMP_PKGCONFIG_VERSION}\interpreters\pygimp.interp');
    DebugMsg('PrepareInterp','Writing interpreter file for gimp-python: ' + InterpFile);

#if Defined(GIMP_UNSTABLE) || !Defined(GIMP_RELEASE)
		//python.exe is prefered in unstable versions because of error output
		#define PYTHON="python.exe"
#else
	  //pythonw.exe is prefered in stable releases because it works silently
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

#include "util_uninst.isi"


//8. DONE/FINAL PAGE (no customizations)


//INITIALIZE AND ORDER INSTALLER PAGES
procedure InitializeWizard();
begin
	UpdateWizardImages();
	InitCustomPages();
	NativeDirIcon();
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

//Reboot if needed
#include "util_rebootcontinue.isi"


#expr SaveToFile(AddBackslash(SourcePath) + "Preprocessed.iss")
