[Messages]
;InfoBefore page is used instead of license page because the GPL only concerns distribution, not use, and as such doesn't have to be accepted
WizardInfoBefore=Licensaftale
AboutSetupNote=Installationen er lavet af Jernej Simonèiè, jernej-gimp@ena.si%n%nBilledet på åbningssiden er lavet af Alexia_Death%nBilledet på afslutningssiden er lavet af Jakub Steiner
WinVersionTooLowError=Denne version af GIMP kræver Windows XP med Service Pack 3, eller en nyere version af Windows.

[CustomMessages]
;shown before the wizard starts on development versions of GIMP
DevelopmentWarningTitle=Development version
;DevelopmentWarning=This is a development version of GIMP. As such, some features aren't finished, and it may be unstable. If you encounter any problems, first verify that they haven't already been fixed in GIT before you contact the developers.%nThis version of GIMP is not intended for day-to-day work, as it may be unstable, and you could lose your work. Do you wish to continue with installation anyway?
DevelopmentWarning=Dette er en development version af installationsprogrammet til GIMP. Det er ikke blevet testet lige så meget som det stabile installationsprogram, hvilket kan resultere i at GIMP ikke virker korrekt. Rapportér venligst de problemer du støder på i GIMP bugzilla (Installer komponent):%n_https://bugzilla.gnome.org/enter_bug.cgi?product=GIMP%n%nØnsker du alligevel at fortsætte installation?
DevelopmentButtonContinue=&Fortsæt
DevelopmentButtonExit=Afslut

;XPSP3Recommended=Warning: you are running an unsupported version of Windows. Please update to at least Windows XP with Service Pack 3 before reporting any problems.
SSERequired=Denne version af GIMP kræver en processor der understøtter SSE-instruktioner.

Require32BPPTitle=Problemer med skærmindstillinger
Require32BPP=Installationen har registreret at Windows skærmindstillinger ikke anvender 32-bits-per-pixel. Det er kendt for at skabe stabilitetsproblemer for GIMP, så det anbefales at ændre skærmens farvedybde til ægte farver (32 bit) før du fortsætter.
Require32BPPContinue=&Fortsæt
Require32BPPExit=A&fslut

InstallOrCustomize=GIMP er nu klar til at blive installeret. Klik på Installer nu-knappen for at installere med standardindstillingerne, eller klik på Brugerdefineret-knappen hvis du ønsker at vælge hvad der skal installeres.
Install=&Installer
Customize=&Brugerdefineret

;setup types
TypeCompact=Kompakt installation
TypeCustom=Brugerdefineret installation
TypeFull=Fuld installation

;text above component description
ComponentsDescription=Beskrivelse
;components
ComponentsGimp=GIMP
ComponentsGimpDescription=GIMP og alle standard plugins
ComponentsDeps=Afviklingsbiblioteker
ComponentsDepsDescription=Afviklingsbiblioteker som GIMP anvender, inklusiv GTK+ afviklingsmiljø
ComponentsGtkWimp=MS-Windows-motor til GTK+
ComponentsGtkWimpDescription=Standard Windows udseende til GIMP
ComponentsCompat=Understøttelse af gamle plugins
ComponentsCompatDescription=Installer biblioteker der kræves til gamle tredjeparts plugins
ComponentsTranslations=Oversættelser
ComponentsTranslationsDescription=Oversættelser
ComponentsPython=Python-scripting
ComponentsPythonDescription=Giver mulighed for at bruge GIMP-plug-ins som er skrevet i Python-scripting-sproget.
ComponentsGhostscript=PostScript understøttelse
ComponentsGhostscriptDescription=GIMP fås mulighed for at indlæse PostScript-filer
;only when installing on x64 Windows
ComponentsGimp32=Understøttelse af 32-bit plugins
ComponentsGimp32Description=Inkludere filer der er nødvendige for at anvende 32-bit plugins.%nPåkrævet for understøttelse af Python.

