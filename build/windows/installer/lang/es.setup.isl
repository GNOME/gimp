[Messages]
;InfoBefore page is used instead of license page because the GPL only concerns distribution, not use, and as such doesn't have to be accepted
WizardInfoBefore=Acuerdo de Licencia
AboutSetupNote=Instalación creada por Jernej Simonèiè, jernej-gimp@ena.si%n%nImagen en la página de inicio de la Instalación por Alexia_Death%nImagen en la página final de la Instalación por Jakub Steiner
WinVersionTooLowError=Esta versión de GIMP requiere Windows XP con Service Pack 3, o una versión más reciente de Windows.

[CustomMessages]
;shown before the wizard starts on development versions of GIMP
DevelopmentWarningTitle=Versión de Desarrollo
;DevelopmentWarning=Esta es una versión de desarrollo de GIMP. Como tal, algunas características están incompletas y pueden ser inestables. Si usted encuentra algún problema, primero verifique que no haya sido solucionado en el GIT antes de contactar a los desarrolladores.%nEsta versión de GIMP no está orientada a trabajo diario o a ambientes de producción, ya que puede ser inestable y podría perder su trabajo. ¿Desea continuar con la instalación de todos modos?
DevelopmentWarning=Esta es una versión de desarrollo del instalador de GIMP. No ha sido probado tan profundamente como el instalador estable, lo que puede resultar en que GIMP no funcione apropiadamente. Por favor reporte cualquier problema que usted encuentre en el bugzilla de GIMP (Installer component):%n_https://bugzilla.gnome.org/enter_bug.cgi?product=GIMP%n%n¿Desea continuar con la instalación de todos modos?
DevelopmentButtonContinue=&Continuar
DevelopmentButtonExit=&Salir

;XPSP3Recommended=Aviso: usted está ejecutando una versión no soportada de Windows. Por favor actualice al menos a Windows XP con Service Pack 3 antes de reportar algún problema.
SSERequired=Esta versión de GIMP requiere un procesador que soporte instrucciones SSE.

Require32BPPTitle=Problema con la configuración de vídeo de su pantalla
Require32BPP=El instalador ha detectado que su Windows no se está ejecutando a 32 bits por píxel de profundidad de color. Se sabe que esto puede causar problemas de estabilidad al GIMP, por lo que se le recomienda que cambie la profundidad de color de la configuración de vídeo de su pantalla a 32BPP antes de continuar.
Require32BPPContinue=&Continuar
Require32BPPExit=&Salir

InstallOrCustomize=GIMP está listo para ser instalado. Haga clic en el botón Instalar para instalar usando la configuración por defecto, o haga clic en el botón Personalizar si desea un mayor control sobre lo que va a instalar.
Install=&Instalar
Customize=&Personalizar

;setup types
TypeCompact=Instalación Compacta
TypeCustom=Instalación Personalizada
TypeFull=Instalación Completa

;text above component description
ComponentsDescription=Descripción
;components
ComponentsGimp=GIMP
ComponentsGimpDescription=GIMP y todos los plug-ins por defecto
ComponentsDeps=Bibliotecas Run-time
ComponentsDepsDescription=Bibliotecas Run-time usadas por GIMP, incluyendo bibliotecas Run-time del Entorno GTK+
ComponentsGtkWimp=Motor(Engine) MS-Windows para GTK+
ComponentsGtkWimpDescription=Aspecto nativo de Windows para GIMP
ComponentsCompat=Soporte para plug-ins antiguos
ComponentsCompatDescription=Instala bibliotecas requeridas por plug-ins antiguos de terceros
ComponentsTranslations=Traducciones
ComponentsTranslationsDescription=Traducciones
ComponentsPython=Python scripting
ComponentsPythonDescription=Le permite usar plug-ins de GIMP escritos en el lenguaje interpretado Python.
ComponentsGhostscript=Soporte para PostScript
ComponentsGhostscriptDescription=Permite a GIMP abrir archivos PostScript
;only when installing on x64 Windows
ComponentsGimp32=Soporte para plug-ins de 32-bit
ComponentsGimp32Description=Incluye archivos necesarios para usar plug-ins de 32-bit.%nRequerido para soportar Python.

