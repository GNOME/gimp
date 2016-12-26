[Messages]
;InfoBefore page is used instead of license page because the GPL only concerns distribution, not use, and as such doesn't have to be accepted
WizardInfoBefore=Lizenzvereinbarung
AboutSetupNote=Setup erstellt von Jernej Simoncic, jernej-gimp@ena.si%n%nGrafik auf der Startseite der Installation von Alexia_Death%nGrafik auf der Abschlussseite der Installation von Jakub Steiner
WinVersionTooLowError=Diese Version von GIMP benötigt Windows XP Service Pack 3 oder jede neuere Version von Windows

[CustomMessages]
;shown before the wizard starts on development versions of GIMP
DevelopmentWarningTitle=Entwicklerversion
;DevelopmentWarning=Dies ist eine Entwicklerversion von GIMP. Diese kann instabil sein oder unvollendete Funktionen enthalten. Sollten Probleme auftreten, prüfen Sie bitte zunächst, ob diese bereits in GIT behoben wurden, bevor Sie die Entwickler kontaktieren.%nDiese Version von GIMP ist nicht für den tagtäglichen Einsatz bestimmt, weil sie abstürzen kann und Sie dadurch Daten verlieren werden. Wollen Sie die Installation dennoch fortsetzen?
DevelopmentWarning=Dies ist eine Entwicklerversion des GIMP-Installers. Er wurde nicht so intensiv wie der stabile Installer getestet, was dazu führen kann, dass GIMP nicht sauber arbeitet. Bitte melden Sie Probleme, auf die Sie stoßen im GIMP Bugzilla (Installationskomponente):%n_https://bugzilla.gnome.org/enter_bug.cgi?product=GIMP%n%nWollen Sie die Installation dennoch fortsetzen?
DevelopmentButtonContinue=&Weiter
DevelopmentButtonExit=&Abbrechen

;XPSP3Recommended=Achtung: Sie verwenden eine nicht unterstützte Version von Windows. Bitte aktualisieren Sie wenigstens auf Windows XP Service Pack 3 bevor Sie Probleme melden.
SSERequired=Diese Version von GIMP benötigt einen Prozessor, der über SSE-Erweiterungen verfügt.

Require32BPPTitle=Problem mit Grafikeinstellungen
Require32BPP=Die Installationsroutine hat festgestellt, dass Ihr Windows nicht derzeit nicht mit 32 Bit Farbtiefe läuft. Diese Einstellung ist bekannt dafür, Stabilitätsprobleme mit GIMP zu verursachen. Wir empfehlen deshalb, die Farbtiefe auf 32 Bit pro Pixel einzustellen, bevor Sie fortfahren.
Require32BPPContinue=&Weiter
Require32BPPExit=&Abbrechen

InstallOrCustomize=GIMP kann jetzt installiert werden. Klicken Sie auf Installieren, um mit den Standardeinstellungen zu installieren oder auf Anpassen, um festzulegen, welche Komponenten wo installiert werden.
Install=&Installieren
Customize=&Anpassen

;setup types
TypeCompact=Einfache Installation
TypeCustom=Benutzerdefinierte Installation
TypeFull=Komplette Installation

;text above component description
ComponentsDescription=Beschreibung
;components
ComponentsGimp=GIMP
ComponentsGimpDescription=GIMP und alle Standard-Plugins
ComponentsDeps=Laufzeitbibliotheken
ComponentsDepsDescription=Von GIMP benötigte Laufzeitbibliotheken inclusive der GTK+-Bibliothek
ComponentsGtkWimp=Windows-Engine für GTK+
ComponentsGtkWimpDescription=Natives Aussehen für GIMP
ComponentsCompat=Kompatibilitätsmodus
ComponentsCompatDescription=Bibliotheken, die von älteren Third-Party-Plug-Ins benötigt werden
ComponentsTranslations=Übersetzungen
ComponentsTranslationsDescription=Übersetzungen
ComponentsPython=Python Scriptumgebung
ComponentsPythonDescription=Erlaubt Ihnen, GIMP-Plug-Ins zu nutzen, die in der Scriptsprache Python geschrieben wurden.
ComponentsGhostscript=Postscript-Unterstützung
ComponentsGhostscriptDescription=ermöglicht es GIMP, Postscript- und PDF-dateien zu laden
;only when installing on x64 Windows
ComponentsGimp32=32-Bit-Unterstützung
ComponentsGimp32Description=Dateien installieren, die für die Nutzung von 32-Bit-Plug-Ins benötigt werden.%nFür Python-Unterstützung erforderlich.

