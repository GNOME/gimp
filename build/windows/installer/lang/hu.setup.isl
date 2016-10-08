[Messages]
;InfoBefore page is used instead of license page because the GPL only concerns distribution, not use, and as such doesn't have to be accepted
WizardInfoBefore=Licencmegállapodás
AboutSetupNote=A telepítõt Jernej Simoncic, jernej-gimp@ena.si készítette%n%nA telepítõ kezdõlapján látható képet Alexia_Death készítette%nA telepítõ utolsó lapján látható képet Jakub Steiner készítette
WinVersionTooLowError=A GIMP ezen verziója a Windows Windows XP Service Pack 3 vagy újabb verzióját igényli.

[CustomMessages]
;shown before the wizard starts on development versions of GIMP
DevelopmentWarningTitle=Fejlesztõi verzió
;DevelopmentWarning=Ez a GIMP fejlesztõi verziója. Mint ilyen, egyes funkciók nincsenek befejezve; továbbá a program instabil is lehet. Ha problémát tapasztal, ellenõrizze hogy nincs-e már javítva Git-ben, mielõtt megkeresi a fejlesztõket.%nA GIMP ezen verzióját nem napi szintû használatra szánjuk, mivel instabil lehet és emiatt elveszíthet adatokat. Mindenképp folytatja a telepítést?
DevelopmentWarning=Ez a GIMP telepítõjének fejlesztõi verziója. Nincs annyira tesztelve, mint a stabil telepítõ, emiatt a GIMP hibásan mûködhet. A tapasztalt hibákat a GIMP Bugzillába jelentse (az Installer összetevõ alá):%n_https://bugzilla.gnome.org/enter_bug.cgi?product=GIMP%n%nMindenképp folytatja a telepítést?
DevelopmentButtonContinue=&Folytatás
DevelopmentButtonExit=Kilépés

;XPSP3Recommended=Figyelem: a Windows nem támogatott verzióját futtatja. Frissítsen legalább a Windows XP Service Pack 3 kiadásra a problémák bejelentése elõtt.
SSERequired=A GIMP ezen verziója az SSE utasításokat támogató processzort igényel.

Require32BPPTitle=Probléma a kijelzõbeállításokkal
Require32BPP=A telepítõ azt észlelte, hogy a Windows nem 32 bites színmélységû módban fut. Ez a GIMP-nek stabilitási problémákat okoz, így javasoljuk, hogy a folytatás elõtt állítsa a színmélységet 32 bitesre.
Require32BPPContinue=&Folytatás
Require32BPPExit=&Kilépés

InstallOrCustomize=A GIMP immár kész a telepítésre. Kattintson a Telepítés gombra az alapértelmezett beállításokkal való telepítéshez, vagy a Személyre szabás gombra, ha módosítani szeretné a telepítendõ összetevõk listáját.
Install=&Telepítés
Customize=&Személyre szabás

;setup types
TypeCompact=Kompakt telepítés
TypeCustom=Egyéni telepítés
TypeFull=Teljes telepítés

;text above component description
ComponentsDescription=Leírás
;components
ComponentsGimp=GIMP
ComponentsGimpDescription=A GIMP és minden alap bõvítménye
ComponentsDeps=Futásidejû programkönyvtárak
ComponentsDepsDescription=A GIMP által használt futásidejû programkönyvtárak, beleértve a GTK+ környezetet
ComponentsGtkWimp=MS-Windows motor a GTK+-hoz
ComponentsGtkWimpDescription=Natív Windows megjelenés a GIMP-hez
ComponentsCompat=Régi bõvítmények támogatása
ComponentsCompatDescription=Régi külsõ bõvítményekhez szükséges programkönyvtárak telepítése
ComponentsTranslations=Fordítások
ComponentsTranslationsDescription=Fordítások
ComponentsPython=Python parancsfájlok
ComponentsPythonDescription=Lehetõvé teszi Python nyelven írt GIMP bõvítmények használatát.
ComponentsGhostscript=PostScript támogatás
ComponentsGhostscriptDescription=A GIMP betöltheti a PostScript fájlokat
;only when installing on x64 Windows
ComponentsGimp32=32 bites bõvítmények támogatása
ComponentsGimp32Description=A 32 bites bõvítmények támogatásához szükséges fájlok.%nSzükséges a Python támogatáshoz.

