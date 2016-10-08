[Messages]
;InfoBefore page is used instead of license page because the GPL only concerns distribution, not use, and as such doesn't have to be accepted
WizardInfoBefore=Acord de llicència
AboutSetupNote=Instal·lació creada per Jernej Simonèiè, jernej-gimp@ena.si%n%nImatge en la pàgina d'inici de la instal·lació per Alexia_Death%nImatge en la pàgina final de la instal·lació per Jakub Steiner
WinVersionTooLowError=Aquesta versió del GIMP requereix Windows XP amb Service Pack 3, o una versió més nova de Windows.

[CustomMessages]
;shown before the wizard starts on development versions of GIMP
DevelopmentWarningTitle=Versió de desenvolupament
;DevelopmentWarning=Aquesta és una versió de desenvolupament del GIMP. Així, algunes característiques no estan acabades i pot ser inestable. Si trobeu qualsevol problema, verifiqueu primer que no ha estat resolt en el GIT abans de contactar amb els desenvolupadors.%nAquesta versió del GIMP no està orientada al treball diari , així pot ser inestable i podríeu perdre la vostra feina. Voleu continuar amb la instal·lació de totes maneres?
DevelopmentWarning=Aquesta és una versió de desenvolupament de l'instal·lador del GIMP. No ha estat provada tan com l'instal·lador estable, i això pot fer que el GIMP no funcioni correctament. Informeu de qualsevol problema que trobeu en el bugzilla del GIMP (Installer component):%n_https://bugzilla.gnome.org/enter_bug.cgi?product=GIMP%n%nVoleu continuar amb la instal·lació de totes maneres?
DevelopmentButtonContinue=&Continua
DevelopmentButtonExit=Surt

;XPSP3Recommended=Avís: Esteu executant una versió no suportada de Windows. Actualitzeu, com a mínim, a Windows XP amb Service Pack 3 abans d'informar de qualsevol problema.
SSERequired=Aquesta versió del GIMP requereix un processador que suporti instruccions SSE.

Require32BPPTitle=Hi ha un problema en la configuració de pantalla
Require32BPP=L'instal·lador ha detectat que el vostre Windows no s'està executant en mode de visualització de 32 bits per píxel. Això pot causar problemes d'estabilitat amb el GIMP, pel que es recomana canviar la profunditat de color de pantalla a 32 BPP abans de continuar.
Require32BPPContinue=&Continua
Require32BPPExit=S&urt

InstallOrCustomize=El GIMP ja està llest per ser instal·lat. Cliqueu en el botó Instal·la per instal·lar usant els paràmetres per defecte o cliqueu el botó Personalitza si us agrada un major control sobre el que voleu instal·lar.
Install=&Instal·la
Customize=&Personalitza

;setup types
TypeCompact=Instal·lació compacta
TypeCustom=Instal·lació personalitzada
TypeFull=Instal·lació completa

;text above component description
ComponentsDescription=Descripció
;components
ComponentsGimp=GIMP
ComponentsGimpDescription=GIMP i tots els connectors per defecte
ComponentsDeps=Biblioteques d'execució
ComponentsDepsDescription=Biblioteques d'execució usades pel GIMP, inclòs l'entorn d'execució GTK+
ComponentsGtkWimp=Motor MS-Windows per GTK+
ComponentsGtkWimpDescription=Aspecte natiu de Windows pel GIMP
ComponentsCompat=Suport per connectors vells
ComponentsCompatDescription=Instal·la les biblioteques necessàries pels connectors vells de terceres parts
ComponentsTranslations=Traduccions
ComponentsTranslationsDescription=Traduccions
ComponentsPython=Python scripting
ComponentsPythonDescription=Us permet usar els connectors del GIMP escrits en llenguatge script de Python.
ComponentsGhostscript=Suport PostScript
ComponentsGhostscriptDescription= Permet al GIMP carregar fitxers PostScript
;only when installing on x64 Windows
ComponentsGimp32=Suport per connectors de 32-bit
ComponentsGimp32Description=Inclou els fitxers necessaris per usar els connectors de 32-bits.%nNecessaris pel suport de Python.

;additional installation tasks
AdditionalIcons=Icones addicionals:
AdditionalIconsDesktop=Crea una icona en l'&escriptori
AdditionalIconsQuickLaunch=Crea una icona d'accés &ràpid

