[Messages]
;InfoBefore page is used instead of license page because the GPL only concerns distribution, not use, and as such doesn't have to be accepted
WizardInfoBefore=License Agreement
AboutSetupNote=Setup built by Jernej Simonèiè, jernej-gimp@ena.si%n%nImage on opening page of Setup by Alexia_Death%nImage on closing page of Setup by Jakub Steiner
WinVersionTooLowError=This version of GIMP requires Windows XP with Service Pack 3, or a newer version of Windows.

[CustomMessages]
;shown before the wizard starts on development versions of GIMP
DevelopmentWarningTitle=Development version
DevelopmentWarning=This is a development version of GIMP installer. It hasn't been tested as much as the stable installer, which can result in GIMP not working properly. Please report any problems you encounter in the GIMP bugzilla (Installer component):%n_https://bugzilla.gnome.org/enter_bug.cgi?product=GIMP%n%nDo you wish to continue with installation anyway?
DevelopmentButtonContinue=&Continue
DevelopmentButtonExit=Exit

;XPSP3Recommended=Warning: you are running an unsupported version of Windows. Please update to at least Windows XP with Service Pack 3 before reporting any problems.
SSERequired=This version of GIMP requires a processor that supports SSE instructions.

Require32BPPTitle=Display settings problem
Require32BPP=Setup has detected that your Windows is not running in 32 bits-per-pixel display mode. This has been known to cause stability problems with GIMP, so it's recommended to change the display colour depth to 32BPP before continuing.
Require32BPPContinue=&Continue
Require32BPPExit=E&xit

InstallOrCustomize=GIMP is now ready to be installed. Click the Install now button to install using the default settings, or click the Customize button if you'd like to have more control over what gets installed.
Install=&Install
Customize=&Customize

;setup types
TypeCompact=Compact installation
TypeCustom=Custom installation
TypeFull=Full installation

;text above component description
ComponentsDescription=Description
;components
ComponentsGimp=GIMP
ComponentsGimpDescription=GIMP and all default plug-ins
ComponentsDeps=Run-time libraries
ComponentsDepsDescription=Run-time libraries used by GIMP, including GTK+ Run-time Environment
ComponentsGtkWimp=MS-Windows engine for GTK+
ComponentsGtkWimpDescription=Native Windows look for GIMP
ComponentsCompat=Support for old plug-ins
ComponentsCompatDescription=Install libraries needed by old third-party plug-ins
ComponentsTranslations=Translations
ComponentsTranslationsDescription=Translations
ComponentsPython=Python scripting
ComponentsPythonDescription=Allows you to use GIMP plugins written in Python scripting language.
ComponentsGhostscript=PostScript support
ComponentsGhostscriptDescription=Allow GIMP to load PostScript files
;only when installing on x64 Windows
ComponentsGimp32=Support for 32-bit plug-ins
ComponentsGimp32Description=Include files necessary for using 32-bit plug-ins.%nRequired for Python support.

;additional installation tasks
AdditionalIcons=Additional icons:
AdditionalIconsDesktop=Create a &desktop icon
AdditionalIconsQuickLaunch=Create a &Quick Launch icon

RemoveOldGIMP=Remove previous GIMP version

;%1 is replaced by file name; these messages should never appear (unless user runs out of disk space at the exact right moment)
ErrorChangingEnviron=There was a problem updating GIMP's environment in %1. If you get any errors loading the plug-ins, try uninstalling and re-installing GIMP.
ErrorExtractingTemp=Error extracting temporary data.
ErrorUpdatingPython=Error updating Python interpreter info.
ErrorReadingGimpRC=There was an error updating %1.
ErrorUpdatingGimpRC=There was an error updating GIMP's configuration file %1.

;displayed in Explorer's right-click menu
OpenWithGimp=Edit with GIMP

;file associations page
SelectAssociationsCaption=Select file associations
SelectAssociationsExtensions=Extensions:
SelectAssociationsInfo1=Select the file types you wish to associate with GIMP
SelectAssociationsInfo2=This will make selected files open in GIMP when you double-click them in Explorer.
SelectAssociationsSelectAll=Select &All
SelectAssociationsUnselectAll=Unselect &All
SelectAssociationsSelectUnused=Select &Unused

;shown on summary screen just before starting the install
ReadyMemoAssociations=File types to associate with GIMP:

RemovingOldVersion=Removing previous version of GIMP:
;%1 = version, %2 = installation directory
;ran uninstaller, but it returned an error, or didn't remove everything
RemovingOldVersionFailed=GIMP %1 cannot be installed over your currently installed GIMP version, and the automatic uninstall of old version has failed.%n%nPlease remove the previous version of GIMP yourself before installing this version in %2, or choose a Custom install, and select a different installation folder.%n%nThe Setup will now exit.
;couldn't find an uninstaller, or found several uninstallers
RemovingOldVersionCantUninstall=GIMP %1 cannot be installed over your currently installed GIMP version, and Setup couldn't determine how to remove the old version automatically.%n%nPlease remove the previous version of GIMP and any add-ons yourself before installing this version in %2, or choose a Custom install, and select a different installation folder.%n%nThe Setup will now exit.

RebootRequiredFirst=Previous GIMP version was removed successfully, but Windows has to be restarted before the Setup can continue.%n%nAfter restarting your computer, Setup will continue next time an administrator logs in.

;displayed if restart settings couldn't be read, or if the setup couldn't re-run itself
ErrorRestartingSetup=There was an error restarting the Setup. (%1)

;displayed while the files are being extracted; note the capitalisation!
Billboard1=Remember: GIMP is Free Software.%n%nPlease visit
;www.gimp.org (displayed between Billboard1 and Billboard2)
Billboard2=for free updates.

SettingUpAssociations=Setting up file associations...
SettingUpPyGimp=Setting up environment for GIMP Python extension...
SettingUpEnvironment=Setting up GIMP environment...
SettingUpGimpRC=Setting up GIMP configuration for 32-bit plug-in support...

;displayed on last page
LaunchGimp=Launch GIMP

;shown during uninstall when removing add-ons
UninstallingAddOnCaption=Removing add-on

InternalError=Internal error (%1).

;used by installer for add-ons (currently only help)
DirNotGimp=GIMP does not appear to be installed in the selected directory. Continue anyway?
