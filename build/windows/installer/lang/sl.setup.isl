[Messages]
WizardInfoBefore=Licenèna pogodba
AboutSetupNote=Namestitveni program je pripravil Jernej Simonèiè, jernej-gimp@ena.si%n%nSlika na prvi strani namestitvenega programa: Alexia_Death%nSlika na zadnji strani namestitvenega programa: Jakub Steiner
WinVersionTooLowError=Ta razlièica programa GIMP potrebuje Windows XP s servisnim paketom 3, ali novejšo razlièico programa Windows.

[CustomMessages]
DevelopmentWarningTitle=Razvojna razlièica
;DevelopmentWarning=To je razvojna razlièica programa GIMP, zaradi èesar nekateri deli programa morda niso dokonèani, poleg tega pa je program lahko tudi nestabilen. V primeru težav preverite, da le-te niso bile že odpravljene v GIT-u, preden stopite v stik z razvijalci.%nZaradi nestabilnosti ta razlièica ni namenjena za vsakodnevno delo, saj lahko kadarkoli izgubite svoje delo. Ali želite vseeno nadaljevati z namestitvijo?
DevelopmentWarning=To je razvojna razlièica namestitvenega programa za GIMP, ki še ni tako preizkušena kot obièajna razlièica. Èe naletite na kakršne koli težave pri namestitvi, jih prosim sporoèite v Bugzilli (komponenta Installer):%n_https://bugzilla.gnome.org/enter_bug.cgi?product=GIMP%n%nAli želite vseeno nadaljevati z namestitvijo?
DevelopmentButtonContinue=&Nadaljuj
DevelopmentButtonExit=Prekini

;XPSP3Recommended=Opozorilo: uporabljate nepodprto razlièico sistema Windows. Prosimo, namestite servisni paket 3 za Windows XP ali novejšo razlièico sistema Windows preden nas obvešèate o kakršnih koli težavah.
SSERequired=Ta razliŸica programa GIMP potrebuje procesor, ki ima podporo za SSE ukaze.

Require32BPPTitle=Težava z zaslonskimi nastavitvami
Require32BPP=Namestitveni program je zaznal, da Windows ne deluje v 32-bitnem barvnem naèinu. Takšne nastavitve lahko povzroèijo nestabilnost programa GIMP, zato je priporoèljivo da pred nadaljevanjem spremenite barvno globino zaslona na 32 bitov.
Require32BPPContinue=&Nadaljuj
Require32BPPExit=I&zhod

InstallOrCustomize=GIMP je pripravljen na namestitev. Kliknite gumb Namesti za namestitev s privzetimi nastavitvami, ali pa kliknite gumb Po meri, èe bi radi imeli veè nadzora nad možnostmi namestitve.
Install=Namest&i
Customize=&Po meri

TypeCompact=Minimalna namestitev
TypeCustom=Namestitev po meri
TypeFull=Polna namestitev

;components
ComponentsDescription=Opis
ComponentsGimp=GIMP
ComponentsGimpDescription=GIMP z vsemi privzetimi vtièniki
ComponentsDeps=Podporne knjižnice
ComponentsDepsDescription=Podporne knjižnice za GIMP, vkljuèno z okoljem GTK+
ComponentsGtkWimp=Tema MS-Windows za GTK+
ComponentsGtkWimpDescription=Windows izgled za GIMP
ComponentsCompat=Podpora za stare vtiènike
ComponentsCompatDescription=Podporne knjižnice za stare zunanje vtiènike za GIMP
ComponentsTranslations=Prevodi
ComponentsTranslationsDescription=Prevodi
ComponentsPython=Podpora za Python
ComponentsPythonDescription=Omogoèa izvajanje vtiènikov za GIMP, napisanih v programskem jeziku Python
ComponentsGhostscript=Podpora za PostScript
ComponentsGhostscriptDescription=Omogoèi nalaganje PostScript datotek
ComponentsGimp32=Podpora za 32-bitne vtiènike
ComponentsGimp32Description=Omogoèa uporabo 32-bitnih vtiènikov.%nPotrebno za uporabo podpore za Python

AdditionalIcons=Dodatne ikone:
AdditionalIconsDesktop=Ustvari ikono na n&amizju
AdditionalIconsQuickLaunch=Ustvari ikono v vrstici &hitri zagon