RemoveOldGIMP=Elimina la versió prèvia del GIMP.

;%1 is replaced by file name; these messages should never appear (unless user runs out of disk space at the exact right moment)
ErrorChangingEnviron=S'ha produït un error en actualitzar l'entorn del GIMP en %1. Si teniu qualsevol en carregar els connectors, proveu a desinstal·lar i tornar a instal·lar el GIMP.
ErrorExtractingTemp=S'ha produït un error en extreure les dades temporals.
ErrorUpdatingPython=S'ha produït un error en actualitzar la informació de l'intèrpret de Python.
ErrorReadingGimpRC=S'ha produït un error en actualitzar %1.
ErrorUpdatingGimpRC=S'ha produït un error en actualitzar el fitxer %1 de configuració del GIMP.

;displayed in Explorer's right-click menu
OpenWithGimp=Edita amb el GIMP

;file associations page
SelectAssociationsCaption=Seleccioneu les associacions dels fitxers
SelectAssociationsExtensions=Extensions:
SelectAssociationsInfo1=Seleccioneu els tipus de fitxers que voleu associar amb el GIMP
SelectAssociationsInfo2=Això farà que els tipus de fitxers seleccionats s'obrin amb el GIMP quan feu doble clic sobre ells en l'explorador.
SelectAssociationsSelectAll=Selecciona-ho &tot
SelectAssociationsUnselectAll=Desselecciona-ho t&ot
SelectAssociationsSelectUnused=Selecciona els no &usats

;shown on summary screen just before starting the install
ReadyMemoAssociations=Tipus de fitxer a associar amb el GIMP:

RemovingOldVersion=Removing previous version of GIMP:
;%1 = version, %2 = installation directory
;ran uninstaller, but it returned an error, or didn't remove everything
RemovingOldVersionFailed= El GIMP %1 no es pot instal·lar sobre la versió del GIMP instal·lada actualment, i la desinstal·lació automàtica de la versió antiga ha fallat.%n%nElimineu la versió prèvia del GIMP abans d'instal·lar aquesta versió en %2, o escolliu Instal·lació personalitzada, i seleccioneu una carpeta d'instal·lació diferent.%n%nL'instal·lador es tancarà ara.
;couldn't find an uninstaller, or found several uninstallers
RemovingOldVersionCantUninstall=El GIMP %1 no es pot instal·lar sobre la versió del GIMP instal·lada actualment, i l'instal·lador no pot determinar com eliminar automaticament.%n%nElimineu la versió prèvia del GIMP i qualsevol connector abans d'instal·lar aquesta versió en %2, o escolliu Instal·lació personalitzada, i seleccioneu una carpeta d'instal·lació diferent.%n%nL'instal·lador es tancarà ara.

RebootRequiredFirst=La versió prèvia del GIMP s'ha eliminat satisfactòriament, però cal reiniciar Windows abans que l'instal·lador pugui continuar.%n%nDesprés de reiniciar l'ordinador, l'instal·lador continuarà un cop un administrador obri sessió.

;displayed if restart settings couldn't be read, or if the setup couldn't re-run itself
ErrorRestartingSetup=S'ha produït un error en reiniciar l'insta·lador. (%1)

;displayed while the files are being extracted; note the capitalisation!
Billboard1=Recordeu: El GIMP és programari lliure.%n%nVisiteu
;www.gimp.org (displayed between Billboard1 and Billboard2)
Billboard2=per actualitzacions gratüites.

SettingUpAssociations=S'estan configurant les associacions de fitxer...
SettingUpPyGimp=S'està configurant l'entorn per l'extensió Python del GIMP...
SettingUpEnvironment=S'està configurant l'entorn del GIMP...
SettingUpGimpRC=S'està configurant el GIMP pel suport de connectors de 32-bits...

;displayed on last page
LaunchGimp=Inicia el GIMP

;shown during uninstall when removing add-ons
UninstallingAddOnCaption=S'estan eliminant complements

InternalError=S'ha produït un error intern (%1).

;used by installer for add-ons (currently only help)
DirNotGimp=El GIMP no sembla instal·lar-se en el directori seleccionat. Voleu continuar de totes maneres?
