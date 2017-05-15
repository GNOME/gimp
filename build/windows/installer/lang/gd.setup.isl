[Messages]
;InfoBefore page is used instead of license page because the GPL only concerns distribution, not use, and as such doesn't have to be accepted
WizardInfoBefore=Aonta ceadachais
AboutSetupNote=Chaidh an t-inneal-stàlaidh a thogail le Jernej Simonèiè, jernej-gimp@ena.si%n%nAn dealbh air duilleag-fhosglaidh an inneil-stàlaidh le Alexia_Death%nAn dealbh air duilleag-dhùnaidh an inneil-stàlaidh le Jakub Steiner
WinVersionTooLowError=Feumaidh an tionndadh seo dhe GIMP Windows XP le Service Pack 3 no tionndadh dhe Windows nas ùire.

[CustomMessages]
;shown before the wizard starts on development versions of GIMP
DevelopmentWarningTitle=Tionndadh an luchd-leasachaidh
;DevelopmentWarning=This is a development version of GIMP. As such, some features aren't finished, and it may be unstable. If you encounter any problems, first verify that they haven't already been fixed in GIT before you contact the developers.%nThis version of GIMP is not intended for day-to-day work, as it may be unstable, and you could lose your work. Do you wish to continue with installation anyway?
DevelopmentWarning=Seo tionndadh luchd-leasachaidh dhen inneal-stàlaidh GIMP. Cha deach a chur fo dheuchainn cho teann ’s a chaidh an t-inneal-stàlaidh seasmhach agus dh’fhaoidte nach obraich GIMP mar bu chòir ri linn sin. Innis dhuinn mu dhuilgheadas sam bith air Bugzilla GIMP (Installer component):%n_https://bugzilla.gnome.org/enter_bug.cgi?product=GIMP%n%nTha duilgheadasan as aithne dhuinn aig an inneal-stàlaidh seo:%n- chan obraich luchdadh fhaidhlichean TIFF%n- cha dèid meud nam faidhlichean a shealltainn mar bu chòir%nNa innis dhuinn mu na duilgheadasan seo on a mhothaich sinn dhaibh mar-thà.%n%nA bheil thu airson leantainn air adhart leis an stàladh co-dhiù?
DevelopmentButtonContinue=&Lean air adhart
DevelopmentButtonExit=Fàg an-seo

;XPSP3Recommended=Warning: you are running an unsupported version of Windows. Please update to at least Windows XP with Service Pack 3 before reporting any problems.
SSERequired=Feumaidh an tionndadh seo dhe GIMP pròiseasar a chuireas tais ri stiùireadh SSE.

Require32BPPTitle=Duilgheadas le roghainnean an uidheim-thaisbeanaidh
Require32BPP=Mhothaich an t-inneal-stàlaidh nach eil an Windows agad ’ga ruith sa mhodh-taisbeanaidh 32 biod gach piogsail. Tha fios againn gun adhbharaich seo trioblaidean seasmhachd le GIMP, mar sin mholamaid gun atharraich thu doimhne dhathan an uidheim-thaisbeanaidh gu 32bpp mus lean thu air adhart.
Require32BPPContinue=&Lean air adhart
Require32BPPExit=&Fàg an-seo

InstallOrCustomize=Tha GIMP ullamh airson stàladh a-nis. Briog air a’ phutan “Stàlaich” gus a stàladh leis na roghainnean tùsail no briog air a’ phutan “Gnàthaich” nam bu toigh leat na thèid a stàladh a stiùireadh na mu mhionaidiche.
Install=&Stàlaich
Customize=&Gnàthaich

;setup types
TypeCompact=Stàladh teannta
TypeCustom=Stàladh gnàthaichte
TypeFull=Stàladh slàn

;text above component description
ComponentsDescription=Tuairisgeul
;components
ComponentsGimp=GIMP
ComponentsGimpDescription=GIMP agus a h-uile plugan tùsail
ComponentsDeps=Leabharlannan ruith
ComponentsDepsDescription=Na leabharlannan ruith a chleachdas GIMP, a’ gabhail a-steach àrainneachd ruith GTK+
ComponentsGtkWimp=Einnsean MS-Windows airson GTK+
ComponentsGtkWimpDescription=Coltas tùsail Windows air GIMP
ComponentsCompat=Taic ri seann-phlugain
ComponentsCompatDescription=Stàlaich na leabharlannan a dh’fheumas na seann-phlugain treas-phàrtaidh
ComponentsTranslations=Eadar-theangachaidhean
ComponentsTranslationsDescription=Eadar-theangachaidhean
ComponentsPython=Sgriobtadh Python
ComponentsPythonDescription=Leigidh seo leat plugain GIMP a chleachdadh a chaidh a sgrìobhadh sa chànan sgriobtaidh Python.
ComponentsGhostscript=Taic ri PostScript
ComponentsGhostscriptDescription=Leig le GIMP faidhlichean PostScript a luchdadh
;only when installing on x64 Windows
ComponentsGimp32=Taic ri plugain 32-biod
ComponentsGimp32Description=Gabh a-steach na faidhlichean a bhios riatanach gus plugain 32-biod a chleachdadh.%nChan fhaigh thu taic ri Python às an aonais.