RemoveOldGIMP=Odstrani prejçnjo razliŸico programa GIMP

;%1 is replaced by file name; these messages should never appear (unless user runs out of disk space at the exact right moment)
ErrorChangingEnviron=Prišlo je do težav pri posodabljanju okolja za GIMP v datoteki %1. Èe se pri nalaganju vtiènikov pojavijo sporoèila o napakah, poizkusite odstraniti in ponovno namestiti GIMP.
ErrorExtractingTemp=Prišlo je do napake pri razširjanju zaèasnih datotek.
ErrorUpdatingPython=Prišlo je do napake pri nastavljanju podpore za Python.
ErrorReadingGimpRC=Prišlo je do napake pri branju datoteke %1.
ErrorUpdatingGimpRC=Prišlo je do napake pri pisanju nastavitev v datoteko %1.

;displayed in Explorer's right-click menu
OpenWithGimp=Uredi z GIMP-om

SelectAssociationsCaption=Povezovanje vrst datotek
SelectAssociationsExtensions=Pripone:
SelectAssociationsInfo1=Izberite vste datotek, ki bi jih radi odpirali z GIMP-om
SelectAssociationsInfo2=Tu lahko izberete vrste datotek, ki se bodo odprle v GIMP-u, ko jih dvokliknete v Raziskovalcu
SelectAssociationsSelectAll=Izber&i vse
SelectAssociationsUnselectAll=Poèist&i izbor
SelectAssociationsSelectUnused=Ne&uporabljene

ReadyMemoAssociations=Vrste datotek, ki jih bo odpiral GIMP:

RemovingOldVersion=Odstranjevanje starejših razlièic programa GIMP:
;%1 = version, %2 = installation directory
;ran uninstaller, but it returned an error, or didn't remove everything
RemovingOldVersionFailed=Te razlièice programa GIMP ne morete namestiti preko prejšnje razlièice, namestitvenemu programu pa prejšnje razlièice ni uspelo samodejno odstraniti.%n%nPred ponovnim namešèanjem te razlièice programa GIMP v mapo %2, odstranite prejšnjo razlièico, ali pa izberite namestitev po meri, in GIMP %1 namestite v drugo mapo.%n%nNamestitveni program se bo zdaj konèal.
;couldn't find an uninstaller, or found several uninstallers
RemovingOldVersionCantUninstall=Te razlièice programa GIMP ne morete namestiti preko prejšnje razlièice, namestitvenemu programu pa ni uspelo ugotoviti, kako prejšnjo razlièico odstraniti samodejno.%n%nPred ponovnim namešèanjem te razlièice programa GIMP v mapo %2, odstranite prejšnjo razlièico, ali pa izberite namestitev po meri, in GIMP %1 namestite v drugo mapo.%n%nNamestitveni program se bo zdaj konèal.

RebootRequiredFirst=Prejšnja razlièica programa GIMP je bila uspešno odstranjena, vendar je pred nadaljevanjem namestitve potrebno znova zagnati Windows.%n%nNamestitveni program bo dokonèal namestitev po ponovnem zagonu, ko se prviè prijavi administrator.

;displayed while the files are being extracted
Billboard1=GIMP spada med prosto programje.%n%nObišèite
;www.gimp.org (displayed between Billboard1 and Billboard2)
Billboard2=za brezplaène posodobitve.

;displayed if restart settings couldn't be read, or if the setup couldn't re-run itself
ErrorRestartingSetup=Prišlo je do napake pri ponovnem zagonu namestitvenega programa. (%1)

SettingUpAssociations=Nastavljam povezane vrste datotek...
SettingUpPyGimp=Pripravljam okolje za GIMP Python...
SettingUpEnvironment=Pripravljam okolje za GIMP...
SettingUpGimpRC=Pripravljam nastavitve za 32-bitne vtiènike...

;displayed on last page
LaunchGimp=Zaženi GIMP

;shown during uninstall when removing add-ons
UninstallingAddOnCaption=Odstranjujem dodatek

InternalError=Notranja napaka (%1).

;used by installer for add-ons (currently only help)
DirNotGimp=GIMP oèitno ni namešèen v izbrani mapi. Želite kljub temu nadaljevati?
