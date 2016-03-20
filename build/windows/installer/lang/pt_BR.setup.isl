[Messages]
;InfoBefore page is used instead of license page because the GPL only concerns distribution, not use, and as such doesn't have to be accepted
WizardInfoBefore=Acordo de Licenciamento
AboutSetupNote=Instalador feito por Jernej Simonèiè, jernej-gimp@ena.si%n%nImagem na página de abertura do instalador po Alexia_Death%nImagem no final do instalador por Jakub Steiner
WinVersionTooLowError= Esta versão do GIMP requer o Windows XP com o Service Pack 3 ou uma versão mais recente do Windows.

[CustomMessages]
;shown before the wizard starts on development versions of GIMP
DevelopmentWarningTitle=Versão de Desenvolvimento
DevelopmentWarning=Esta é uma versão de desenvolvimento do GIMP. Sendo assim, algumas funcionalidades não estão prontas, e ele pode ser instável. Se você encontrar algum problema, primeiro verifique se ele já não foi arrumado no GIT antes de contatar os desenvolvedores.%nEsta versão do GIMP não foi feita para ser usada no dia a dia, uma vez que ela pode ser instável e pode por o seu trabalho em risco. Você quer continuar com esta instalação mesmo assim?
;DevelopmentWarning=Esta é uma versão de desenvolvimento do instalador do GIMP. Ela não foi testada tanto quanto o instalador estável, o que pode resultar no GIMP não ser corretamente instalado. Por favor, reporte quaisquer problemas que você encontrar no bugizlla do GIMP (Installer component):%n_https://bugzilla.gnome.org/enter_bug.cgi?product=GIMP%n%nHá alguns problemas conhecidos neste instalador:%n- Arquivos TIFF não estão abrindo%n-Tamanho dos arquivos não estão sendo exibidos corretamente%nPor favor não reporte esses problemas, uma vez que já estamos a par dos mesmos.%n%nVocê quer continuar com a instalação mesmo assim?
DevelopmentButtonContinue=&Continuar
DevelopmentButtonExit=Sair

;XPSP3Recommended=Aviso: Você está executando uma versão não suportada do Windows. Por favor atualize pelo menos para WIndows XP com Service Pack 3 antes de mencionar quaisquer problemas.
SSERequired=Esta versão do GIMP requer um processador que suporte as instruções SSE.

Require32BPPTitle=Problema nas configurações de tela
Require32BPP=O instalador detectou que seu windows não está rodando num modo de tela de 32 bits por pixel. Isso sabidamente pode causar alguns problemas de estabilidade com o GIMP, então é recomendado mudar a profundidade de cores da tela para 32BPP antesd e continuar.
Require32BPPContinue=&Continuar
Require32BPPExit=Sair

InstallOrCustomize=O GIMP está pronto para ser instalado. Clique no botão Instalar para instalar com as configurações padrão, ou clique no botão Personalizar se você deseja ter mais controle sobre o que será instalado.
Install=&Instalar
Customize=&Personalizar

;setup types
TypeCompact=Instalação Compacta
TypeCustom=Instalação personalizada
TypeFull=Instalação completa

;text above component description
ComponentsDescription=Descrição
;components
ComponentsGimp=GIMP
ComponentsGimpDescription=GIMP e todos os plug-ins padrão
ComponentsDeps=Bibliotecas de execução
ComponentsDepsDescription=Biblitoecas de execução utilizadas pelo GIMP, incluindo o ambiente de execução do GTK+
ComponentsGtkWimp=Motor do GTK+ para MS-Windows
ComponentsGtkWimpDescription=Aparência nativa de Windows para o GIMP
ComponentsCompat=Suporte para plug-ins antigos
ComponentsCompatDescription=Instalar bibliotecas necessárias para plug-ins antigos de terceiros
ComponentsTranslations=Traduções
ComponentsTranslationsDescription=Traduções
ComponentsPython=Suporte a scripts em Python
ComponentsPythonDescription=Permite que você use plug-ins escritos na linguagem Python(necessário para algumas funcionalidades).
ComponentsGhostscript=Suporte a PostScript
ComponentsGhostscriptDescription=Permite que o GIMP possa abrir arquivos PostScript
;only when installing on x64 Windows
ComponentsGimp32=Suporte a plug-ins de 32-bit
ComponentsGimp32Description=Inclui arquivos necessários para o uso de plug-ins de 32bits.%nNecessário para o suporte a Python.

