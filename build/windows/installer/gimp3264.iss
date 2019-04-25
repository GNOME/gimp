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
;requires Inno Setup 5.4.2 unicode + ISPP
;
;See directories.isi 
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

#ifndef VERSION
	#define VERSION "2.7.0"
	#define REVISION "20100414"
	#define NOFILES
#endif

#include "directories.isi"
#include "version.isi"

[Setup]
AppName=GIMP
#if Defined(DEVEL) && DEVEL != ""
AppID=GIMP-{#MAJOR}.{#MINOR}
#else
AppID=GIMP-{#MAJOR}
#endif
VersionInfoVersion={#VERSION}
#if !defined(REVISION)
AppVerName=GIMP {#VERSION}
#else
AppVerName=GIMP {#VERSION}-{#REVISION}
#endif
AppPublisherURL=https://www.gimp.org/
AppSupportURL=https://www.gimp.org/docs/
AppUpdatesURL=https://www.gimp.org/
AppPublisher=The GIMP Team
AppVersion={#VERSION}
DisableProgramGroupPage=yes
DisableWelcomePage=no
DisableDirPage=auto
AlwaysShowDirOnReadyPage=yes
ChangesEnvironment=yes

#if Defined(DEVEL) && DEVEL != ""
DefaultDirName={pf}\GIMP {#MAJOR}.{#MINOR}
LZMANumBlockThreads=4
LZMABlockSize=76800
#else
DefaultDirName={pf}\GIMP {#MAJOR}
#endif

;AllowNoIcons=true
FlatComponentsList=yes
InfoBeforeFile=gpl+python.rtf
ChangesAssociations=true

WizardImageFile=windows-installer-intro-big.bmp
WizardImageStretch=yes
WizardSmallImageFile=wilber.bmp

UninstallDisplayIcon={app}\bin\gimp-{#MAJOR}.{#MINOR}.exe
UninstallFilesDir={app}\uninst

MinVersion=6.1
ArchitecturesInstallIn64BitMode=x64

#ifdef NOCOMPRESSION
;UseSetupLdr=no
OutputDir=_Output\unc
Compression=none
;InternalCompressLevel=0
#else
OutputDir=_Output
Compression=lzma2/ultra64
InternalCompressLevel=ultra
SolidCompression=yes
LZMAUseSeparateProcess=yes
LZMANumFastBytes=273
LZMADictionarySize=524288

;SignTool=Default
SignedUninstaller=yes
SignedUninstallerDir=_Uninst
#endif //NOCOMPRESSION

#if !defined(REVISION)
OutputBaseFileName=gimp-{#VERSION}-setup
#else
OutputBaseFileName=gimp-{#VERSION}-{#REVISION}-setup
#endif

[Languages]
Name: "en"; MessagesFile: "compiler:Default.isl,lang\en.setup.isl"
Name: "ca"; MessagesFile: "compiler:Languages\Catalan.isl,lang\ca.setup.isl"
Name: "da"; MessagesFile: "compiler:Languages\Danish.isl,lang\da.setup.isl"
Name: "de"; MessagesFile: "compiler:Languages\German.isl,lang\de.setup.isl"
Name: "el"; MessagesFile: "compiler:Languages\Greek.isl,lang\el.setup.isl"
Name: "eo"; MessagesFile: "compiler:Languages\Unofficial\Esperanto.isl,lang\eo.setup.isl"
Name: "es"; MessagesFile: "compiler:Languages\Spanish.isl,lang\es.setup.isl"
Name: "eu"; MessagesFile: "compiler:Languages\Unofficial\Basque.isl,lang\eu.setup.isl"
Name: "fr"; MessagesFile: "compiler:Languages\French.isl,lang\fr.setup.isl"
Name: "hu"; MessagesFile: "compiler:Languages\Hungarian.isl,lang\hu.setup.isl"
Name: "id"; MessagesFile: "compiler:Languages\Unofficial\Indonesian.isl,lang\id.setup.isl"
Name: "is"; MessagesFile: "compiler:Languages\Unofficial\Icelandic.isl,lang\is.setup.isl"
Name: "it"; MessagesFile: "compiler:Languages\Italian.isl,lang\it.setup.isl"
Name: "ko"; MessagesFile: "compiler:Languages\Unofficial\Korean.isl,lang\ko.setup.isl"
Name: "lv"; MessagesFile: "compiler:Languages\Unofficial\Latvian.isl,lang\lv.setup.isl"
Name: "nl"; MessagesFile: "compiler:Languages\Dutch.isl,lang\nl.setup.isl"
Name: "pl"; MessagesFile: "compiler:Languages\Polish.isl,lang\pl.setup.isl"
Name: "pt_BR"; MessagesFile: "compiler:Languages\BrazilianPortuguese.isl,lang\pt_BR.setup.isl"
Name: "ru"; MessagesFile: "compiler:Languages\Russian.isl,lang\ru.setup.isl"
Name: "sl"; MessagesFile: "compiler:Languages\Slovenian.isl,lang\sl.setup.isl"
Name: "sv"; MessagesFile: "compiler:Languages\Unofficial\Swedish.isl,lang\sv.setup.isl"
Name: "tr"; MessagesFile: "compiler:Languages\Turkish.isl,lang\tr.setup.isl"
Name: "zh_CN"; MessagesFile: "compiler:Languages\Unofficial\ChineseSimplified.isl,lang\zh_CN.setup.isl"
Name: "zh_TW"; MessagesFile: "compiler:Languages\Unofficial\ChineseTraditional.isl,lang\zh_TW.setup.isl"
;Name: "ro"; MessagesFile: "Romanian.islu,ro.setup.islu"

[Types]
;Name: normal; Description: "{cm:TypeTypical}"
Name: full; Description: "{cm:TypeFull}"
Name: compact; Description: "{cm:TypeCompact}"
Name: custom; Description: "{cm:TypeCustom}"; Flags: iscustom

[Components]
Name: gimp32; Description: "{cm:ComponentsGimp,{#VERSION}}"; Types: full compact custom; Flags: fixed; Check: Check3264('32')
Name: gimp64; Description: "{cm:ComponentsGimp,{#VERSION}}"; Types: full compact custom; Flags: fixed; Check: Check3264('64')

Name: deps32; Description: "{cm:ComponentsDeps,{#GTK_VERSION}}"; Types: full compact custom; Flags: checkablealone fixed; Check: Check3264('32')
Name: deps32\wimp; Description: "{cm:ComponentsGtkWimp}"; Types: full custom; Flags: dontinheritcheck disablenouninstallwarning; Check: Check3264('32')
Name: deps32\compat; Description: "{cm:ComponentsCompat}"; Types: full custom; Flags: dontinheritcheck; Check: Check3264('32')
Name: deps64; Description: "{cm:ComponentsDeps,{#GTK_VERSION}}"; Types: full compact custom; Flags: checkablealone fixed; Check: Check3264('64')
Name: deps64\wimp; Description: "{cm:ComponentsGtkWimp}"; Types: full custom; Flags: dontinheritcheck disablenouninstallwarning; Check: Check3264('64')

#ifdef DEBUG_SYMBOLS
Name: debug; Description: "{cm:ComponentsDebug}"; Types: full custom; Flags: disablenouninstallwarning
#endif

Name: gs; Description: "{cm:ComponentsGhostscript}"; Types: full custom

Name: loc; Description: "{cm:ComponentsTranslations}"; Types: full custom

Name: mypaint; Description: "{cm:ComponentsMyPaint}"; Types: full custom

#ifdef PYTHON
Name: py; Description: "{cm:ComponentsPython}"; Types: full custom; Check: Check3264('32')
#endif

Name: gimp32on64; Description: "{cm:ComponentsGimp32}"; Types: full custom; Flags: checkablealone; Check: Check3264('64')
#ifdef PYTHON
Name: gimp32on64\py; Description: "{cm:ComponentsPython}"; Types: full custom; Check: Check3264('64')
#endif
Name: gimp32on64\compat; Description: "{cm:ComponentsCompat}"; Types: full custom; Flags: dontinheritcheck; Check: Check3264('64')

[Tasks]
Name: desktopicon; Description: "{cm:AdditionalIconsDesktop}"; GroupDescription: "{cm:AdditionalIcons}"; Flags: unchecked

[Icons]
#define ICON_VERSION=MAJOR + "." + MINOR + "." + MICRO
Name: "{commonprograms}\GIMP {#ICON_VERSION}"; Filename: "{app}\bin\gimp-{#MAJOR}.{#MINOR}.exe"; WorkingDir: "%USERPROFILE%"; Comment: "GIMP {#VERSION}"
Name: "{commondesktop}\GIMP {#ICON_VERSION}"; Filename: "{app}\bin\gimp-{#MAJOR}.{#MINOR}.exe"; WorkingDir: "%USERPROFILE%"; Comment: "GIMP {#VERSION}"; Tasks: desktopicon

[Files]
;setup files
Source: "setup.ini"; Flags: dontcopy
Source: "windows-installer-intro-small.bmp"; Flags: dontcopy
Source: "installsplash.bmp"; Flags: dontcopy
Source: "installsplash_small.bmp"; Flags: dontcopy

#ifndef NOFILES
;files common to both 32 and 64-bit versions
Source: "{#GIMP_DIR32}\etc\*"; DestDir: "{app}\etc"; Components: gimp32 or gimp64; Flags: recursesubdirs restartreplace uninsrestartdelete ignoreversion
Source: "{#GIMP_DIR32}\lib\gimp\2.0\environ\*"; DestDir: "{app}\lib\gimp\2.0\environ"; Components: gimp32 or gimp64; Flags: recursesubdirs restartreplace uninsrestartdelete ignoreversion
Source: "{#GIMP_DIR32}\lib\gimp\2.0\interpreters\*"; DestDir: "{app}\lib\gimp\2.0\interpreters"; Components: gimp32 or gimp64; Flags: recursesubdirs restartreplace uninsrestartdelete ignoreversion
Source: "{#GIMP_DIR32}\share\gimp\*"; DestDir: "{app}\share\gimp"; Components: gimp32 or gimp64; Flags: recursesubdirs createallsubdirs restartreplace uninsrestartdelete ignoreversion
Source: "{#DEPS_DIR32}\share\enchant\*"; DestDir: "{app}\share\enchant"; Components: deps32 or deps64; Flags: recursesubdirs restartreplace uninsrestartdelete ignoreversion
Source: "{#DEPS_DIR32}\share\libwmf\*"; DestDir: "{app}\share\libwmf"; Components: deps32 or deps64; Flags: recursesubdirs restartreplace uninsrestartdelete ignoreversion
Source: "{#DEPS_DIR32}\share\themes\*"; DestDir: "{app}\share\themes"; Components: deps32 or deps64; Flags: recursesubdirs restartreplace uninsrestartdelete ignoreversion
Source: "{#DEPS_DIR32}\share\xml\*"; DestDir: "{app}\share\xml"; Components: deps32 or deps64; Flags: recursesubdirs restartreplace uninsrestartdelete ignoreversion

Source: "{#DEPS_DIR32}\share\poppler\*.*"; DestDir: "{app}\share\poppler"; Components: deps32 or deps64; Flags: recursesubdirs restartreplace uninsrestartdelete ignoreversion
Source: "{#DEPS_DIR32}\share\libthai\*"; DestDir: "{app}\share\libthai"; Components: deps32 or deps64; Flags: recursesubdirs restartreplace uninsrestartdelete ignoreversion

Source: "{#DEPS_DIR32}\share\locale\*"; DestDir: "{app}\share\locale"; Components: loc; Flags: recursesubdirs restartreplace uninsrestartdelete ignoreversion
Source: "{#GIMP_DIR32}\share\locale\*"; DestDir: "{app}\share\locale"; Components: loc; Flags: recursesubdirs restartreplace uninsrestartdelete ignoreversion

;configure gimp with --enable-bundled-mypaint-brushes for the correct path to
;be picked by gimp
Source: "{#DEPS_DIR32}\share\mypaint-data\*"; DestDir: "{app}\share\mypaint-data"; Components: mypaint; Flags: recursesubdirs restartreplace uninsrestartdelete ignoreversion

Source: "{#DEPS_DIR32}\etc\fonts\*"; DestDir: "{app}\etc\fonts"; Components: deps32 or deps64; Flags: recursesubdirs restartreplace uninsrestartdelete ignoreversion
Source: "{#DEPS_DIR32}\etc\gtk-2.0\*"; DestDir: "{app}\etc\gtk-2.0"; Excludes: gtkrc; Components: deps32 or deps64; Flags: recursesubdirs restartreplace uninsrestartdelete ignoreversion
Source: "{#DEPS_DIR32}\etc\gtk-2.0\gtkrc"; DestDir: "{app}\etc\gtk-2.0"; Components: deps32\wimp or deps64\wimp; Flags: recursesubdirs restartreplace uninsrestartdelete ignoreversion

;ghostscript TODO: detect version automatically
Source: "{#DEPS_DIR32}\share\ghostscript\9.25\lib\*.*"; DestDir: "{app}\share\ghostscript\9.25\lib"; Components: gs and (gimp32 or gimp64); Flags: recursesubdirs restartreplace uninsrestartdelete ignoreversion

;32-on-64bit
#include "32on64.isi"
;prefer 32bit twain plugin over 64bit because 64bit twain drivers are rare
Source: "{#GIMP_DIR32}\lib\gimp\2.0\plug-ins\twain\twain.exe"; DestDir: "{app}\lib\gimp\2.0\plug-ins\twain"; Components: gimp32on64; Flags: recursesubdirs restartreplace uninsrestartdelete ignoreversion
Source: "{#GIMP_DIR64}\lib\gimp\2.0\plug-ins\twain\twain.exe"; DestDir: "{app}\lib\gimp\2.0\plug-ins\twain"; Components: (not gimp32on64) and gimp64; Flags: recursesubdirs restartreplace uninsrestartdelete ignoreversion
;special case due to MS-Windows engine
Source: "{#DEPS_DIR32}\etc\gtk-2.0\*"; DestDir: "{app}\32\etc\gtk-2.0"; Excludes: gtkrc; Components: gimp32on64; Flags: recursesubdirs restartreplace uninsrestartdelete ignoreversion
Source: "{#DEPS_DIR32}\etc\gtk-2.0\gtkrc"; DestDir: "{app}\32\etc\gtk-2.0"; Components: gimp32on64 and deps64\wimp; Flags: recursesubdirs restartreplace uninsrestartdelete ignoreversion
;python scripts
#ifdef PYTHON
Source: "{#GIMP_DIR32}\lib\gimp\2.0\plug-ins\*.py"; DestDir: "{app}\lib\gimp\2.0\plug-ins"; Components: gimp32on64\py; Flags: recursesubdirs restartreplace uninsrestartdelete ignoreversion
Source: "{#GIMP_DIR32}\lib\gimp\2.0\python\*.p*"; Excludes: "*.debug"; DestDir: "{app}\32\lib\gimp\2.0\python"; Components: gimp32on64\py; Flags: recursesubdirs restartreplace uninsrestartdelete ignoreversion
#endif
;compat libraries
Source: "{#DEPS_DIR}\compat\*.dll"; DestDir: "{app}\32\"; Components: gimp32on64\compat; Flags: recursesubdirs restartreplace uninsrestartdelete ignoreversion

;32bit
#define PLATFORM 32
#include "files.isi"
;special case, since 64bit version doesn't work, and is excluded in files.isi
Source: "{#GIMP_DIR32}\lib\gimp\2.0\plug-ins\twain.exe"; DestDir: "{app}\lib\gimp\2.0\plug-ins"; Components: gimp32; Flags: recursesubdirs restartreplace uninsrestartdelete ignoreversion
;python scripts
#ifdef PYTHON
Source: "{#GIMP_DIR32}\lib\gimp\2.0\plug-ins\*.py"; DestDir: "{app}\lib\gimp\2.0\plug-ins"; Components: py; Flags: recursesubdirs restartreplace uninsrestartdelete ignoreversion
Source: "{#GIMP_DIR32}\lib\gimp\2.0\python\*.p*"; Excludes: "*.debug"; DestDir: "{app}\lib\gimp\2.0\python"; Components: py; Flags: recursesubdirs restartreplace uninsrestartdelete ignoreversion
#endif
Source: "{#DEPS_DIR}\compat\*.dll"; DestDir: "{app}"; Components: deps32\compat; Flags: recursesubdirs restartreplace uninsrestartdelete ignoreversion

;64bit
#define PLATFORM 64
#include "files.isi"

;upgrade zlib1.dll in System32 if it's present there to avoid breaking plugins
;sharedfile flag will ensure that the upgraded file is left behind on uninstall to avoid breaking other programs that use the file
Source: "{#DEPS_DIR32}\bin\zlib1.dll"; DestDir: "{sys}"; Components: gimp32 or gimp64; Flags: restartreplace sharedfile 32bit uninsrestartdelete comparetimestamp; Check: BadSysDLL('zlib1.dll',32)
Source: "{#DEPS_DIR64}\bin\zlib1.dll"; DestDir: "{sys}"; Components: gimp64; Flags: restartreplace sharedfile uninsrestartdelete comparetimestamp; Check: BadSysDLL('zlib1.dll',64)

;overridden configuration files
#include "configoverride.isi"

;python
#ifdef PYTHON
Source: "{#DEPS_DIR32}\bin\python2w.exe"; DestDir: "{app}\bin"; DestName: "pythonw.exe"; Components: py; Flags: restartreplace uninsrestartdelete ignoreversion
Source: "{#DEPS_DIR32}\bin\python2.exe"; DestDir: "{app}\bin"; DestName: "python.exe"; Components: py; Flags: restartreplace uninsrestartdelete ignoreversion
Source: "{#DEPS_DIR32}\bin\libpython2.7.dll"; DestDir: "{app}\bin"; Components: py; Flags: restartreplace uninsrestartdelete ignoreversion
Source: "{#DEPS_DIR32}\lib\python2.7\*"; DestDir: "{app}\lib\python2.7"; Components: py; Flags: recursesubdirs restartreplace uninsrestartdelete ignoreversion

Source: "{#DEPS_DIR32}\bin\python2w.exe"; DestDir: "{app}\32\bin"; DestName: "pythonw.exe"; Components: gimp32on64\py; Flags: restartreplace uninsrestartdelete ignoreversion
Source: "{#DEPS_DIR32}\bin\python2.exe"; DestDir: "{app}\32\bin"; DestName: "python.exe"; Components: gimp32on64\py; Flags: restartreplace uninsrestartdelete ignoreversion
Source: "{#DEPS_DIR32}\bin\libpython2.7.dll"; DestDir: "{app}\32\bin"; Components: gimp32on64\py; Flags: restartreplace uninsrestartdelete ignoreversion
Source: "{#DEPS_DIR32}\lib\python2.7\*"; DestDir: "{app}\32\lib\python2.7"; Components: gimp32on64\py; Flags: recursesubdirs restartreplace uninsrestartdelete ignoreversion
#endif
#endif //NOFILES

[InstallDelete]
Type: files; Name: "{app}\bin\gimp-?.?.exe"
Type: files; Name: "{app}\bin\gimp-?.??.exe"
Type: files; Name: "{app}\bin\gimp-console-?.?.exe"
Type: files; Name: "{app}\bin\gimp-console-?.??.exe"
Type: files; Name: "{app}\lib\gegl-0.1\*.dll"
;obsolete plugins from gimp 2.6
Type: files; Name: "{app}\lib\gimp\2.0\plug-ins\file-pdf.exe"
Type: files; Name: "{app}\lib\gimp\2.0\plug-ins\gee.exe"
Type: files; Name: "{app}\lib\gimp\2.0\plug-ins\gee-zoom.exe"
;old Python
Type: filesandordirs; Name: "{app}\Python\*"
Type: dirifempty; Name: "{app}\Python"
;remove incompatible version shipped with 2.8
Type: files; Name: "{app}\bin\zlib1.dll"
Type: files; Name: "{app}\32\bin\zlib1.dll"
;old ghostscript
Type: filesandordirs; Name: "{app}\share\ghostscript\*"
;obsolete plugins from gimp 2.8
Type: files; Name: "{app}\lib\babl-0.1\sse-fixups.dll"
Type: files; Name: "{app}\lib\gimp\2.0\plug-ins\pyconsole.py"
Type: files; Name: "{app}\lib\gimp\2.0\plug-ins\python-console.py"
Type: files; Name: "{app}\lib\gimp\2.0\plug-ins\metadata.exe"
Type: files; Name: "{app}\lib\gimp\2.0\plug-ins\file-psd-save.exe"
Type: files; Name: "{app}\lib\gimp\2.0\plug-ins\file-psd-load.exe"
Type: files; Name: "{app}\lib\gimp\2.0\plug-ins\alien-map.exe"
Type: files; Name: "{app}\lib\gimp\2.0\plug-ins\antialias.exe"
Type: files; Name: "{app}\lib\gimp\2.0\plug-ins\apply-canvas.exe"
Type: files; Name: "{app}\lib\gimp\2.0\plug-ins\blur-gauss.exe"
Type: files; Name: "{app}\lib\gimp\2.0\plug-ins\blur-gauss-selective.exe"
Type: files; Name: "{app}\lib\gimp\2.0\plug-ins\blur-motion.exe"
Type: files; Name: "{app}\lib\gimp\2.0\plug-ins\bump-map.exe"
Type: files; Name: "{app}\lib\gimp\2.0\plug-ins\channel-mixer.exe"
Type: files; Name: "{app}\lib\gimp\2.0\plug-ins\color-exchange.exe"
Type: files; Name: "{app}\lib\gimp\2.0\plug-ins\color-rotate.exe"
Type: files; Name: "{app}\lib\gimp\2.0\plug-ins\color-to-alpha.exe"
Type: files; Name: "{app}\lib\gimp\2.0\plug-ins\contrast-stretch.exe"
Type: files; Name: "{app}\lib\gimp\2.0\plug-ins\contrast-stretch-hsv.exe"
Type: files; Name: "{app}\lib\gimp\2.0\plug-ins\convolution-matrix.exe"
Type: files; Name: "{app}\lib\gimp\2.0\plug-ins\crop-auto.exe"
Type: files; Name: "{app}\lib\gimp\2.0\plug-ins\cubism.exe"
Type: files; Name: "{app}\lib\gimp\2.0\plug-ins\deinterlace.exe"
Type: files; Name: "{app}\lib\gimp\2.0\plug-ins\diffraction.exe"
Type: files; Name: "{app}\lib\gimp\2.0\plug-ins\displace.exe"
Type: files; Name: "{app}\lib\gimp\2.0\plug-ins\edge.exe"
Type: files; Name: "{app}\lib\gimp\2.0\plug-ins\edge-laplace.exe"
Type: files; Name: "{app}\lib\gimp\2.0\plug-ins\edge-sobel.exe"
Type: files; Name: "{app}\lib\gimp\2.0\plug-ins\engrave.exe"
Type: files; Name: "{app}\lib\gimp\2.0\plug-ins\file-raw.exe"
Type: files; Name: "{app}\lib\gimp\2.0\plug-ins\file-tiff-load.exe"
Type: files; Name: "{app}\lib\gimp\2.0\plug-ins\file-tiff-save.exe"
Type: files; Name: "{app}\lib\gimp\2.0\plug-ins\file-uri.exe"
Type: files; Name: "{app}\lib\gimp\2.0\plug-ins\illusion.exe"
Type: files; Name: "{app}\lib\gimp\2.0\plug-ins\iwarp.exe"
Type: files; Name: "{app}\lib\gimp\2.0\plug-ins\lcms.exe"
Type: files; Name: "{app}\lib\gimp\2.0\plug-ins\lens-apply.exe"
Type: files; Name: "{app}\lib\gimp\2.0\plug-ins\lens-distortion.exe"
Type: files; Name: "{app}\lib\gimp\2.0\plug-ins\lens-flare.exe"
Type: files; Name: "{app}\lib\gimp\2.0\plug-ins\maze.exe"
Type: files; Name: "{app}\lib\gimp\2.0\plug-ins\mosaic.exe"
Type: files; Name: "{app}\lib\gimp\2.0\plug-ins\noise-hsv.exe"
Type: files; Name: "{app}\lib\gimp\2.0\plug-ins\noise-randomize.exe"
Type: files; Name: "{app}\lib\gimp\2.0\plug-ins\noise-rgb.exe"
Type: files; Name: "{app}\lib\gimp\2.0\plug-ins\noise-solid.exe"
Type: files; Name: "{app}\lib\gimp\2.0\plug-ins\noise-spread.exe"
Type: files; Name: "{app}\lib\gimp\2.0\plug-ins\nova.exe"
Type: files; Name: "{app}\lib\gimp\2.0\plug-ins\pixelize.exe"
Type: files; Name: "{app}\lib\gimp\2.0\plug-ins\plasma.exe"
Type: files; Name: "{app}\lib\gimp\2.0\plug-ins\polar-coords.exe"
Type: files; Name: "{app}\lib\gimp\2.0\plug-ins\red-eye-removal.exe"
Type: files; Name: "{app}\lib\gimp\2.0\plug-ins\ripple.exe"
Type: files; Name: "{app}\lib\gimp\2.0\plug-ins\rotate.exe"
Type: files; Name: "{app}\lib\gimp\2.0\plug-ins\semi-flatten.exe"
Type: files; Name: "{app}\lib\gimp\2.0\plug-ins\shift.exe"
Type: files; Name: "{app}\lib\gimp\2.0\plug-ins\sinus.exe"
Type: files; Name: "{app}\lib\gimp\2.0\plug-ins\threshold-alpha.exe"
Type: files; Name: "{app}\lib\gimp\2.0\plug-ins\tile-glass.exe"
Type: files; Name: "{app}\lib\gimp\2.0\plug-ins\tile-paper.exe"
Type: files; Name: "{app}\lib\gimp\2.0\plug-ins\tile-seamless.exe"
Type: files; Name: "{app}\lib\gimp\2.0\plug-ins\value-invert.exe"
Type: files; Name: "{app}\lib\gimp\2.0\plug-ins\value-propagate.exe"
Type: files; Name: "{app}\lib\gimp\2.0\plug-ins\video.exe"
Type: files; Name: "{app}\lib\gimp\2.0\plug-ins\waves.exe"
Type: files; Name: "{app}\lib\gimp\2.0\plug-ins\whirl-pinch.exe"
Type: files; Name: "{app}\lib\gimp\2.0\plug-ins\win-snap.exe"
Type: files; Name: "{app}\lib\gimp\2.0\plug-ins\wind.exe"
Type: files; Name: "{app}\lib\gimp\2.0\plug-ins\text-brush.py"
;<2.10.6 plug-ins
Type: files; Name: "{app}\lib\gimp\2.0\plug-ins\align-layers.exe"
Type: files; Name: "{app}\lib\gimp\2.0\plug-ins\animation-optimize.exe"
Type: files; Name: "{app}\lib\gimp\2.0\plug-ins\animation-play.exe"
Type: files; Name: "{app}\lib\gimp\2.0\plug-ins\blinds.exe"
Type: files; Name: "{app}\lib\gimp\2.0\plug-ins\blur.exe"
Type: files; Name: "{app}\lib\gimp\2.0\plug-ins\border-average.exe"
Type: files; Name: "{app}\lib\gimp\2.0\plug-ins\busy-dialog.exe"
Type: files; Name: "{app}\lib\gimp\2.0\plug-ins\cartoon.exe"
Type: files; Name: "{app}\lib\gimp\2.0\plug-ins\checkerboard.exe"
Type: files; Name: "{app}\lib\gimp\2.0\plug-ins\cml-explorer.exe"
Type: files; Name: "{app}\lib\gimp\2.0\plug-ins\color-cube-analyze.exe"
Type: files; Name: "{app}\lib\gimp\2.0\plug-ins\color-enhance.exe"
Type: files; Name: "{app}\lib\gimp\2.0\plug-ins\colorify.exe"
Type: files; Name: "{app}\lib\gimp\2.0\plug-ins\colormap-remap.exe"
Type: files; Name: "{app}\lib\gimp\2.0\plug-ins\compose.exe"
Type: files; Name: "{app}\lib\gimp\2.0\plug-ins\contrast-normalize.exe"
Type: files; Name: "{app}\lib\gimp\2.0\plug-ins\contrast-retinex.exe"
Type: files; Name: "{app}\lib\gimp\2.0\plug-ins\crop-zealous.exe"
Type: files; Name: "{app}\lib\gimp\2.0\plug-ins\curve-bend.exe"
Type: files; Name: "{app}\lib\gimp\2.0\plug-ins\decompose.exe"
Type: files; Name: "{app}\lib\gimp\2.0\plug-ins\depth-merge.exe"
Type: files; Name: "{app}\lib\gimp\2.0\plug-ins\despeckle.exe"
Type: files; Name: "{app}\lib\gimp\2.0\plug-ins\destripe.exe"
Type: files; Name: "{app}\lib\gimp\2.0\plug-ins\edge-dog.exe"
Type: files; Name: "{app}\lib\gimp\2.0\plug-ins\edge-neon.exe"
Type: files; Name: "{app}\lib\gimp\2.0\plug-ins\emboss.exe"
Type: files; Name: "{app}\lib\gimp\2.0\plug-ins\file-bmp.exe"
Type: files; Name: "{app}\lib\gimp\2.0\plug-ins\file-cel.exe"
Type: files; Name: "{app}\lib\gimp\2.0\plug-ins\file-compressor.exe"
Type: files; Name: "{app}\lib\gimp\2.0\plug-ins\file-csource.exe"
Type: files; Name: "{app}\lib\gimp\2.0\plug-ins\file-darktable.exe"
Type: files; Name: "{app}\lib\gimp\2.0\plug-ins\file-desktop-link.exe"
Type: files; Name: "{app}\lib\gimp\2.0\plug-ins\file-dicom.exe"
Type: files; Name: "{app}\lib\gimp\2.0\plug-ins\file-exr.exe"
Type: files; Name: "{app}\lib\gimp\2.0\plug-ins\file-faxg3.exe"
Type: files; Name: "{app}\lib\gimp\2.0\plug-ins\file-fits.exe"
Type: files; Name: "{app}\lib\gimp\2.0\plug-ins\file-fli.exe"
Type: files; Name: "{app}\lib\gimp\2.0\plug-ins\file-gbr.exe"
Type: files; Name: "{app}\lib\gimp\2.0\plug-ins\file-gegl.exe"
Type: files; Name: "{app}\lib\gimp\2.0\plug-ins\file-gif-load.exe"
Type: files; Name: "{app}\lib\gimp\2.0\plug-ins\file-gif-save.exe"
Type: files; Name: "{app}\lib\gimp\2.0\plug-ins\file-gih.exe"
Type: files; Name: "{app}\lib\gimp\2.0\plug-ins\file-glob.exe"
Type: files; Name: "{app}\lib\gimp\2.0\plug-ins\file-header.exe"
Type: files; Name: "{app}\lib\gimp\2.0\plug-ins\file-heif.exe"
Type: files; Name: "{app}\lib\gimp\2.0\plug-ins\file-html-table.exe"
Type: files; Name: "{app}\lib\gimp\2.0\plug-ins\file-ico.exe"
Type: files; Name: "{app}\lib\gimp\2.0\plug-ins\file-jp2-load.exe"
Type: files; Name: "{app}\lib\gimp\2.0\plug-ins\file-jpeg.exe"
Type: files; Name: "{app}\lib\gimp\2.0\plug-ins\file-mng.exe"
Type: files; Name: "{app}\lib\gimp\2.0\plug-ins\file-pat.exe"
Type: files; Name: "{app}\lib\gimp\2.0\plug-ins\file-pcx.exe"
Type: files; Name: "{app}\lib\gimp\2.0\plug-ins\file-pdf-load.exe"
Type: files; Name: "{app}\lib\gimp\2.0\plug-ins\file-pdf-save.exe"
Type: files; Name: "{app}\lib\gimp\2.0\plug-ins\file-pix.exe"
Type: files; Name: "{app}\lib\gimp\2.0\plug-ins\file-png.exe"
Type: files; Name: "{app}\lib\gimp\2.0\plug-ins\file-pnm.exe"
Type: files; Name: "{app}\lib\gimp\2.0\plug-ins\file-ps.exe"
Type: files; Name: "{app}\lib\gimp\2.0\plug-ins\file-psd.exe"
Type: files; Name: "{app}\lib\gimp\2.0\plug-ins\file-psp.exe"
Type: files; Name: "{app}\lib\gimp\2.0\plug-ins\file-raw-data.exe"
Type: files; Name: "{app}\lib\gimp\2.0\plug-ins\file-raw-placeholder.exe"
Type: files; Name: "{app}\lib\gimp\2.0\plug-ins\file-rawtherapee.exe"
Type: files; Name: "{app}\lib\gimp\2.0\plug-ins\file-sgi.exe"
Type: files; Name: "{app}\lib\gimp\2.0\plug-ins\file-sunras.exe"
Type: files; Name: "{app}\lib\gimp\2.0\plug-ins\file-svg.exe"
Type: files; Name: "{app}\lib\gimp\2.0\plug-ins\file-tga.exe"
Type: files; Name: "{app}\lib\gimp\2.0\plug-ins\file-tiff.exe"
Type: files; Name: "{app}\lib\gimp\2.0\plug-ins\file-webp.exe"
Type: files; Name: "{app}\lib\gimp\2.0\plug-ins\file-wmf.exe"
Type: files; Name: "{app}\lib\gimp\2.0\plug-ins\file-xbm.exe"
Type: files; Name: "{app}\lib\gimp\2.0\plug-ins\file-xpm.exe"
Type: files; Name: "{app}\lib\gimp\2.0\plug-ins\file-xwd.exe"
Type: files; Name: "{app}\lib\gimp\2.0\plug-ins\film.exe"
Type: files; Name: "{app}\lib\gimp\2.0\plug-ins\filter-pack.exe"
Type: files; Name: "{app}\lib\gimp\2.0\plug-ins\flame.exe"
Type: files; Name: "{app}\lib\gimp\2.0\plug-ins\fractal-explorer.exe"
Type: files; Name: "{app}\lib\gimp\2.0\plug-ins\fractal-trace.exe"
Type: files; Name: "{app}\lib\gimp\2.0\plug-ins\gfig.exe"
Type: files; Name: "{app}\lib\gimp\2.0\plug-ins\gimpressionist.exe"
Type: files; Name: "{app}\lib\gimp\2.0\plug-ins\goat-exercise.exe"
Type: files; Name: "{app}\lib\gimp\2.0\plug-ins\gradient-flare.exe"
Type: files; Name: "{app}\lib\gimp\2.0\plug-ins\gradient-map.exe"
Type: files; Name: "{app}\lib\gimp\2.0\plug-ins\grid.exe"
Type: files; Name: "{app}\lib\gimp\2.0\plug-ins\guillotine.exe"
Type: files; Name: "{app}\lib\gimp\2.0\plug-ins\help-browser.exe"
Type: files; Name: "{app}\lib\gimp\2.0\plug-ins\help.exe"
Type: files; Name: "{app}\lib\gimp\2.0\plug-ins\hot.exe"
Type: files; Name: "{app}\lib\gimp\2.0\plug-ins\ifs-compose.exe"
Type: files; Name: "{app}\lib\gimp\2.0\plug-ins\imagemap.exe"
Type: files; Name: "{app}\lib\gimp\2.0\plug-ins\jigsaw.exe"
Type: files; Name: "{app}\lib\gimp\2.0\plug-ins\lighting.exe"
Type: files; Name: "{app}\lib\gimp\2.0\plug-ins\map-object.exe"
Type: files; Name: "{app}\lib\gimp\2.0\plug-ins\max-rgb.exe"
Type: files; Name: "{app}\lib\gimp\2.0\plug-ins\metadata-editor.exe"
Type: files; Name: "{app}\lib\gimp\2.0\plug-ins\metadata-viewer.exe"
Type: files; Name: "{app}\lib\gimp\2.0\plug-ins\newsprint.exe"
Type: files; Name: "{app}\lib\gimp\2.0\plug-ins\nl-filter.exe"
Type: files; Name: "{app}\lib\gimp\2.0\plug-ins\oilify.exe"
Type: files; Name: "{app}\lib\gimp\2.0\plug-ins\pagecurl.exe"
Type: files; Name: "{app}\lib\gimp\2.0\plug-ins\photocopy.exe"
Type: files; Name: "{app}\lib\gimp\2.0\plug-ins\plugin-browser.exe"
Type: files; Name: "{app}\lib\gimp\2.0\plug-ins\print.exe"
Type: files; Name: "{app}\lib\gimp\2.0\plug-ins\procedure-browser.exe"
Type: files; Name: "{app}\lib\gimp\2.0\plug-ins\qbist.exe"
Type: files; Name: "{app}\lib\gimp\2.0\plug-ins\sample-colorize.exe"
Type: files; Name: "{app}\lib\gimp\2.0\plug-ins\screenshot.exe"
Type: files; Name: "{app}\lib\gimp\2.0\plug-ins\script-fu.exe"
Type: files; Name: "{app}\lib\gimp\2.0\plug-ins\selection-to-path.exe"
Type: files; Name: "{app}\lib\gimp\2.0\plug-ins\sharpen.exe"
Type: files; Name: "{app}\lib\gimp\2.0\plug-ins\smooth-palette.exe"
Type: files; Name: "{app}\lib\gimp\2.0\plug-ins\softglow.exe"
Type: files; Name: "{app}\lib\gimp\2.0\plug-ins\sparkle.exe"
Type: files; Name: "{app}\lib\gimp\2.0\plug-ins\sphere-designer.exe"
Type: files; Name: "{app}\lib\gimp\2.0\plug-ins\tile-small.exe"
Type: files; Name: "{app}\lib\gimp\2.0\plug-ins\tile.exe"
Type: files; Name: "{app}\lib\gimp\2.0\plug-ins\twain.exe"
Type: files; Name: "{app}\lib\gimp\2.0\plug-ins\unit-editor.exe"
Type: files; Name: "{app}\lib\gimp\2.0\plug-ins\unsharp-mask.exe"
Type: files; Name: "{app}\lib\gimp\2.0\plug-ins\van-gogh-lic.exe"
Type: files; Name: "{app}\lib\gimp\2.0\plug-ins\warp.exe"
Type: files; Name: "{app}\lib\gimp\2.0\plug-ins\wavelet-decompose.exe"
Type: files; Name: "{app}\lib\gimp\2.0\plug-ins\web-browser.exe"
Type: files; Name: "{app}\lib\gimp\2.0\plug-ins\web-page.exe"
Type: files; Name: "{app}\lib\gimp\2.0\plug-ins\colorxhtml.py"
Type: files; Name: "{app}\lib\gimp\2.0\plug-ins\file-openraster.py"
Type: files; Name: "{app}\lib\gimp\2.0\plug-ins\foggify.py"
Type: files; Name: "{app}\lib\gimp\2.0\plug-ins\gradients-save-as-css.py"
Type: files; Name: "{app}\lib\gimp\2.0\plug-ins\histogram-export.py"
Type: files; Name: "{app}\lib\gimp\2.0\plug-ins\palette-offset.py"
Type: files; Name: "{app}\lib\gimp\2.0\plug-ins\palette-sort.py"
Type: files; Name: "{app}\lib\gimp\2.0\plug-ins\palette-to-gradient.py"
Type: files; Name: "{app}\lib\gimp\2.0\plug-ins\py-slice.py"
Type: files; Name: "{app}\lib\gimp\2.0\plug-ins\python-eval.py"
Type: files; Name: "{app}\lib\gimp\2.0\plug-ins\python-console\pyconsole.py"
Type: files; Name: "{app}\lib\gimp\2.0\plug-ins\python-console\python-console.py"
;DLLs in plug-ins directory (see bug 796225)
Type: files; Name: "{app}\lib\gimp\2.0\plug-ins\*.dll"
;gegl 0.2
Type: filesandordirs; Name: "{app}\lib\gegl-0.2"
;old icons
Type: files; Name: "{commonprograms}\GIMP 2.lnk"
Type: files; Name: "{commondesktop}\GIMP 2.lnk"
;get previous GIMP icon name from uninstall name in Registry
Type: files; Name: "{commonprograms}\{reg:HKLM\SOFTWARE\Microsoft\Windows\CurrentVersion\Uninstall\GIMP-2_is1,DisplayName|GIMP 2}.lnk"; Check: CheckRegValueExists('SOFTWARE\Microsoft\Windows\CurrentVersion\Uninstall\GIMP-2_is1','DisplayName')
Type: files; Name: "{commondesktop}\{reg:HKLM\SOFTWARE\Microsoft\Windows\CurrentVersion\Uninstall\GIMP-2_is1,DisplayName|GIMP 2}.lnk"; Check: CheckRegValueExists('SOFTWARE\Microsoft\Windows\CurrentVersion\Uninstall\GIMP-2_is1','DisplayName')

[Registry]
;fix broken toolbox icons
Root: HKLM; Subkey: "Software\Classes\.svg"; ValueType: string; ValueName: "Content Type"; ValueData: "image/svg+xml"; Flags: noerror createvalueifdoesntexist
;remove LIBTHAI_DICTDIR variable set by original 2.10.8 installer
Root: HKLM; Subkey: "SYSTEM\CurrentControlSet\Control\Session Manager\Environment"; ValueType: none; ValueName: "LIBTHAI_DICTDIR"; Flags: deletevalue uninsdeletevalue

[UninstallDelete]
Type: files; Name: "{app}\uninst\uninst.inf"
;need to clean out all the generated .pyc files
Type: filesandordirs; Name: "{app}\Python\*"

[Code]

function WideCharToMultiByte(CodePage: Cardinal; dwFlags: DWORD; lpWideCharStr: String; cchWideCharStr: Integer;
                             lpMultiByteStr: PAnsiChar; cbMultiByte: Integer; lpDefaultChar: Integer;
                             lpUsedDefaultChar: Integer): Integer; external 'WideCharToMultiByte@Kernel32 stdcall';

function MultiByteToWideChar(CodePage: Cardinal; dwFlags: DWORD; lpMultiByteStr: PAnsiChar; cbMultiByte: Integer;
                             lpWideCharStr: String; cchWideChar: Integer): Integer;
                             external 'MultiByteToWideChar@Kernel32 stdcall';

function GetLastError(): DWORD; external 'GetLastError@Kernel32 stdcall';

function GetSysColor(nIndex: Integer): DWORD; external 'GetSysColor@user32.dll stdcall';

function IsProcessorFeaturePresent(ProcessorFeature: DWORD): LongBool; external 'IsProcessorFeaturePresent@kernel32 stdcall';

//functions needed to get BPP
function GetDC(hWnd: Integer): Integer; external 'GetDC@User32 stdcall';
function ReleaseDC(hWnd, hDC: Integer): Integer; external 'ReleaseDC@User32 stdcall';
function GetDeviceCaps(hDC, nIndex: Integer): Integer; external 'GetDeviceCaps@GDI32 stdcall';


procedure ComponentsListOnClick(pSender: TObject); forward;
procedure SaveToUninstInf(const pText: AnsiString); forward;
procedure CreateRunOnceEntry; forward;
function RestartSetupAfterReboot(): Boolean; forward;

const
	CP_ACP = 0;
	CP_UTF8 = 65001;

	COLOR_HOTLIGHT = 26;

	PF_XMMI_INSTRUCTIONS_AVAILABLE = 6;

	BITSPIXEL = 12;
	PLANES = 14;

	GIMP_URL = 'https://www.gimp.org/';

	RTFHeader = '{\rtf1\deff0{\fonttbl{\f0\fswiss\fprq2\fcharset0 Tahoma;}{\f1\fnil\fcharset2 Symbol;}}\viewkind4\uc1\fs16';
	//RTFBullet = '{\pntext\f1\''B7\tab}';
	RTFPara	  = '\par ';

	RunOnceName = 'Resume GIMP {#VERSION} install';

	CONFIG_OVERRIDE_PARAM = 'configoverride';

	UNINSTALL_MAX_WAIT_TIME = 10000;
	UNINSTALL_CHECK_TIME    =   250;

type
	TRemoveOldGIMPResult = (rogContinue, rogRestartRequired, rogUninstallFailed, rogCantUninstall);

var
	lblComponentDescription: TNewStaticText;

	SetupINI: String;

	ReadyMemoRichText: String;

	WelcomeBitmapBottom: TBitmapImage;

	Associations: record
		AssociationsPage: record
			Page: TWizardPage;
			clbAssociations: TNewCheckListBox;
			lblAssocInfo2: TNewStaticText;
		end;
		Association: array of record
			Description: String;
			Extensions: TArrayOfString;
			Selected: Boolean;
			Associated: Boolean;
			AssociatedElsewhere: String;
		end;
	end;

	//pgSimple: TWizardPage;
	btnInstall, btnCustomize: TNewButton;

	InstallMode: (imNone, imSimple, imCustom, imRebootContinue);

	ConfigOverride: (coUndefined, coOverride, coDontOverride);

	Force32bitInstall: Boolean;

	asUninstInf: TArrayOfString; //uninst.inf contents (loaded at start of uninstall, executed at the end)


#include "MessageWithURL.isi"

function Check3264(const pWhich: String): Boolean;
begin
	if pWhich = '64' then
		Result := Is64BitInstallMode() and (not Force32bitInstall)
	else if pWhich = '32' then
		Result := (not Is64BitInstallMode()) or Force32bitInstall
	else
		RaiseException('Unknown check');
end;


#include "utils.isi"


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


function CheckRegValueExists(const SubKeyName, ValueName: String): Boolean;
begin
	Result := RegValueExists(HKEY_LOCAL_MACHINE, SubKeyName, ValueName);
end;


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


#include "associations.isi"


procedure PreparePyGimp();
var PyGimpInterp,Interp: String;
begin
	if IsComponentSelected('py') or IsComponentSelected('gimp32on64\py') then
	begin
		StatusLabel(CustomMessage('SettingUpPyGimp'),'');

		PyGimpInterp := ExpandConstant('{app}\lib\gimp\2.0\interpreters\pygimp.interp');
        DebugMsg('PreparePyGimp','Writing interpreter file for gimp-python: ' + PyGimpInterp);
		
		if IsComponentSelected('py') then
		begin
			Interp := 'python=' + ExpandConstant('{app}\bin\pythonw.exe') + #10 +
			          'python2=' + ExpandConstant('{app}\bin\pythonw.exe') + #10 +
			          '/usr/bin/python=' + ExpandConstant('{app}\bin\pythonw.exe') + #10':Python:E::py::python:'#10;
		end else
		begin
			Interp := 'python=' + ExpandConstant('{app}\32\bin\pythonw.exe') + #10 +
			          'python2=' + ExpandConstant('{app}\32\bin\pythonw.exe') + #10 +
			          '/usr/bin/python=' + ExpandConstant('{app}\32\bin\pythonw.exe') + #10':Python:E::py::python:'#10;
		end;

		
		if not SaveStringToUTF8File(PyGimpInterp,Interp,False) then
		begin
			DebugMsg('PreparePyGimp','Problem writing the file. [' + Interp + ']');
			SuppressibleMsgBox(CustomMessage('ErrorUpdatingPython') + ' (2)',mbInformation,mb_ok,IDOK);
		end;

	end;
end;


procedure PrepareGimpEnvironment();
var EnvFile,Env: String;
begin
	
	StatusLabel(CustomMessage('SettingUpEnvironment'),'');

	//set PATH to be used by plug-ins
	EnvFile := ExpandConstant('{app}\lib\gimp\2.0\environ\default.env');
	DebugMsg('PrepareGimpEnvironment','Setting environment in ' + EnvFile);

	Env := #10'PATH=${gimp_installation_dir}\bin';
	
	if IsComponentSelected('gimp32on64') then
	begin

		Env := Env + ';${gimp_installation_dir}\32\bin' + #10;

		if IsComponentSelected('gimp32on64\py') then
			Env := Env + 'PYTHONPATH=${gimp_installation_dir}\32\lib\gimp\2.0\python;${gimp_plug_in_dir}\plug-ins\python-console' + #10;

	end else
	begin

		Env := Env + #10;

	end;

	if IsComponentSelected('py') then
		Env := Env + 'PYTHONPATH=${gimp_installation_dir}\lib\gimp\2.0\python;${gimp_plug_in_dir}\plug-ins\python-console' + #10;

	DebugMsg('PrepareGimpEnvironment','Appending ' + Env);

	if not SaveStringToUTF8File(EnvFile,Env,True) then
	begin
		DebugMsg('PrepareGimpEnvironment','Problem appending');
		SuppressibleMsgBox(FmtMessage(CustomMessage('ErrorChangingEnviron'),[EnvFile]),mbInformation,mb_ok,IDOK);
	end;

	//workaround for high-DPI awareness of Python plug-ins
	if IsComponentSelected('gimp32on64\py') or IsComponentSelected('py') then
	begin
		EnvFile := ExpandConstant('{app}\lib\gimp\2.0\environ\pygimp.env');
		DebugMsg('PrepareGimpEnvironment','Setting environment in ' + EnvFile);

		Env := '__COMPAT_LAYER=HIGHDPIAWARE' + #10

		if not SaveStringToUTF8File(EnvFile,Env,True) then
		begin
			DebugMsg('PrepareGimpEnvironment','Problem appending');
			SuppressibleMsgBox(FmtMessage(CustomMessage('ErrorChangingEnviron'),[EnvFile]),mbInformation,mb_ok,IDOK);
		end;
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


procedure InitCustomPages();
var lblAssocInfo: TNewStaticText;
	btnSelectAll,btnSelectUnused: TNewButton;
	i,ButtonWidth: Integer;
	ButtonText: TArrayOfString;
	MeasureLabel: TNewStaticText;
	//lblInfo: TNewStaticText;
begin
	DebugMsg('InitCustomPages','pgAssociations');
	Associations.AssociationsPage.Page := CreateCustomPage(wpSelectComponents,CustomMessage('SelectAssociationsCaption'),CustomMessage('SelectAssociationsCaption'));

	lblAssocInfo := TNewStaticText.Create(Associations.AssociationsPage.Page);
	with lblAssocInfo do
	begin
		Parent := Associations.AssociationsPage.Page.Surface;
		Left := 0;
		Top := 0;
		Width := Associations.AssociationsPage.Page.SurfaceWidth;
		Height := ScaleY(13);
		WordWrap := True;
		AutoSize := True;
		Caption := CustomMessage('SelectAssociationsInfo1')+#13+CustomMessage('SelectAssociationsInfo2')+#13;
	end;	
	Associations.AssociationsPage.lblAssocInfo2 := TNewStaticText.Create(Associations.AssociationsPage.Page);
	with Associations.AssociationsPage.lblAssocInfo2 do
	begin
		Parent := Associations.AssociationsPage.Page.Surface;
		Left := 0;
		Width := Associations.AssociationsPage.Page.SurfaceWidth;
		Height := 13;
		AutoSize := True;
		Caption := #13+CustomMessage('SelectAssociationsExtensions');
	end;
	
	Associations.AssociationsPage.clbAssociations := TNewCheckListBox.Create(Associations.AssociationsPage.Page);
	with Associations.AssociationsPage.clbAssociations do
	begin
		Parent := Associations.AssociationsPage.Page.Surface;
		Left := 0;
		Top := lblAssocInfo.Top + lblAssocInfo.Height;
		Width := Associations.AssociationsPage.Page.SurfaceWidth;
		Height := Associations.AssociationsPage.Page.SurfaceHeight - lblAssocInfo.Height - Associations.AssociationsPage.lblAssocInfo2.Height - ScaleX(8);
		TabOrder := 1;
		Flat := True;

                // the box's height, minus the 2-pixel border, has to be a
                // multiple of the item height, which has to be even, otherwise
                // clicking the last visible item may scroll to the next item,
                // causing it to be toggled.  see bug #786840.
                MinItemHeight := ScaleY(16) / 2 * 2;
                Height := (Height - 4) / MinItemHeight * MinItemHeight + 4;

		Associations.AssociationsPage.lblAssocInfo2.Top := Height + Top;

		OnClick := @Associations_OnClick;

		for i := 0 to GetArrayLength(Associations.Association) - 1 do
			AddCheckBox(Associations.Association[i].Description,
			            '',
			            0,
			            Associations.Association[i].Selected,
			            not Associations.Association[i].Associated, //don't allow unchecking associations that are already present
			            False,
			            False,
			            nil
			           );			

	end;

	SetArrayLength(ButtonText, 3);
	ButtonText[0] := CustomMessage('SelectAssociationsSelectUnused');
	ButtonText[1] := CustomMessage('SelectAssociationsSelectAll');
	ButtonText[2] := CustomMessage('SelectAssociationsUnselectAll');
	ButtonWidth := GetButtonWidth(ButtonText, WizardForm.NextButton.Width);

	btnSelectUnused := TNewButton.Create(Associations.AssociationsPage.Page);
	with btnSelectUnused do
	begin
		Parent := Associations.AssociationsPage.Page.Surface;
		Width := ButtonWidth;
		Left := Associations.AssociationsPage.Page.SurfaceWidth - Width * 2;
		Height := WizardForm.NextButton.Height;
		Top := Associations.AssociationsPage.clbAssociations.Top + Associations.AssociationsPage.clbAssociations.Height + ScaleX(8);
		Caption := CustomMessage('SelectAssociationsSelectUnused');
		OnClick := @Associations_SelectUnused;
	end;

	btnSelectAll := TNewButton.Create(Associations.AssociationsPage.Page);
	with btnSelectAll do
	begin
		Parent := Associations.AssociationsPage.Page.Surface;
		Width := ButtonWidth;
		Left := Associations.AssociationsPage.Page.SurfaceWidth - Width;
		Height := WizardForm.NextButton.Height;
		Top := Associations.AssociationsPage.clbAssociations.Top + Associations.AssociationsPage.clbAssociations.Height + ScaleX(8);
		Caption := CustomMessage('SelectAssociationsSelectAll');
		OnClick := @Associations_SelectAll;
	end;

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
		Height := ScaleY(16);
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


procedure ComponentsListOnClick(pSender: TObject);
var i,j: Integer;
	Components: TArrayOfString;
	ComponentDesc: String;
begin
	DebugMsg('ComponentsListOnClick','');

	Components := ['Gimp','Deps','Debug','GtkWimp','Translations','MyPaint','Python','Ghostscript','Gimp32','Compat'];
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

	for i := 0 to GetArrayLength(Associations.Association) - 1 do
		if Associations.Association[i].Selected then
			bShowAssoc := True;

	if bShowAssoc then
	begin
		sText := sText + '\sb100\b '+Unicode2RTF(CustomMessage('ReadyMemoAssociations'))+'\par \sb0\li284\b0 '; //which file types to associate

		for i := 0 to GetArrayLength(Associations.Association) - 1 do							
			if Associations.Association[i].Selected then
				for j := 0 to GetArrayLength(Associations.Association[i].Extensions) - 1 do
					sText := sText + LowerCase(Associations.Association[i].Extensions[j]) + ', ';
					
		sText := Copy(sText, 1, Length(sText) - 2) + '\par \pard';

	end;

	If pMemoTasksInfo<>'' then
		sText := sText + '\sb100' + ParseReadyMemoText(pSpace,pMemoTasksInfo);
		
	ReadyMemoRichText := Copy(sText,1,Length(sText)-6) + '}';

	Result := 'If you see this, something went wrong';
end;



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
				NewBitmap2 := TFileStream.Create(ExpandConstant('{tmp}\windows-installer-intro-small.bmp'),fmOpenRead);
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


procedure InfoBeforeLikeLicense();
begin
	WizardForm.InfoBeforeClickLabel.Visible := False;
	WizardForm.InfoBeforeMemo.Height := WizardForm.InfoBeforeMemo.Height + WizardForm.InfoBeforeMemo.Top - WizardForm.InfoBeforeClickLabel.Top;
	WizardForm.InfoBeforeMemo.Top := WizardForm.InfoBeforeClickLabel.Top;
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


function ShouldSkipPage(pPageID: Integer): Boolean;
begin
	DebugMsg('ShouldSkipPage','ID: '+IntToStr(pPageID));

	Result := ((InstallMode = imSimple) or (InstallMode = imRebootContinue)) and (pPageID <> wpFinished);
	                                                               //skip all pages except for the finished one if using simple
	                                                               //install mode
	if Result then
		DebugMsg('ShouldSkipPage','Yes')
	else
		DebugMsg('ShouldSkipPage','No');

end;


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

//remove .debug files from previous installs - there's no built-in way in Inno to recursivly delete files with wildcard+extension
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


procedure CurStepChanged(pCurStep: TSetupStep);
begin
	case pCurStep of
		ssInstall:
		begin
			RemoveDebugFiles();
		end;
		ssPostInstall:
		begin
			Associations_Create();
			PreparePyGimp();
			PrepareGimpEnvironment();
			//PrepareGimpRC();
		end;
	end;
end;


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
		Result := FmtMessage(CustomMessage('RemovingOldVersionFailed'),['{#VERSION}',ExpandConstant('{app}')]);
	end else
	if RemoveResult = rogCantUninstall then
	begin
		DebugMsg('PrepareToInstall','RemoveOldGIMPVersions failed to uninstall old GIMP version [1]');
		Result := FmtMessage(CustomMessage('RemovingOldVersionCantUninstall'),['{#VERSION}',ExpandConstant('{app}')]);
	end else
	begin
		DebugMsg('PrepareToInstall','Internal error 11');
		Result := FmtMessage(CustomMessage('InternalError'),['11']); //should never happen
	end;

end;


procedure InitializeWizard();
begin
	UpdateWizardImages();
	InitCustomPages();
end;


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


function InitializeSetup(): Boolean;
#if (Defined(DEVEL) && DEVEL != "") || Defined(DEVEL_WARNING)
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

#if (Defined(DEVEL) && DEVEL != "") || Defined(DEVEL_WARNING)
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
		ExtractTemporaryFile('setup.ini');
		SetupINI := ExpandConstant('{tmp}\setup.ini');
		ExtractTemporaryFile('windows-installer-intro-small.bmp');
		ExtractTemporaryFile('installsplash.bmp');
		ExtractTemporaryFile('installsplash_small.bmp');
	except
		DebugMsg('InitializeSetup','Error extracting temporary file: ' + GetExceptionMessage);
		MsgBox(CustomMessage('ErrorExtractingTemp') + #13#13 + GetExceptionMessage,mbError,MB_OK);
		Result := False;
		exit;
	end;

	Associations_Init();

	//if InstallMode <> imRebootContinue then
	//	SuppressibleMsgBox(CustomMessage('UninstallWarning'),mbError,MB_OK,IDOK);

end;


procedure RegisterPreviousData(PreviousDataKey: Integer);
begin
	if Is64BitInstallMode() then
		SetPreviousData(PreviousDataKey,'32BitMode',BoolToStr(Force32bitInstall));
end;


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

#include "rebootcontinue.isi"

#include "uninst.isi"

#expr SaveToFile(AddBackslash(SourcePath) + "Preprocessed.iss")

