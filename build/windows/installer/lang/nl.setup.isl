[Messages]
;InfoBefore page is used instead of license page because the GPL only concerns distribution, not use, and as such doesn't have to be accepted
WizardInfoBefore=Licentieovereenkomst
AboutSetupNote=Installatieprogramma gecreëerd door Jernej Simonèiè, jernej-gimp@ena.si%n%nAfbeelding op startpagina van het installatieprogramma door Alexia_Death%nAfbeelding op eindpagina van het instalatieprogramma door Jakub Steiner
WinVersionTooLowError=Deze versie van GIMP vereist Windows XP met Service Pack 3, of een recentere versie van Windows.

[CustomMessages]
;shown before the wizard starts on development versions of GIMP
DevelopmentWarningTitle=Ontwikkelaarsversie
;DevelopmentWarning=Dit is een ontwikkelingsversie van GIMP. Zodanig zijn sommige functies nog niet klaar en kan het onstabiel zijn. Als je problemen tegenkomt, controleer eerst of het nog niet is gerepareerd in GIT voordat u contact opneemt met de ontwikkelaars.%nDeze versie van GIMP is niet bedoeld voor dagelijks werk, omdat het onstabiel kan zijn en u zou u werk kunnen verliezen. Wenst u alsnog verder te gaan met de installatie?
DevelopmentWarning=Dit is een ontwikkelingsversie van GIMP installatie. Het is nog niet zoveel getest als de stabiele installatie, dit kan resulteren in GIMP niet optimaal werken. Gelieve problemen die je ondervindt te rapporteren in de GIMP bugzilla (Installatiecomponent):%n_https://bugzilla.gnome.org/enter_bug.cgi?product=GIMP%n%nWenst u alsnog verder te gaan met de installatie?
DevelopmentButtonContinue=&Verdergaan
DevelopmentButtonExit=Sluiten

;XPSP3Recommended=Waarschuwing: u gebruikt een niet ondersteunde versie van Windows. Gelieve deze bij te werken tot minstens Windows XP met Service Pack 3 voor het rapporteren van problemen.
SSERequired=Deze versie van GIMP vereist een processor die SSE instructies ondersteunt.

Require32BPPTitle=Probleem beeldscherminstellingen
Require32BPP=Het installatieprogramma heeft gedetecteerd dat u Windows momenteel niet werkt in 32-bit kleurdiepte beeldmodus. Dit is geweten om stabiliteitsproblemen met GIMP te veroorzaken, dus het is aangeraden om de beeldscherm kleurdiepte te veranderen naar 32-bit voordat u verder gaat.
Require32BPPContinue=&Verdergaan
Require32BPPExit=&Sluiten

InstallOrCustomize=GIMP is nu klaar voor installatie. Klik op de Installeer knop voor de standaard instellingen, of klik de Aanpassen knop om meer controle te hebben over wat er wordt geïnstalleerd.
Install=&Installeer
Customize=&Aanpassen

;setup types
TypeCompact=Eenvoudige installatie
TypeCustom=Aangepaste installatie
TypeFull=Volledige installatie

;text above component description
ComponentsDescription=Beschrijving
;components
ComponentsGimp=GIMP
ComponentsGimpDescription=GIMP en alle standaard plugins
ComponentsDeps=Run-time bibliotheken
ComponentsDepsDescription=Run-time bibliotheken gebruikt door GIMP, inclusief GTK+ Run-time Environment
ComponentsGtkWimp=Windows engine voor GTK+
ComponentsGtkWimpDescription=Natieve Windows uiterlijk voor GIMP
ComponentsCompat=Ondersteuning voor oude plugins
ComponentsCompatDescription=Installeer bibliotheken nodig door oude plugins van derden
ComponentsTranslations=Vertalingen
ComponentsTranslationsDescription=Vertalingen
ComponentsPython=Python scripting
ComponentsPythonDescription=Staat toe van GIMP plugins geschreven in Python scripting taal te gebruiken.
ComponentsGhostscript=PostScript ondersteuning
ComponentsGhostscriptDescription=Staat GIMP toe PostScript bestanden te laden
;only when installing on x64 Windows
ComponentsGimp32=Ondersteuning voor 32-bit plugins
ComponentsGimp32Description=Installeer bestanden nodig voor gebruik van 32-bit plugins.%nVereist voor Python ondersteuning.

