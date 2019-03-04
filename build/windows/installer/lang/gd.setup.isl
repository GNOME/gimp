[Messages]
;InfoBefore page is used instead of license page because the GPL only concerns distribution, not use, and as such doesn't have to be accepted
WizardInfoBefore=Aonta ceadachais
AboutSetupNote=Chaidh an t-inneal-st�laidh a thogail le Jernej Simon�i�, jernej-gimp@ena.si%n%nAn dealbh air duilleag-fhosglaidh an inneil-st�laidh le Alexia_Death%nAn dealbh air duilleag-dh�naidh an inneil-st�laidh le Jakub Steiner
WinVersionTooLowError=Feumaidh an tionndadh seo dhe GIMP Windows XP le Service Pack 3 no tionndadh dhe Windows nas �ire.

[CustomMessages]
;shown before the wizard starts on development versions of GIMP
DevelopmentWarningTitle=Tionndadh an luchd-leasachaidh
;DevelopmentWarning=This is a development version of GIMP. As such, some features aren't finished, and it may be unstable. If you encounter any problems, first verify that they haven't already been fixed in GIT before you contact the developers.%nThis version of GIMP is not intended for day-to-day work, as it may be unstable, and you could lose your work. Do you wish to continue with installation anyway?
DevelopmentWarning=Seo tionndadh luchd-leasachaidh dhen inneal-st�laidh GIMP. Cha deach a chur fo dheuchainn cho teann �s a chaidh an t-inneal-st�laidh seasmhach agus dh�fhaoidte nach obraich GIMP mar bu ch�ir ri linn sin. Innis dhuinn mu dhuilgheadas sam bith air Bugzilla GIMP (Installer component):%n_https://bugzilla.gnome.org/enter_bug.cgi?product=GIMP%n%nTha duilgheadasan as aithne dhuinn aig an inneal-st�laidh seo:%n- chan obraich luchdadh fhaidhlichean TIFF%n- cha d�id meud nam faidhlichean a shealltainn mar bu ch�ir%nNa innis dhuinn mu na duilgheadasan seo on a mhothaich sinn dhaibh mar-th�.%n%nA bheil thu airson leantainn air adhart leis an st�ladh co-dhi�?
DevelopmentButtonContinue=&Lean air adhart
DevelopmentButtonExit=F�g an-seo

;XPSP3Recommended=Warning: you are running an unsupported version of Windows. Please update to at least Windows XP with Service Pack 3 before reporting any problems.
SSERequired=Feumaidh an tionndadh seo dhe GIMP pr�iseasar a chuireas tais ri sti�ireadh SSE.

Require32BPPTitle=Duilgheadas le roghainnean an uidheim-thaisbeanaidh
Require32BPP=Mhothaich an t-inneal-st�laidh nach eil an Windows agad �ga ruith sa mhodh-taisbeanaidh 32 biod gach piogsail. Tha fios againn gun adhbharaich seo trioblaidean seasmhachd le GIMP, mar sin mholamaid gun atharraich thu doimhne dhathan an uidheim-thaisbeanaidh gu 32bpp mus lean thu air adhart.
Require32BPPContinue=&Lean air adhart
Require32BPPExit=&F�g an-seo

InstallOrCustomize=Tha GIMP ullamh airson st�ladh a-nis. Briog air a� phutan �St�laich� gus a st�ladh leis na roghainnean t�sail no briog air a� phutan �Gn�thaich� nam bu toigh leat na th�id a st�ladh a sti�ireadh na mu mhionaidiche.
Install=&St�laich
Customize=&Gn�thaich

;setup types
TypeCompact=St�ladh teannta
TypeCustom=St�ladh gn�thaichte
TypeFull=St�ladh sl�n

;text above component description
ComponentsDescription=Tuairisgeul
;components
ComponentsGimp=GIMP
ComponentsGimpDescription=GIMP agus a h-uile plugan t�sail
ComponentsDeps=Leabharlannan ruith
ComponentsDepsDescription=Na leabharlannan ruith a chleachdas GIMP, a� gabhail a-steach �rainneachd ruith GTK+
ComponentsGtkWimp=Einnsean MS-Windows airson GTK+
ComponentsGtkWimpDescription=Coltas t�sail Windows air GIMP
ComponentsCompat=Taic ri seann-phlugain
ComponentsCompatDescription=St�laich na leabharlannan a dh�fheumas na seann-phlugain treas-ph�rtaidh
ComponentsTranslations=Eadar-theangachaidhean
ComponentsTranslationsDescription=Eadar-theangachaidhean
ComponentsPython=Sgriobtadh Python
ComponentsPythonDescription=Leigidh seo leat plugain GIMP a chleachdadh a chaidh a sgr�obhadh sa ch�nan sgriobtaidh Python.
ComponentsGhostscript=Taic ri PostScript
ComponentsGhostscriptDescription=Leig le GIMP faidhlichean PostScript a luchdadh
;only when installing on x64 Windows
ComponentsGimp32=Taic ri plugain 32-biod
ComponentsGimp32Description=Gabh a-steach na faidhlichean a bhios riatanach gus plugain 32-biod a chleachdadh.%nChan fhaigh thu taic ri Python �s an aonais.