;additional installation tasks
AdditionalIcons=Icones adicionais:
AdditionalIconsDesktop=Criar um ícone de na Área de Trabalho
AdditionalIconsQuickLaunch=Criar um ícone de Lançamento Rápido

RemoveOldGIMP=Remover a versão anterior do GIMP

;%1 is replaced by file name; these messages should never appear (unless user runs out of disk space at the exact right moment)
ErrorChangingEnviron=Houve um problema ao atualizar o ambiente em %1. Se você ver algum erro ao carregar os plug-ins, tente desisntalar e re-instalar o GIMP
ErrorExtractingTemp=Erro ao extrair dados temporários.
ErrorUpdatingPython=Erro ao atualizar informações do interpretador Python.
ErrorReadingGimpRC=Houve um erro ao atualizar %1.
ErrorUpdatingGimpRC=Houve um erro ao atualizar o arquivo de configuração do GIMP %1.

;displayed in Explorer's right-click menu
OpenWithGimp=Editar com o GIMP

;file associations page
SelectAssociationsCaption=Escolha as associações de arquivos
SelectAssociationsExtensions=Extenções:
SelectAssociationsInfo1=Selecione os tipos de arquivos que você deseja associar com  GIMP
SelectAssociationsInfo2=Isso fará com que os arquivos selecionados abram no GIMP quando você clicar nos mesos no Explorer.
SelectAssociationsSelectAll=Selecionar &todos
SelectAssociationsUnselectAll=&Des-selecionar todos
SelectAssociationsSelectUnused=Selecionar os &não utilizados

;shown on summary screen just before starting the install
ReadyMemoAssociations=Tipos de arquivo que serão associados ao GIMP:

RemovingOldVersion=Removendo versão anterior do GIMP:
;%1 = version, %2 = installation directory
;ran uninstaller, but it returned an error, or didn't remove everything
RemovingOldVersionFailed=GIMP %1 não pode ser instalado por cima da sua versão do GIMP instalada atualmente, e a desinstalação automatica da versão antiga falhou.%n%nPor favor, remova manualmente a versão anterior do GIMP antes de instalar esta versão em %2, ou escolha a Instalação Personalizada e selecione uma pasta diferente para instalação.%n%nA instalação será encerrada.
;couldn't find an uninstaller, or found several uninstallers
RemovingOldVersionCantUninstall=GIMP %1 não pode ser instalado por cima da sua versão do GIMP instalada atualmente, e  Instalador não pode determinar como remover a versão anterior automaticamente.%n%nPor favor, remova manualmente a versão anterior do GIMP antes de instalar esta versão em %2, ou escolha a Instalação Personalizada e selecione uma pasta diferente para instalação.%n%nA instalação será encerrada.

RebootRequiredFirst=A versão anterior do GIMP foi removida com sucesso, mas o Windows deve ser reiniciado antes que o instalador possa continuar.%n%nApós reiniciar seu computador, o Instalador vai continuar assim que um Administrador fizer o login.

;displayed if restart settings couldn't be read, or if the setup couldn't re-run itself
ErrorRestartingSetup=Houve um erro ao reiniciar o instalador do GIMP. (%1)

;displayed while the files are being extracted; note the capitalisation!
Billboard1=Lembre-se: o GIMP é Software Livre.%n%nPor favor visite
;www.gimp.org (displayed between Billboard1 and Billboard2)
Billboard2=para atualizações gratuitas

SettingUpAssociations=Configurando associações de arquivo...
SettingUpPyGimp=Configurando ambiente para a extensão Python do GIMP...
SettingUpEnvironment=Configurando ambiente do GIMP...
SettingUpGimpRC=Configurando o suporte a plug-ins de 32bit...

;displayed on last page
LaunchGimp=Iniciar o GIMP

;shown during uninstall when removing add-ons
UninstallingAddOnCaption=Removendo o componente extra

InternalError=Erro interno (%1).

;used by installer for add-ons (currently only help)
DirNotGimp=GIMP não parece estar instalado no diretório selecionado. Continuar mesmo assim?
