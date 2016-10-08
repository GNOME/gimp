[Messages]
;InfoBefore page is used instead of license page because the GPL only concerns distribution, not use, and as such doesn't have to be accepted
WizardInfoBefore=Лицензионное соглашение
AboutSetupNote=Создатель пакета установки Jernej Simoncic, jernej-gimp@ena.si%n%nАвтор изображения в начале установки Alexia_Death%nАвтор изображения в конце установки Jakub Steiner
WinVersionTooLowError=Для этой версии GIMP требуется Windows XP с Пакетом обновлений 3 (Service Pack 3), или более современная версия Windows.

[CustomMessages]
;shown before the wizard starts on development versions of GIMP
DevelopmentWarningTitle=Разрабатываемая версия
;DevelopmentWarning=Это разрабатываемая версия GIMP. Некоторые функции не доделаны, и они могут работать нестабильно. Если у Вас возникнут какие-либо проблемы, сначала убедитесь, что они ещё не были исправлены в GIT, прежде чем связаться с разработчиками.%nЭта версия GIMP не предназначена для повседневной работы, так как она может работать нестабильно, и Вы можете потерять свои данные. Вы хотите продолжить установку?
DevelopmentWarning=Это разрабатываемая версия GIMP. Она не была протестирована так же, как стабильная версия, в результате GIMP может не работать должным образом. Просим Вас сообщить о возникших проблемах в GIMP bugzilla (компонент Installer):%n_https://bugzilla.gnome.org/enter_bug.cgi?product=GIMP%n%nВы хотите продолжить установку?
DevelopmentButtonContinue=&Далее
DevelopmentButtonExit=&Выход

;XPSP3Recommended=Внимание: вы используете неподдерживаемую версию Windows. Пожалуйста, обновите её до Windows XP с Пакетом обновлений 3 (Service Pack 3), прежде чем сообщать о проблемах.
SSERequired=Этой версии GIMP требуется процессор с поддержкой инструкций SSE.

Require32BPPTitle=Проблема с параметрами дисплея
Require32BPP=Программа установки обнаружила, что Ваша Windows сейчас работает с качеством цветопередачи не 32 бита. Известно, что это может вызвать нестабильную работу GIMP, поэтому рекомендуется сменить качество цветопередачи на 32 бита в параметрах дисплея, прежде чем продолжить.
Require32BPPContinue=&Далее
Require32BPPExit=&Выход

InstallOrCustomize=GIMP готов к установке. Нажмите Установить для установки с настройками по умолчанию, или Настроить для выбора компонентов.
Install=&Установить
Customize=&Настроить

;setup types
TypeCompact=Компактная установка
TypeCustom=Выборочная установка
TypeFull=Полная установка

;text above component description
ComponentsDescription=Описание
;components
ComponentsGimp=GIMP
ComponentsGimpDescription=GIMP и все стандартные плагины
ComponentsDeps=Библиотеки времени выполнения
ComponentsDepsDescription=Библиотеки времени выполнения для GIMP, включая окружение времени выполнения GTK+
ComponentsGtkWimp=Движок MS-Windows для GTK+
ComponentsGtkWimpDescription=Интегрированный внешний вид для GIMP
ComponentsCompat=Поддержка старых плагинов
ComponentsCompatDescription=Установка библиотек, необходимых для старых сторонних плагинов
ComponentsTranslations=Локализации
ComponentsTranslationsDescription=Локализации
ComponentsPython=Автоматизация на Python
ComponentsPythonDescription=Обеспечивает работу плагинов GIMP, написанных на языке Python.
ComponentsGhostscript=Поддержка PostScript
ComponentsGhostscriptDescription=Обеспечивает загрузку файлов PostScript в GIMP
;only when installing on x64 Windows
ComponentsGimp32=Поддержка 32-битных плагинов
ComponentsGimp32Description=Включает файлы, необходимые для использования 32-битных плагинов.%nНеобходимо для поддержки Python.