;additional installation tasks
AdditionalIcons=Ìomhaigheagan a bharrachd:
AdditionalIconsDesktop=Cruthaich ìomhaigheag air an &deasg
AdditionalIconsQuickLaunch=Cruthaich ìomhaigheag &grad-tòiseachaidh

RemoveOldGIMP=Thoir air falbh an tionndadh roimhe dhe GIMP

;%1 is replaced by file name; these messages should never appear (unless user runs out of disk space at the exact right moment)
ErrorChangingEnviron=Thachair duilgheadas ag ùrachadh àrainneachd GIMP an-seo: %1. Ma gheibh thu mearachdan le luchdadh nam plugan, feuch an dì-stàlaich thu GIMP ’s gun stàlaich thu às ùr e.
ErrorExtractingTemp=Mearachd le às-tharraing dàta shealaich.
ErrorUpdatingPython=Mearachd le ùrachadh fiosrachaidh an eadar-theangadair Python.
ErrorReadingGimpRC=Mearachd le ùrachadh %1.
ErrorUpdatingGimpRC=Mearachd le ùrachadh faidhle rèiteachaidh GIMP %1.

;displayed in Explorer's right-click menu
OpenWithGimp=Deasaich le GIMP

;file associations page
SelectAssociationsCaption=Tagh co-cheangal nam faidhlichean
SelectAssociationsExtensions=Leudachain:
SelectAssociationsInfo1=Tagh na seòrsaichean faidhle a bu toigh leat co-cheangal ri GIMP
SelectAssociationsInfo2=Bheir seo air na faidhlichean gun dèid am fosgladh le GIMP nuair a nì thu briogadh dùbailte orra ann an Taisgealaiche nam faidhle.
SelectAssociationsSelectAll=T&agh na h-uile
SelectAssociationsUnselectAll=Dì-th&agh na h-uile
SelectAssociationsSelectUnused=Tagh an fheadhainn g&un chleachdadh

;shown on summary screen just before starting the install
ReadyMemoAssociations=Na seòrsaichean faidhle a thèid a cho-cheangal ri GIMP:

RemovingOldVersion=A’ toirt air falbh tionndaidhean roimhe dhe GIMP:
;%1 = version, %2 = installation directory
;ran uninstaller, but it returned an error, or didn't remove everything
RemovingOldVersionFailed=Chan urrainn dhuinn GIMP %1 a stàladh thar an tionndaidh dhe GIMP a tha stàlaichte agad an-dràsta agus dh’fhàillig le dì-stàladh fèin-obrachail an t-seann-tionndaidh.%n%nThoir air falbh an tionndadh roimhe dhe GIMP a làimh mus stàlaich thu an tionndadh seo gu %2 no tagh stàladh gnàthaichte agus pasgan stàlaidh eadar-dhealaichte.%n%nThèid an t-inneal-stàlaidh fhàgail a-nis.
;couldn't find an uninstaller, or found several uninstallers
RemovingOldVersionCantUninstall=Chan urrainn dhuinn GIMP %1 a stàladh thar an tionndaidh dhe GIMP a tha stàlaichte agad an-dràsta agus cha d’fhuair an t-inneal-stàlaidh a-mach ciamar a bheir e air falbh an seann-tionndadh gu fèin-obrachail.%n%nThoir air falbh an tionndadh roimhe dhe GIMP ’s a h-uile tuilleadan aige a làimh mus stàlaich thu an tionndadh seo gu %2 no tagh stàladh gnàthaichte agus pasgan stàlaidh eadar-dhealaichte.%n%nThèid an t-inneal-stàlaidh fhàgail a-nis.

RebootRequiredFirst=Chaidh an tionndadh roimhe dhe GIMP a thoirt air falbh ach feumaidh sinn Windows ath-thòiseachadh mus urrainn dhuinn leantainn air adhart leis an stàladh.%n%nÀs dèidh an coimpiutair agad ath-thòiseachadh, leanaidh an t-inneal-stàlaidh air adhart an ath-thuras a chlàraicheas rianaire a-steach.

;displayed if restart settings couldn't be read, or if the setup couldn't re-run itself
ErrorRestartingSetup=Thachair mearachd le ath-thòiseachadh an inneil-stàlaidh. (%1)

;displayed while the files are being extracted; note the capitalisation!
Billboard1=Cuimhnich: ’S e bathar-bog saor a th’ ann an GIMP.%n%nTadhail air
;www.gimp.org (displayed between Billboard1 and Billboard2)
Billboard2=airson ùrachaidhean saora ’s an-asgaidh.

SettingUpAssociations=A’ suidheachadh co-cheangal nam faidhlichean…
SettingUpPyGimp=A’ suidheachadh àrainneachd an leudachain Python airson GIMP…
SettingUpEnvironment=A’ suidheachadh àrainneachd GIMP…
SettingUpGimpRC=A’ suidheachadh rèiteachadh GIMP airson taic ri plugain 32-biod…

;displayed on last page
LaunchGimp=Tòisich GIMP

;shown during uninstall when removing add-ons
UninstallingAddOnCaption=A’ toirt air falbh nan tuilleadan

InternalError=Mearachd inntearnail (%1).

;used by installer for add-ons (currently only help)
DirNotGimp=Tha coltas nach deach GIMP a stàladh dhan phasgan a thagh thu. A bheil thu airson leantainn air adhart co-dhiù?
