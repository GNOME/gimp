[Messages]
;InfoBefore page is used instead of license page because the GPL only concerns distribution, not use, and as such doesn't have to be accepted
WizardInfoBefore=Umowa licencyjna
AboutSetupNote=Instalator utworzony przez Jerneja Simonèièa, jernej-gimp@ena.si%n%nObraz na stronie otwieraj¹cej autorstwa Alexia_Death%nObraz na stronie zamykaj¹cej autorstwa Jakuba Steinera
WinVersionTooLowError=Ta wersja programu GIMP wymaga systemu Windows XP z Service Pack 3 lub nowsz¹ wersjê systemu Windows.

[CustomMessages]
;shown before the wizard starts on development versions of GIMP
DevelopmentWarningTitle=Wersja rozwojowa
;DevelopmentWarning=To jest rozwojowa wersja programu GIMP. Niektóre funkcje nie zosta³y jeszcze ukoñczone, a ca³y program mo¿e byæ niestabilny. W razie wyst¹pienia b³êdu przed skontaktowaniem siê z programistami nale¿y sprawdziæ, czy nie zosta³ on naprawiony w repozytorium git.%nTa wersja programu GIMP nie jest przeznaczona do codziennej pracy z powodu niestabilnoœci i mo¿liwoœci utraty danych. Kontynuowaæ instalacjê mimo to?
DevelopmentWarning=To jest rozwojowa wersja instalatora programu GIMP. Nie zosta³a ona przetestowana tak dok³adnie, jak stabilna wersja, co mo¿e powodowaæ nieprawid³owe dzia³anie programu GIMP. Prosimy zg³aszaæ napotkane b³êdy w systemie Bugzilla programu GIMP (komponent \"Installer\"):%n_https://bugzilla.gnome.org/enter_bug.cgi?product=GIMP%n%nKontynuowaæ instalacjê mimo to?
DevelopmentButtonContinue=&Kontynuuj
DevelopmentButtonExit=Zakoñcz

;XPSP3Recommended=Ostrze¿enie: na komputerze uruchomiona jest nieobs³ugiwana wersja systemu Windows. Prosimy zaktualizowaæ do co najmniej systemu Windows XP z Service Pack 3 przed zg³aszaniem problemów.
SSERequired=Ta wersja programu GIMP wymaga procesora obs³uguj¹cego instrukcje SSE.

Require32BPPTitle=Problem ustawieñ ekranu
Require32BPP=Instalator wykry³, ¿e system Windows nie dzia³a w trybie wyœwietlania 32 bitów g³êbi kolorów. Powoduje to problemy stabilnoœci programu GIMP. Zalecana jest zmiana g³êbi kolorów ekranu na 32 bity na piksel przed kontynuowaniem.
Require32BPPContinue=&Kontynuuj
Require32BPPExit=Za&koñcz

InstallOrCustomize=Program GIMP jest gotowy do instalacji. Klikniêcie przycisku Zainstaluj spowoduje instalacjê u¿ywaj¹c domyœlnych ustawieñ, a klikniêcie przycisku Dostosuj udostêpnia wiêcej mo¿liwoœci kontroli nad procesem instalacji.
Install=Za&instaluj
Customize=&Dostosuj

;setup types
TypeCompact=Instalacja podstawowa
TypeCustom=Instalacja u¿ytkownika
TypeFull=Pe³na instalacja

;text above component description
ComponentsDescription=Opis
;components
ComponentsGimp=GIMP
ComponentsGimpDescription=GIMP i wszystkie domyœlne wtyczki
ComponentsDeps=Biblioteki wykonawcze
ComponentsDepsDescription=Biblioteki wykonawcze u¿ywane przez program GIMP, w tym œrodowisko wykonawcze GTK+
ComponentsGtkWimp=Mechanizm MS-Windows dla biblioteki GTK+
ComponentsGtkWimpDescription=Natywny wygl¹d programu GIMP
ComponentsCompat=Obs³uga starszych wtyczek
ComponentsCompatDescription=Instaluje biblioteki wymagane przez starsze wtyczki firm trzecich
ComponentsTranslations=T³umaczenia
ComponentsTranslationsDescription=T³umaczenia
ComponentsPython=Jêzyk skryptowy Python
ComponentsPythonDescription=Umo¿liwia u¿ywanie wtyczek programu GIMP napisanych w jêzyku skryptowym Python.
ComponentsGhostscript=Obs³uga plików PostScript
ComponentsGhostscriptDescription=Umo¿liwia programowi GIMP wczytywanie plików w formacie PostScript
;only when installing on x64 Windows
ComponentsGimp32=Obs³uga wtyczek 32-bitowych
ComponentsGimp32Description=Do³¹cza pliki wymagane, aby u¿ywaæ wtyczki 32-bitowe.%nWymagane do obs³ugi jêzyka Python.

;additional installation tasks
AdditionalIcons=Dodatkowe ikony:
AdditionalIconsDesktop=&Utworzenie ikony na pulpicie
AdditionalIconsQuickLaunch=Utworzenie ikony na pasku &szybkiego uruchamiania

RemoveOldGIMP=Usuniêcie poprzedniej wersji programu GIMP

;%1 is replaced by file name; these messages should never appear (unless user runs out of disk space at the exact right moment)
ErrorChangingEnviron=Wyst¹pi³ problem podczas aktualizowania œrodowiska programu GIMP w %1. Jeœli wyst¹pi¹ b³êdy podczas wczytywania wtyczek, to nale¿y odinstalowaæ i ponownie zainstalowaæ program GIMP.
ErrorExtractingTemp=B³¹d podczas wypakowywania plików tymczasowych.
ErrorUpdatingPython=B³¹d podczas aktualizowania informacji o interpreterze jêzyka Python.
ErrorReadingGimpRC=Wyst¹pi³ b³¹d podczas aktualizowania %1.
ErrorUpdatingGimpRC=Wyst¹pi³ b³¹d podczas aktualizowania pliku konfiguracji %1 programu GIMP.

;displayed in Explorer's right-click menu
OpenWithGimp=Edytuj za pomoc¹ GIMP

;file associations page
SelectAssociationsCaption=Wybór powi¹zañ plików
SelectAssociationsExtensions=Rozszerzenia:
SelectAssociationsInfo1=Proszê wybraæ typy plików do powi¹zania z programem GIMP
SelectAssociationsInfo2=Spowoduje to otwieranie wybranych plików w programie GIMP po podwójnym klikniêciu ich w Eksploratorze.
SelectAssociationsSelectAll=Zaznacz &wszystkie
SelectAssociationsUnselectAll=Odznacz w&szystkie
SelectAssociationsSelectUnused=Zaznacz &nieu¿ywane

;shown on summary screen just before starting the install
ReadyMemoAssociations=Typy plików do powi¹zania z programem GIMP:

RemovingOldVersion=Usuwanie poprzedniej wersji programu GIMP:
;%1 = version, %2 = installation directory
;ran uninstaller, but it returned an error, or didn't remove everything
RemovingOldVersionFailed=GIMP %1 nie mo¿e zostaæ zainstalowany w miejscu obecnie zainstalowanej wersji, a automatyczne odinstalowanie poprzedniej wersji siê nie powiod³o.%n%nProszê samodzielnie usun¹æ poprzedni¹ wersjê programu GIMP przed zainstalowaniem tej wersji w katalogu %2 lub wybraæ instalacjê u¿ytkownika i podaæ inny katalog instalacji.%n%nInstalator zostanie zakoñczony.
;couldn't find an uninstaller, or found several uninstallers
RemovingOldVersionCantUninstall=GIMP %1 nie mo¿e zostaæ zainstalowany w miejscu obecnie zainstalowanej wersji, a instalator nie mo¿e ustaliæ sposobu automatycznego usuniêcia poprzedniej wersji.%n%nPrzed zainstalowaniem tej wersji w katalogu %2 proszê usun¹æ poprzedni¹ wersjê programu GIMP i wszystkie dodatki zainstalowane przez u¿ytkownika lub wybraæ instalacjê u¿ytkownika i podaæ inny katalog instalacji.%n%nInstalator zostanie zakoñczony.

RebootRequiredFirst=Pomyœlnie usuniêto poprzedni¹ wersjê programu GIMP, ale system Windows musi zostaæ ponownie uruchomiony przed kontynuowaniem instalacji.%n%nPo ponownym uruchomieniu komputera instalator zostanie kontynuowany, kiedy administrator siê zaloguje.

;displayed if restart settings couldn't be read, or if the setup couldn't re-run itself
ErrorRestartingSetup=Wyst¹pi³ b³¹d podczas ponownego uruchamiania instalatora. (%1)

;displayed while the files are being extracted; note the capitalisation!
Billboard1=Prosimy pamiêtaæ: program GIMP jest wolnym oprogramowaniem.%n%nZapraszamy do odwiedzenia witryny
;www.gimp.org (displayed between Billboard1 and Billboard2)
Billboard2=zawieraj¹cej darmowe aktualizacje.

SettingUpAssociations=Ustawianie powi¹zañ plików...
SettingUpPyGimp=Ustawianie œrodowiska dla rozszerzenia jêzyka Python programu GIMP...
SettingUpEnvironment=Ustawianie œrodowiska programu GIMP...
SettingUpGimpRC=Ustawianie konfiguracji programu GIMP dla obs³ugi wtyczek 32-bitowych...

;displayed on last page
LaunchGimp=Uruchomienie programu GIMP

;shown during uninstall when removing add-ons
UninstallingAddOnCaption=Usuwanie dodatku

InternalError=Wewnêtrzny b³¹d (%1).

;used by installer for add-ons (currently only help)
DirNotGimp=Program GIMP nie jest zainstalowany w wybranym katalogu. Kontynuowaæ mimo to?
