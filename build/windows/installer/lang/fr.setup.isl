[Messages]
;InfoBefore page is used instead of license page because the GPL only concerns distribution, not use, and as such doesn't have to be accepted
WizardInfoBefore=Contrat de licence utilisateur final
AboutSetupNote=Installateur réalisé par Jernej Simonèiè, jernej-gimp@ena.si%n%nImage d'accueil de l'installateur par Alexia_Death%nImage de fin de l'installateur par Jakub Steiner
WinVersionTooLowError=Cette version de GIMP requiert Windows XP Service Pack 3, ou supérieur.

[CustomMessages]
;shown before the wizard starts on development versions of GIMP
DevelopmentWarningTitle=Version de développement
;DevelopmentWarning=This is a development version of GIMP. As such, some features aren't finished, and it may be unstable. If you encounter any problems, first verify that they haven't already been fixed in GIT before you contact the developers.%nThis version of GIMP is not intended for day-to-day work, as it may be unstable, and you could lose your work. Do you wish to continue with installation anyway?
DevelopmentWarning=Ceci est une version de développement de l'installateur GIMP. Elle a moins été testée que l'installateur stable, ce qui peut causer des dysfonctionnements de GIMP. Veuillez rapporter les problèmes rencontrés dans le bugzilla GIMP (composant: "Installer"):%n_https://bugzilla.gnome.org/enter_bug.cgi?product=GIMP%n%nSouhaitez-vous tout de même poursuivre l'installation ?
DevelopmentButtonContinue=&Continuer
DevelopmentButtonExit=Quitter

;XPSP3Recommended=Warning: you are running an unsupported version of Windows. Please update to at least Windows XP with Service Pack 3 before reporting any problems.
SSERequired=Cette version de GIMP requiert un processeur prenant en charger les instructions SSE.

Require32BPPTitle=Problème de paramètres d'affichage
Require32BPP=L'installateur a détecté que Windows ne s'exécute pas en affichage 32 bits par pixel. C'est une cause connue d'instabilité de GIMP, nous vous recommandons de changer la profondeur d'affichage de couleurs en 32BPP avant de poursuivre.
Require32BPPContinue=&Continuer
Require32BPPExit=&Quitter

InstallOrCustomize=GIMP est prêt à être installé. Cliquez sur le bouton « Installer » pour utiliser les paramètres par défaut, ou sur « Personnaliser » pour choisir plus finement ce qui sera installé.
Install=&Installer
Customize=&Personnaliser

;setup types
TypeCompact=Installation compacte
TypeCustom=Installation personnalisée
TypeFull=Installation complète

;text above component description
ComponentsDescription=Description
;components
ComponentsGimp=GIMP
ComponentsGimpDescription=GIMP et tous les greffons par défaut
ComponentsDeps=Bibliothèques d'exécution
ComponentsDepsDescription=Bibliothèques d'exécution utilisées par GIMP, y compris l'environnement d'exécution GTK+
ComponentsGtkWimp=Moteur GTK+ pour Windows
ComponentsGtkWimpDescription=Apparence native pour Windows
ComponentsCompat=Prise en charge des anciens greffons
ComponentsCompatDescription=Installe les bibliothèques requises par d'anciens greffons
ComponentsTranslations=Traductions
ComponentsTranslationsDescription=Traductions
ComponentsPython=Prise en charge des scripts Python
ComponentsPythonDescription=Prise en charge des greffons GIMP écrits en langage Python
ComponentsGhostscript=Prise en charge de PostScript
ComponentsGhostscriptDescription=Permet le chargement de fichiers PostScript dans GIMP
;only when installing on x64 Windows
ComponentsGimp32=Gestion des greffons 32 bits
ComponentsGimp32Description=Inclut les fichiers nécessaires à l'utilisation de greffons 32 bits.%nRequis pour la prise en charge de Python.

;additional installation tasks
AdditionalIcons=Icônes additionnelles:
AdditionalIconsDesktop=Créer une icône sur le &bureau
AdditionalIconsQuickLaunch=Créer une icône dans la barre de lancement &rapide