;additional installation tasks
AdditionalIcons=Zusätzliche Verknüpfungen:
AdditionalIconsDesktop=&Desktop-Verknüpfung erstellen
AdditionalIconsQuickLaunch=&Quicklaunch-Verknüpfung erstellen

RemoveOldGIMP=Ältere GIMP-Versionen entfernen

;%1 is replaced by file name; these messages should never appear (unless user runs out of disk space at the exact right moment)
ErrorChangingEnviron=Es gab ein Problem bei der Aktualisierung von GIMPs Umgebung in %1. Sollten Fehler beim Laden von Plug-Ins auftauchen, probieren Sie, GIMP zu deinstallieren und neu zu installieren. 
ErrorExtractingTemp=Fehler beim Entpacken temporärer Dateien.
ErrorUpdatingPython=Fehler bei der Aktualisierung des Python-Interpreters.
ErrorReadingGimpRC=Bei der Aktualisierung von %1 trat ein Fehler auf.
ErrorUpdatingGimpRC=Bei der Aktualisierung der Konfigurationsdatei %1 trat ein Fehler auf.

;displayed in Explorer's right-click menu
OpenWithGimp=Mit GIMP öffnen

;file associations page
SelectAssociationsCaption=Dateizuordnungen auswählen
SelectAssociationsExtensions=Erweiterungen:
SelectAssociationsInfo1=Wählen Sie die Dateitypen, die Sie mit GIMP öffnen wollen
SelectAssociationsInfo2=Ausgewählte Dateitypen werden nach Doppelklick im Explorer automatisch mit GIMP geöffnet.
SelectAssociationsSelectAll=&Alle auswählen
SelectAssociationsUnselectAll=Auswahl auf&heben
SelectAssociationsSelectUnused=&Unbenutzte auswählen

;shown on summary screen just before starting the install
ReadyMemoAssociations=Dateizuordnungen für GIMP:

RemovingOldVersion=Entfernung von älteren GIMP-Installationen:
;%1 = version, %2 = installation directory
;ran uninstaller, but it returned an error, or didn't remove everything
RemovingOldVersionFailed=GIMP %1 kann nicht über eine ältere Version von GIMP installiert werden und die automatische Deinstallation schlug fehl.%n%nBitte entfernen Sie die vorhandene GIMP-Installation manuell bevor Sie diese Version nach %2 installieren, oder wählen Sie Benutzerdefinierte Installation und verwenden Sie einen anderen Installationsordner.%n%nDie Einrichtung wird nun beendet.
;couldn't find an uninstaller, or found several uninstallers
RemovingOldVersionCantUninstall=GIMP %1 kann nicht über die derzeit installierte Version von GIMP installiert werden und die Installationsroutine konnte die vorhandene Version nicht automatisch deinstallieren.%n%nBitte entfernen Sie die vorhandene GIMP-Installation manuell bevor Sie diese Version nach %2 installieren, oder wählen Sie Benutzerdefinierte Installation und verwenden Sie einen anderen Installationsordner.%n%nDie Einrichtung wird nun beendet.

RebootRequiredFirst=Die vorhandene GIMP-Version wurde erfolgreich entfernt, aber Windows muss neu gestartet werden, bevor die Installation fortgeführt werden kann.%n%nNach dem Neustart wird die Installation automatisch fortgesetzt, sobald sich ein Administrator einloggt. 

;displayed if restart settings couldn't be read, or if the setup couldn't re-run itself
ErrorRestartingSetup=Bei der Fortsetzung der Installation trat ein Fehler auf (%1).

;displayed while the files are being extracted; note the capitalisation!
Billboard1=Beachten Sie: GIMP ist Freie Software.%n%nBitte besuchen Sie
;www.gimp.org (displayed between Billboard1 and Billboard2)
Billboard2=für kostenlose Aktualisierungen.

SettingUpAssociations=Richte Dateizuordnungen ein...
SettingUpPyGimp=Richte Umgebung für die GIMP Python-Erweiterung ein...
SettingUpEnvironment=Richte Umgebung für GIMP ein...
SettingUpGimpRC=Richte GIMP-Einstellungen für 32-Bit-Plug-Ins ein...

;displayed on last page
LaunchGimp=GIMP jetzt starten

;shown during uninstall when removing add-ons
UninstallingAddOnCaption=Entferne Erweiterung

InternalError=Interner Fehler (%1).

;used by installer for add-ons (currently only help)
DirNotGimp=GIMP scheint nicht im ausgewählten Ordner installiert zu sein. Dennoch fortfahren?