;additional installation tasks
AdditionalIcons=Iconos adicionales:
AdditionalIconsDesktop=Crear un icono de acceso directo en el &Escritorio
AdditionalIconsQuickLaunch=Crear un icono de acceso directo en la barra de Inicio &Rápido

RemoveOldGIMP=Elimina versión anterior de GIMP

;%1 is replaced by file name; these messages should never appear (unless user runs out of disk space at the exact right moment)
ErrorChangingEnviron=Ocurrió un problema al actualizar el ambiente de GIMP en %1. Si encuentra algún error cargando los plug-ins, pruebe desinstalar y reinstalar GIMP.
ErrorExtractingTemp=Error al extraer los archivos temporales.
ErrorUpdatingPython=Error al actualizar la información del intérprete de Python.
ErrorReadingGimpRC=Ocurrió un problema al actualizar %1.
ErrorUpdatingGimpRC=Ocurrió un problema al actualizar el archivo de configuración de GIMP %1.

;displayed in Explorer's right-click menu
OpenWithGimp=Editar con GIMP

;file associations page
SelectAssociationsCaption=Seleccione la asociación de archivos
SelectAssociationsExtensions=Extensiones:
SelectAssociationsInfo1=Seleccione los tipos de archivo que desea asociar con GIMP
SelectAssociationsInfo2=Esto hará que los tipos de archivo seleccionados se abran con GIMP cuando haga doble clic sobre ellos en el Explorador.
SelectAssociationsSelectAll=Seleccionar &Todos
SelectAssociationsUnselectAll=Deseleccionar T&odos
SelectAssociationsSelectUnused=Seleccionar los no &Utilizados

;shown on summary screen just before starting the install
ReadyMemoAssociations=Tipos de archivo que se asociarán con GIMP:

RemovingOldVersion=Eliminando versión anterior de GIMP:
;%1 = version, %2 = installation directory
;ran uninstaller, but it returned an error, or didn't remove everything
RemovingOldVersionFailed=GIMP %1 no se puede instalar sobre la versión de GIMP instalado actualmente, y la desinstalación automática de la versión antigua ha fallado.%n%nPor favor desinstale la versión anterior de GIMP usted mismo antes de instalar esta versión en %2, o seleccione Instalación Personalizada y escoja otra carpeta de instalación.%n%nEl instalador se cerrará ahora.
;couldn't find an uninstaller, or found several uninstallers
RemovingOldVersionCantUninstall=GIMP %1 no se puede instalar sobre la versión de GIMP instalado actualmente, y el instalador no pudo determinar como eliminar la versión antigua automáticamente.%n%nPor favor desinstale la versión anterior de GIMP y todos sus complementos(add-ons) usted mismo antes de instalar esta versión en %2, o seleccione Instalación Personalizada y escoja otra carpeta de instalación.%n%nEl instalador se cerrará ahora.

RebootRequiredFirst=La versión anterior de GIMP se eliminó satisfactoriamente, pero Windows necesita reiniciar antes de que el instalador pueda continuar.%n%nDespués de reiniciar su computadora, el instalador continuará la próxima vez que un administrador inicie sesión en el sistema.

;displayed if restart settings couldn't be read, or if the setup couldn't re-run itself
ErrorRestartingSetup=Ocurrió un problema al reiniciar el instalador. (%1)

;displayed while the files are being extracted; note the capitalisation!
Billboard1=Recuerde: GIMP es Software Libre.%n%nPor favor visite
;www.gimp.org (displayed between Billboard1 and Billboard2)
Billboard2=para obtener actualizaciones gratuitas.

SettingUpAssociations=Estableciendo la asociación de archivos...
SettingUpPyGimp=Estableciendo el entorno para las extensiones en Python de GIMP...
SettingUpEnvironment=Estableciendo el entorno de GIMP...
SettingUpGimpRC=Estableciendo la configuración de GIMP para el soporte de plug-ins de 32-bit...

;displayed on last page
LaunchGimp=Iniciar GIMP

;shown during uninstall when removing add-ons
UninstallingAddOnCaption=Eliminando complementos(add-ons)

InternalError=Error interno (%1).

;used by installer for add-ons (currently only help)
DirNotGimp=GIMP no parece estar instalado en el directorio seleccionado. ¿Desea continuar de todos modos?
