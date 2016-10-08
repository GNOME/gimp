[Messages]
;InfoBefore page is used instead of license page because the GPL only concerns distribution, not use, and as such doesn't have to be accepted
WizardInfoBefore=Accordo di licenza
AboutSetupNote=Installazione creata da Jernej Simonèiè, jernej-gimp@ena.si%n%nImmagine all'avvio dell'installazione di Alexia_Death%nImmagine alla fine dell'installazione di Jakub Steiner
WinVersionTooLowError=Questa versione di GIMP richiede Windows XP aggiornato al Service Pack 3, o una versione più recente di Windows.

[CustomMessages]
;shown before the wizard starts on development versions of GIMP
DevelopmentWarningTitle=Versione di sviluppo
;DevelopmentWarning=Questa è una versione di sviluppo di GIMP. Come tale, alcune funzioni non sono complete e potrebbero renderla instabile. Se si riscontrano dei problemi, verificare prima che questi non siano già stati risolti in GIT prima di contattare gli sviluppatori.%nQuesta versione di GIMP non è adatta ad un uso in produzione dato che, a causa della sua instabilità, potrebbe far perdere tutto il proprio lavoro. Continuare ugualmente l'installazione?
DevelopmentWarning=Questa è una versione di sviluppo dell'installatore di GIMP. Non è stata verificata come la versione stabile e ciò potrebbe rendere il funzionamento di GIMP instabile. Segnalare ogni eventuale problema riscontrato sul sito bugzilla di GIMP (Installer component):%n_https://bugzilla.gnome.org/enter_bug.cgi?product=GIMP%n%nContinuare ugualmente con l'installazione?
DevelopmentButtonContinue=&Continua
DevelopmentButtonExit=Esci

;XPSP3Recommended=Warning: you are running an unsupported version of Windows. Please update to at least Windows XP with Service Pack 3 before reporting any problems.
SSERequired=Questa versione di GIMP richiede un processore che supporti le istruzioni SSE.

Require32BPPTitle=Problema di impostazione dello schermo
Require32BPP=L'installatore ha rilevato che Windows attualmente non è in funzione in modalità schermo a 32 bits-per-pixel. È risaputo che ciò può causare problemi di instabilità in GIMP, perciò si raccomanda di cambiare la profondità di colore dello schermo a 32BPP prima di continuare.
Require32BPPContinue=&Continua
Require32BPPExit=E&sci

InstallOrCustomize=Ora GIMP è pronto per essere installato. Fare clic sul pulsante Installa per installarlo usando le impostazioni predefinite, o su Personalizza se si desidera un maggior livello di controllo sui parametri di installazione.
Install=&Installa
Customize=&Personalizza

;setup types
TypeCompact=Installazione compatta
TypeCustom=Installazione personalizzata
TypeFull=Installazione completa

;text above component description
ComponentsDescription=Descrizione
;components
ComponentsGimp=GIMP
ComponentsGimpDescription=GIMP e tutti i plugin predefiniti.
ComponentsDeps=Librerie a run-time
ComponentsDepsDescription=Librerie a run-time usate da GIMP, incluso l'ambiente run-time GTK+.
ComponentsGtkWimp=Motore GTK+ per MS-Windows
ComponentsGtkWimpDescription=Aspetto nativo Windows per GIMP.
ComponentsCompat=Supporto per i vecchi plugin
ComponentsCompatDescription=Installazione delle librerie necessarie per i vecchi plug-in di terze parti.
ComponentsTranslations=Traduzioni
ComponentsTranslationsDescription=Traduzioni.
ComponentsPython=Scripting Python
ComponentsPythonDescription=Consente di usare i plugin di GIMP scritti in liguaggio Python.
ComponentsGhostscript=Supporto PostScript
ComponentsGhostscriptDescription=Permette a GIMP di caricare file in formato PostScript.
;only when installing on x64 Windows
ComponentsGimp32=Supporto per i plugin a 32-bit
ComponentsGimp32Description=Include i file necessari per l'utilizzo di plugin a 32-bit.%nÈ richiesto per il supporto Python.