;additional installation tasks
AdditionalIcons=Дополнительные значки:
AdditionalIconsDesktop=Создать значок на &Рабочем столе
AdditionalIconsQuickLaunch=Создать значок в &Панели быстрого запуска

RemoveOldGIMP=Удалить предыдущую версию GIMP

;%1 is replaced by file name; these messages should never appear (unless user runs out of disk space at the exact right moment)
ErrorChangingEnviron=Возникла проблема при обновлении окружения GIMP в %1. Если Вы получите ошибки при загрузке плагинов, попробуйте удалить и заново установить GIMP.
ErrorExtractingTemp=Ошибка при извлечении временных данных.
ErrorUpdatingPython=Ошибка обновления информации интерпретатора Python.
ErrorReadingGimpRC=Возникла ошибка при обновлении %1.
ErrorUpdatingGimpRC=Возникла ошибка при обновлении файла настроек GIMP %1.

;displayed in Explorer's right-click menu
OpenWithGimp=Изменить в GIMP

;file associations page
SelectAssociationsCaption=Выбор файловых ассоциаций
SelectAssociationsExtensions=Расширения:
SelectAssociationsInfo1=Выберите типы файлов, которые будут ассоциированы с GIMP
SelectAssociationsInfo2=Это позволит открывать выбранные файлы в GIMP по двойному щелчку в Проводнике.
SelectAssociationsSelectAll=&Выбрать все
SelectAssociationsUnselectAll=&Снять все
SelectAssociationsSelectUnused=Выбрать &неиспользуемые

;shown on summary screen just before starting the install
ReadyMemoAssociations=Типы файлов, которые будут ассоциированы с GIMP:

RemovingOldVersion=Удаление предыдущей версии GIMP:
;%1 = version, %2 = installation directory
;ran uninstaller, but it returned an error, or didn't remove everything
RemovingOldVersionFailed=GIMP %1 не может быть установлен поверх уже установленной версии GIMP, и автоматическое удаление старой версии не удалось.%n%nПожалуйста, удалите предыдущую версию GIMP вручную, прежде чем устанавливать эту версию в %2, или нажмите Настроить в начале, и выберите другую папку для установки.%n%nСейчас установка будет прервана.
;couldn't find an uninstaller, or found several uninstallers
RemovingOldVersionCantUninstall=GIMP %1 не может быть установлен поверх уже установленной версии GIMP, и программа установки не может определить, как удалить предыдущую версию автоматически.%n%nПожалуйста, удалите предыдущую версию GIMP и все дополнения вручную, прежде чем устанавливать эту версию в %2, или нажмите Настроить в начале, и выберите другую папку для установки.%n%nСейчас установка будет прервана.

RebootRequiredFirst=Предыдущая версия GIMP успешно удалена, но необходимо перезагрузить Windows перед продолжением установки.%n%nПосле перезагрузки компьютера установка будет продолжена, как только любой пользователь с правами администратора войдёт в систему.

;displayed if restart settings couldn't be read, or if the setup couldn't re-run itself
ErrorRestartingSetup=Возникла ошибка при перезапуске программы установки. (%1)

;displayed while the files are being extracted; note the capitalisation!
Billboard1=Помните: GIMP является Свободным программным обеспечением.%n%nПожалуйста, посетите
;www.gimp.org (displayed between Billboard1 and Billboard2)
Billboard2=для бесплатных обновлений.

SettingUpAssociations=Установка файловых ассоциаций...
SettingUpPyGimp=Установка окружения для дополнений GIMP Python...
SettingUpEnvironment=Установка окружения GIMP...
SettingUpGimpRC=Установка настроек GIMP для поддержки 32-битных плагинов...

;displayed on last page
LaunchGimp=Запустить GIMP

;shown during uninstall when removing add-ons
UninstallingAddOnCaption=Удаление дополнений

InternalError=Внутренняя ошибка (%1).

;used by installer for add-ons (currently only help)
DirNotGimp=Кажется, GIMP не был установлен в выбранный каталог. Всё равно продолжить?