;additional installation tasks
AdditionalIcons=Yderligere ikoner:
AdditionalIconsDesktop=Opret ikon på &skrivebordet
AdditionalIconsQuickLaunch=Opret ikon i &Hurtig start

RemoveOldGIMP=Fjern forrige GIMP-version

;%1 is replaced by file name; these messages should never appear (unless user runs out of disk space at the exact right moment)
ErrorChangingEnviron=Der opstod et problem ved opdatering af GIMP's-miljø i %1. Hvis du får fejl ved indlæsning af plugins, så prøv at afinstallere og geninstaller GIMP.
ErrorExtractingTemp=Fejl ved udtrækning af midlertidige data.
ErrorUpdatingPython=Fejl ved opdatering af Python-fortolker information.
ErrorReadingGimpRC=Der opstod en fejl ved opdatering af %1.
ErrorUpdatingGimpRC=Der opstod en fejl ved opdatering af GIMP's konfigurationsfil %1.

;displayed in Explorer's right-click menu
OpenWithGimp=Rediger i GIMP

;file associations page
SelectAssociationsCaption=Vælg filtilknytninger
SelectAssociationsExtensions=Filtyper:
SelectAssociationsInfo1=Vælg de filtyper som du have tilknyttet med GIMP
SelectAssociationsInfo2=Markerede filer åbnes i GIMP, når du dobbeltklikker på dem i Stifinder.
SelectAssociationsSelectAll=Vælg &alle
SelectAssociationsUnselectAll=Fravælg &alle
SelectAssociationsSelectUnused=Vælg &ubrugte

;shown on summary screen just before starting the install
ReadyMemoAssociations=Filtyper der skal tilknyttes GIMP:

RemovingOldVersion=Fjerner forrige version af GIMP:
;%1 = version, %2 = installation directory
;ran uninstaller, but it returned an error, or didn't remove everything
RemovingOldVersionFailed=GIMP %1 kan ikke installeres oven på den GIMP-version der er installeret i øjeblikket, og automatisk afinstallation af gamle versioner mislykkedes.%n%nFjern venligst selv den forrige version af GIMP, før denne version installeres i %2, eller vælg brugerdefineret installation, og vælg en anden installationsmappe.%n%nInstallationen vil nu blive afsluttet.
;couldn't find an uninstaller, or found several uninstallers
RemovingOldVersionCantUninstall=GIMP %1 kan ikke installeres oven på den GIMP-version der er installeret i øjeblikket, og installationen var ikke i stand til at fastslå hvordan den gamle version kunne fjernes.%n%nFjern venligst selv den forrige version af GIMP og alle tilføjelser, før denne version installeres i %2, eller vælg brugerdefineret installation, og vælg en anden installationsmappe.%n%nInstallationen vil nu blive afsluttet.

RebootRequiredFirst=Forrige GIMP-version blev fjernet, men Windows skal genstartes før installationen kan fortsætte.%n%nEfter computeren er blevet genstartet vil installationen fortsætte, næste gang en administrator logger på.

;displayed if restart settings couldn't be read, or if the setup couldn't re-run itself
ErrorRestartingSetup=Der opstod en fejl ved genstart af installationen. (%1)

;displayed while the files are being extracted; note the capitalisation!
Billboard1=Husk: GIMP er fri software.%n%nBesøg venligst
;www.gimp.org (displayed between Billboard1 and Billboard2)
Billboard2=for gratis opdateringer.

SettingUpAssociations=Opsætter filtilknytninger...
SettingUpPyGimp=Opsætter miljø til GIMP Python-udvidelse...
SettingUpEnvironment=Opsætter GIMP-miljø...
SettingUpGimpRC=Opsætter GIMP-konfiguration til understøttelses af 32-bit plugin...

;displayed on last page
LaunchGimp=Start GIMP

;shown during uninstall when removing add-ons
UninstallingAddOnCaption=Fjerner add-on

InternalError=Intern fejl (%1).

;used by installer for add-ons (currently only help)
DirNotGimp=GIMP ser ikke ud til at være installeret i den angivne mappe. Fortsæt alligevel?