RemoveOldGIMP=Supprimer les versions antérieures de GIMP

;%1 is replaced by file name; these messages should never appear (unless user runs out of disk space at the exact right moment)
ErrorChangingEnviron=Une erreur s'est produite lors de la mise à jour de l'environnement de GIMP dans %1. Si des erreurs surviennent au chargement des greffons, tentez de désinstaller puis réinstaller GIMP.
ErrorExtractingTemp=Erreur durant l'extraction de données temporaires.
ErrorUpdatingPython=Erreur durant la mise à jour de l'interpréteur Python.
ErrorReadingGimpRC=Erreur de mise à jour du fichier %1.
ErrorUpdatingGimpRC=Erreur de mise à jour du fichier %1 de configuration de GIMP.

;displayed in Explorer's right-click menu
OpenWithGimp=Modifier avec GIMP

;file associations page
SelectAssociationsCaption=Sélectionner les extensions à associer
SelectAssociationsExtensions=Extensions:
SelectAssociationsInfo1=Sélectionner les extensions de fichiers à associer à GIMP
SelectAssociationsInfo2=En double-cliquant dans l'Explorateur Windows, les fichiers portant ces extensions s'ouvriront dans GIMP.
SelectAssociationsSelectAll=&Sélectionner toutes
SelectAssociationsUnselectAll=&Désélectionner toutes
SelectAssociationsSelectUnused=Sélectionner les &inutilisées

;shown on summary screen just before starting the install
ReadyMemoAssociations=Types de fichiers à associer à GIMP:

RemovingOldVersion=Désinstallation de la version précédente de GIMP:
;%1 = version, %2 = installation directory
;ran uninstaller, but it returned an error, or didn't remove everything
RemovingOldVersionFailed=La désinstallation automatique de votre version de GIMP actuelle a échoué, et GIMP %1 ne peut l'écraser.%n%nVeuillez désinstaller manuellement l'ancienne version de GIMP et relancez l'installation dans %2, ou choisissez l'option d'installation personnalisée et un dossier de destination différent.%n%nL'installateur va à présent s'arrêter.
;couldn't find an uninstaller, or found several uninstallers
RemovingOldVersionCantUninstall=La méthode de désinstallation automatique de votre version de GIMP actuelle n'a pu être déterminée, et GIMP %1 ne peut écraser votre version de GIMP actuelle.%n%nVeuillez désinstaller manuellement l'ancienne version de GIMP et ses greffons avant de retenter une instalation dans %2, ou choisissez une installation personnalisée et un dossier de destination différent.%n%nL'installateur va à présent s'arrêter.

RebootRequiredFirst=Votre version précédente de GIMP a été supprimée avec succès, mais Windows requiert un redémarrage avant de poursuivre l'installation.%n%nAprès le redémarrage, l'installation reprendra à la connexion d'un administrateur.

;displayed if restart settings couldn't be read, or if the setup couldn't re-run itself
ErrorRestartingSetup=L'installateur a rencontré une erreur au redémarrage. (%1)

;displayed while the files are being extracted; note the capitalisation!
Billboard1=GIMP est un Logiciel Libre.%n%nVisitez
;www.gimp.org (displayed between Billboard1 and Billboard2)
Billboard2=pour des mises à jour gratuites.

SettingUpAssociations=Associations des extensions de fichiers...
SettingUpPyGimp=Configuration de l'environnement d'extension de GIMP en Python...
SettingUpEnvironment=Configuration de l'environnement GIMP...
SettingUpGimpRC=Configuration de la gestion des greffons 32 bits...

;displayed on last page
LaunchGimp=Exécuter GIMP

;shown during uninstall when removing add-ons
UninstallingAddOnCaption=Suppression de l'extension

InternalError=Erreur interne (%1).

;used by installer for add-ons (currently only help)
DirNotGimp=GIMP ne semble pas être installé dans le dossier sélectionné. Souhaitez vous continuer ?