;additional installation tasks
AdditionalIcons=További ikonok:
AdditionalIconsDesktop=&Asztali ikon létrehozása
AdditionalIconsQuickLaunch=&Gyorsindító ikon létrehozása

RemoveOldGIMP=Korábbi GIMP verzió eltávolítása

;%1 is replaced by file name; these messages should never appear (unless user runs out of disk space at the exact right moment)
ErrorChangingEnviron=Hiba történt a GIMP környezetének frissítésekor ebben: %1. Ha hibaüzeneteket kap a bõvítmények betöltésekor, akkor próbálja meg eltávolítani és újratelepíteni a GIMP-et.
ErrorExtractingTemp=Hiba az ideiglenes adatok kibontásakor.
ErrorUpdatingPython=Hiba a Python értelmezõ információinak frissítésekor.
ErrorReadingGimpRC=Hiba történt a következõ frissítésekor: %1.
ErrorUpdatingGimpRC=Hiba történt a GIMP beállítófájljának frissítésekor: %1.

;displayed in Explorer's right-click menu
OpenWithGimp=Szerkesztés a GIMP-pel

;file associations page
SelectAssociationsCaption=Válasszon fájltársításokat
SelectAssociationsExtensions=Kiterjesztések:
SelectAssociationsInfo1=Válassza ki a GIMP-hez társítandó fájltípusokat
SelectAssociationsInfo2=Ennek hatására a kijelölt típusú fájlok a GIMP-ben nyílnak meg, amikor duplán kattint rájuk az Intézõben.
SelectAssociationsSelectAll=Összes &kijelölése
SelectAssociationsUnselectAll=Kijelölés &törlése
SelectAssociationsSelectUnused=Tö&bbi kijelölése

;shown on summary screen just before starting the install
ReadyMemoAssociations=A GIMP-hez társítandó fájltípusok:

RemovingOldVersion=A GIMP korábbi verziójának eltávolítása:
;%1 = version, %2 = installation directory
;ran uninstaller, but it returned an error, or didn't remove everything
RemovingOldVersionFailed=A GIMP %1 nem telepíthetõ a jelenlegi GIMP verzió fölé, és a régi verzió automatikus eltávolítása meghiúsult.%n%nTávolítsa el a GIMP korábbi verzióját, mielõtt ezt a verziót ide telepíti: %2, vagy válassza az Egyéni telepítést és válasszon másik telepítési mappát.%n%nA telepítõ most kilép.
;couldn't find an uninstaller, or found several uninstallers
RemovingOldVersionCantUninstall=A GIMP %1 nem telepíthetõ a jelenlegi GIMP verzió fölé, és a telepítõ nem tudta megállapítani, hogyan távolítható el a régi verzió automatikusan.%n%nTávolítsa el a GIMP korábbi verzióját és a bõvítményeket, mielõtt ezt a verziót ide telepíti: %2, vagy válassza az Egyéni telepítést és válasszon másik telepítési mappát.%n%nA telepítõ most kilép.

RebootRequiredFirst=A GIMP korábbi verziója sikeresen eltávolítva, de a Windowst újra kell indítani, mielõtt a telepítés folytatódhatna.%n%nA számítógép újraindítása és egy adminisztrátor bejelentkezése után a telepítõ futása folytatódik.

;displayed if restart settings couldn't be read, or if the setup couldn't re-run itself
ErrorRestartingSetup=Hiba történt a Telepítõ újraindításakor. (%1)

;displayed while the files are being extracted; note the capitalisation!
Billboard1=Ne feledje: A GIMP szabad szoftver.%n%nFrissítésekért keresse fel a
;www.gimp.org (displayed between Billboard1 and Billboard2)
Billboard2=oldalt.

SettingUpAssociations=Fájltársítások beállítása...
SettingUpPyGimp=Környezet beállítása a GIMP Python kiterjesztéséhez...
SettingUpEnvironment=A GIMP környezetének beállítása...
SettingUpGimpRC=A GIMP beállítása a 32 bites bõvítmények támogatásához...

;displayed on last page
LaunchGimp=A GIMP indítása

;shown during uninstall when removing add-ons
UninstallingAddOnCaption=Bõvítmény eltávolítása

InternalError=Belsõ hiba (%1).

;used by installer for add-ons (currently only help)
DirNotGimp=A GIMP nem található a kijelölt könyvtárban. Mindenképp folytatja?