;additional installation tasks
AdditionalIcons=Icone aggiuntive:
AdditionalIconsDesktop=Crea un'icona sul &desktop
AdditionalIconsQuickLaunch=Crea un'icona di &avvio rapido

RemoveOldGIMP=Rimuove la versione precedente di GIMP

;%1 is replaced by file name; these messages should never appear (unless user runs out of disk space at the exact right moment)
ErrorChangingEnviron=Si è verificato un problema aggiornando l'ambiente di GIMP in %1. Se si verificano errori caricando i plugin, provare a disinstallare e reinstallare GIMP.
ErrorExtractingTemp=Errore durante l'estrazione dei dati temporanei.
ErrorUpdatingPython=Errore aggiornando i dati dell'interprete Python.
ErrorReadingGimpRC=Si è verificato un errore aggiornando %1.
ErrorUpdatingGimpRC=Si è verificato un errore aggiornando il file di configurazione di GIMP %1.

;displayed in Explorer's right-click menu
OpenWithGimp=Modifica con GIMP

;file associations page
SelectAssociationsCaption=Seleziona le associazioni di file
SelectAssociationsExtensions=Estensioni:
SelectAssociationsInfo1=Selezionare i tipi di file che si desidera associare a GIMP
SelectAssociationsInfo2=Ciò renderà possibile aprire automaticamente i file selezionati in GIMP quando si fa doppio clic in Explorer.
SelectAssociationsSelectAll=Seleziona &tutti
SelectAssociationsUnselectAll=Deseleziona tutt&i
SelectAssociationsSelectUnused=Seleziona i non &usati

;shown on summary screen just before starting the install
ReadyMemoAssociations=Tipi di file da associare a GIMP:

RemovingOldVersion=Rimozione della versione precedente di GIMP:
;%1 = version, %2 = installation directory
;ran uninstaller, but it returned an error, or didn't remove everything
RemovingOldVersionFailed=GIMP %1 non può essere installato sopra la versione di GIMP installata attualmente, e la funzione di disinstallazione automatica della vecchia versione ha fallito.%n%nRimuovere la versione precedente di GIMP manualmente prima di installare questa versione in %2, o scegliere l'installazione personalizzata selezionando una diversa cartella di installazione.%n%nL'installatore ora verrà chiuso.
;couldn't find an uninstaller, or found several uninstallers
RemovingOldVersionCantUninstall=GIMP %1 non può essere installato sopra la versione di GIMP installata attualmente, e l'installatore non riesce a determinare come rimuovere automaticamente la vecchia versione.%n%nRimuovere manualmente la versione precedente di GIMP e ogni elemento che sia stato aggiunto prima di installare questa versione in %2, o scegliere l'installazione personalizzata selezionando una diversa cartella di installazione.%n%nL'installatore ora verrà chiuso.

RebootRequiredFirst=La versione precedente di GIMP è stata rimossa con successo, ma Windows deve essere riavviato prima che l'installatore possa continuare.%n%nDopo il riavvio del computer, l'installatore continuerà non appena un amministratore entrerà nel sistema.

;displayed if restart settings couldn't be read, or if the setup couldn't re-run itself
ErrorRestartingSetup=Si è verificato un errore durante il riavvio dell'installatore. (%1)

;displayed while the files are being extracted; note the capitalisation!
Billboard1=Ricorda: GIMP è Software Libero.%n%nVisitare
;www.gimp.org (displayed between Billboard1 and Billboard2)
Billboard2=per aggiornarlo in libertà.

SettingUpAssociations=Impostaione delle associazioni di file...
SettingUpPyGimp=Impostazione dell'ambiente per l'estensione Python di GIMP...
SettingUpEnvironment=Impostazione dell'ambiente di GIMP...
SettingUpGimpRC=Impostazione del supporto ai plugin di GIMP a 32-bit...

;displayed on last page
LaunchGimp=Avvio di GIMP

;shown during uninstall when removing add-ons
UninstallingAddOnCaption=Rimozione aggiunte

InternalError=Errore interno (%1).

;used by installer for add-ons (currently only help)
DirNotGimp=GIMP non sembra essere installato nella directory selezionata. Continuare ugualmente?