;additional installation tasks
AdditionalIcons=Extra snelkoppelingen:
AdditionalIconsDesktop=Snelkoppeling op het &bureaublad aanmaken
AdditionalIconsQuickLaunch=Snelkoppeling in &Quicklaunch aanmaken

RemoveOldGIMP=Verwijder oudere GIMP versies

;%1 is replaced by file name; these messages should never appear (unless user runs out of disk space at the exact right moment)
ErrorChangingEnviron=Er was een probleem tijdens het bijwerken van GIMP's omgeving in %1. Als u een probleem krijgt bij het laden van plugins, probeer GIMP te verwijderen en opnieuw te installeren.
ErrorExtractingTemp=Er was een fout uitpakken tijdelijke gegevens.
ErrorUpdatingPython=Er was een fout bijwerken Python interpreter info.
ErrorReadingGimpRC=Er was een fout tijdens het bijwerken van %1.
ErrorUpdatingGimpRC=Er was een fout tijdens het bijwerken van GIMP's configuratie bestand %1.

;displayed in Explorer's right-click menu
OpenWithGimp=Bewerken met GIMP

;file associations page
SelectAssociationsCaption=Selecteer bestandskoppelingen
SelectAssociationsExtensions=Bestandsextensies:
SelectAssociationsInfo1=Selecteer de bestandsextensie die u wenst te associëren met GIMP
SelectAssociationsInfo2=Dit zal geselecteerde bestanden openen in GIMP wanneer u deze dubbelklikt in Verkenner.
SelectAssociationsSelectAll=&Alles selecteren
SelectAssociationsUnselectAll=&Alles deselecteren
SelectAssociationsSelectUnused=&Ongebruikte selecteren

;shown on summary screen just before starting the install
ReadyMemoAssociations=Bestandtypes te associeren met GIMP:

RemovingOldVersion=Removing previous version of GIMP:
;%1 = version, %2 = installation directory
;ran uninstaller, but it returned an error, or didn't remove everything
RemovingOldVersionFailed=GIMP %1 kan niet worden geïnstalleerd over u huidige geïnstalleerde GIMP versie en de automatische verwijdering van de oude versie is gefaald.%n%nGelieve zelf de vorige versie van GIMP te verwijderen voordat u deze installeerd in %2 of kies een Aangepaste installatie en selecteer een andere installatie map.%n%nHet installatieprogramma zal nu afsluiten.
;couldn't find an uninstaller, or found several uninstallers
RemovingOldVersionCantUninstall=GIMP %1 kan niet worden geïnstalleerd over u huidige geïnstalleerde GIMP versie en het installatieprogramma kon geen manier vinden om de oude versie automatisch te verwijderen.%n%nGelieve zelf de vorige versie van GIMP en plugins te verwijderen voordat u deze versie in %2 installeert of kies een Aangepaste installatie en selecteer een andere map.%n%nHet installatieprogramma zal nu afsluiten.

RebootRequiredFirst=De vorige versie van GIMP wer succesvol verwijderd, maar Windows moet heropstarten voordat de installatie kan verdergaan.%n%nNa het heropstarten van u computer zal de installatie verdergaan de volgende keer een administrator inlogt.

;displayed if restart settings couldn't be read, or if the setup couldn't re-run itself
ErrorRestartingSetup=Er was een fout bij het heropstarten van de installatie. (%1)

;displayed while the files are being extracted; note the capitalisation!
Billboard1=Remember: GIMP is Vrije Software.%n%nBezoek alstublieft
;www.gimp.org (displayed between Billboard1 and Billboard2)
Billboard2=voor gratis bijwerkingen.

SettingUpAssociations=Bezig met het opzetten van bestandskoppelingen...
SettingUpPyGimp=Bezig met het opzetten van omgeving voor GIMP Python extensie...
SettingUpEnvironment=Bezig met het opzetten van GIMP omgeving...
SettingUpGimpRC=Bezig met het opzetten van GIMP configuratie voor 32-bit plugin ondersteuning...

;displayed on last page
LaunchGimp=GIMP starten

;shown during uninstall when removing add-ons
UninstallingAddOnCaption=Verwijderen van add-on

InternalError=Interne fout (%1).

;used by installer for add-ons (currently only help)
DirNotGimp=GIMP schijnt niet in de geselecteerde map te worden geïnstalleerd. Toch doorgaan?