;additional installation tasks
AdditionalIcons=�omhaigheagan a bharrachd:
AdditionalIconsDesktop=Cruthaich �omhaigheag air an &deasg
AdditionalIconsQuickLaunch=Cruthaich �omhaigheag &grad-t�iseachaidh

RemoveOldGIMP=Thoir air falbh an tionndadh roimhe dhe GIMP

;%1 is replaced by file name; these messages should never appear (unless user runs out of disk space at the exact right moment)
ErrorChangingEnviron=Thachair duilgheadas ag �rachadh �rainneachd GIMP an-seo: %1. Ma gheibh thu mearachdan le luchdadh nam plugan, feuch an d�-st�laich thu GIMP �s gun st�laich thu �s �r e.
ErrorExtractingTemp=Mearachd le �s-tharraing d�ta shealaich.
ErrorUpdatingPython=Mearachd le �rachadh fiosrachaidh an eadar-theangadair Python.
ErrorReadingGimpRC=Mearachd le �rachadh %1.
ErrorUpdatingGimpRC=Mearachd le �rachadh faidhle r�iteachaidh GIMP %1.

;displayed in Explorer's right-click menu
OpenWithGimp=Deasaich le GIMP

;file associations page
SelectAssociationsCaption=Tagh co-cheangal nam faidhlichean
SelectAssociationsExtensions=Leudachain:
SelectAssociationsInfo1=Tagh na se�rsaichean faidhle a bu toigh leat co-cheangal ri GIMP
SelectAssociationsInfo2=Bheir seo air na faidhlichean gun d�id am fosgladh le GIMP nuair a n� thu briogadh d�bailte orra ann an Taisgealaiche nam faidhle.
SelectAssociationsSelectAll=T&agh na h-uile
SelectAssociationsUnselectAll=D�-th&agh na h-uile
SelectAssociationsSelectUnused=Tagh an fheadhainn g&un chleachdadh

;shown on summary screen just before starting the install
ReadyMemoAssociations=Na se�rsaichean faidhle a th�id a cho-cheangal ri GIMP:

RemovingOldVersion=A� toirt air falbh tionndaidhean roimhe dhe GIMP:
;%1 = version, %2 = installation directory
;ran uninstaller, but it returned an error, or didn't remove everything
RemovingOldVersionFailed=Chan urrainn dhuinn GIMP %1 a st�ladh thar an tionndaidh dhe GIMP a tha st�laichte agad an-dr�sta agus dh�fh�illig le d�-st�ladh f�in-obrachail an t-seann-tionndaidh.%n%nThoir air falbh an tionndadh roimhe dhe GIMP a l�imh mus st�laich thu an tionndadh seo gu %2 no tagh st�ladh gn�thaichte agus pasgan st�laidh eadar-dhealaichte.%n%nTh�id an t-inneal-st�laidh fh�gail a-nis.
;couldn't find an uninstaller, or found several uninstallers
RemovingOldVersionCantUninstall=Chan urrainn dhuinn GIMP %1 a st�ladh thar an tionndaidh dhe GIMP a tha st�laichte agad an-dr�sta agus cha d�fhuair an t-inneal-st�laidh a-mach ciamar a bheir e air falbh an seann-tionndadh gu f�in-obrachail.%n%nThoir air falbh an tionndadh roimhe dhe GIMP �s a h-uile tuilleadan aige a l�imh mus st�laich thu an tionndadh seo gu %2 no tagh st�ladh gn�thaichte agus pasgan st�laidh eadar-dhealaichte.%n%nTh�id an t-inneal-st�laidh fh�gail a-nis.

RebootRequiredFirst=Chaidh an tionndadh roimhe dhe GIMP a thoirt air falbh ach feumaidh sinn Windows ath-th�iseachadh mus urrainn dhuinn leantainn air adhart leis an st�ladh.%n%n�s d�idh an coimpiutair agad ath-th�iseachadh, leanaidh an t-inneal-st�laidh air adhart an ath-thuras a chl�raicheas rianaire a-steach.

;displayed if restart settings couldn't be read, or if the setup couldn't re-run itself
ErrorRestartingSetup=Thachair mearachd le ath-th�iseachadh an inneil-st�laidh. (%1)

;displayed while the files are being extracted; note the capitalisation!
Billboard1=Cuimhnich: �S e bathar-bog saor a th� ann an GIMP.%n%nTadhail air
;www.gimp.org (displayed between Billboard1 and Billboard2)
Billboard2=airson �rachaidhean saora �s an-asgaidh.

SettingUpAssociations=A� suidheachadh co-cheangal nam faidhlichean�
SettingUpPyGimp=A� suidheachadh �rainneachd an leudachain Python airson GIMP�
SettingUpEnvironment=A� suidheachadh �rainneachd GIMP�
SettingUpGimpRC=A� suidheachadh r�iteachadh GIMP airson taic ri plugain 32-biod�

;displayed on last page
LaunchGimp=T�isich GIMP

;shown during uninstall when removing add-ons
UninstallingAddOnCaption=A� toirt air falbh nan tuilleadan

InternalError=Mearachd inntearnail (%1).

;used by installer for add-ons (currently only help)
DirNotGimp=Tha coltas nach deach GIMP a st�ladh dhan phasgan a thagh thu. A bheil thu airson leantainn air adhart co-dhi�?
