#!/usr/bin/env pwsh

# Parameters
param ($build_dir = '_build',
       $a64_bundle = 'gimp-a64',
       $x64_bundle = 'gimp-x64')


# Autodetects latest WinSDK installed
if ($Env:PROCESSOR_ARCHITECTURE -eq 'ARM64')
  {
    $cpu_arch = 'arm64'
  }
else
  {
    $cpu_arch = 'x64'
  }
$win_sdk_path = Resolve-Path "C:\Program Files (x86)\Windows Kits\10\bin\*\$cpu_arch" | Select-Object -Last 1

# Needed tools from Windows SDK
Set-Alias 'makepri' "$win_sdk_path\makepri.exe"
Set-Alias 'makeappx' 'C:\Program Files (x86)\Windows Kits\10\App Certification Kit\MakeAppx.exe'
Set-Alias 'signtool' "$win_sdk_path\signtool.exe"


# Global variables
$config_path = "$build_dir\config.h"

## Get Identity Name (the dir shown in Explorer)
$GIMP_UNSTABLE = Get-Content "$CONFIG_PATH"                               | Select-String 'GIMP_UNSTABLE' |
                 Foreach-Object {$_ -replace '#define GIMP_UNSTABLE ',''}
if ($GIMP_UNSTABLE -eq '1')
  {
    $IDENTITY_NAME="GIMP.GIMPPreview"
  }
else
  {
    $IDENTITY_NAME="GIMP.GIMP"
  }

## Get GIMP version (major.minor.micro)
$GIMP_VERSION = Get-Content "$CONFIG_PATH"                               | Select-String 'GIMP_VERSION'        |
                Foreach-Object {$_ -replace '#define GIMP_VERSION "',''} | Foreach-Object {$_ -replace '"',''}


# Autodetects what arch bundles will be packaged
Copy-Item .gitignore .gitignore.bak
$supported_archs = "$a64_bundle","$x64_bundle"
foreach ($bundle in $supported_archs)
  {
    if (Test-Path "$bundle")
      {
        if (("$bundle" -like '*a64*') -or ("$bundle" -like '*aarch64*') -or ("$bundle" -like '*arm64*'))
          {
            $msix_arch = 'arm64'
          }
        else
          {
            $msix_arch = 'x64'
          }

        $ig_content = "`n$bundle`n$msix_arch`n*.appxsym`n*.zip"
        if (Test-Path .gitignore -Type Leaf)
          {
            Add-Content .gitignore "$ig_content"
          }
        else
          {
            New-Item .gitignore
            Set-Content .gitignore "$ig_content"
          }

        New-Item $msix_arch -ItemType Directory


        # 1. CONFIGURE MANIFEST
        Copy-Item build\windows\store\AppxManifest.xml $msix_arch

        ## Set Identity Name
        (Get-Content $msix_arch\AppxManifest.xml) | Foreach-Object {$_ -replace "@IDENTITY_NAME@","$IDENTITY_NAME"} |
        Set-Content $msix_arch\AppxManifest.xml

        ## Set Display Name (the name shown in MS Store)
        if ($GIMP_UNSTABLE -eq '1')
          {
            $display_name='GIMP (Preview)'
          }
        else
          {
            $display_name='GIMP'
          }
        (Get-Content $msix_arch\AppxManifest.xml) | Foreach-Object {$_ -replace "@DISPLAY_NAME@","$display_name"} |
        Set-Content $msix_arch\AppxManifest.xml

        ## Set GIMP version
        (Get-Content $msix_arch\AppxManifest.xml) | Foreach-Object {$_ -replace "@GIMP_VERSION@","$GIMP_VERSION"} |
        Set-Content $msix_arch\AppxManifest.xml

        ## Set GIMP app version (major.minor)
        $gimp_app_version = Get-Content "$CONFIG_PATH"                                   | Select-String 'GIMP_APP_VERSION "'  |
                            Foreach-Object {$_ -replace '#define GIMP_APP_VERSION "',''} | Foreach-Object {$_ -replace '"',''}
        (Get-Content $msix_arch\AppxManifest.xml) | Foreach-Object {$_ -replace "@GIMP_APP_VERSION@","$gimp_app_version"} |
        Set-Content $msix_arch\AppxManifest.xml

        ## Set msix_arch
        (Get-Content $msix_arch\AppxManifest.xml) | Foreach-Object {$_ -replace "neutral","$msix_arch"} |
        Set-Content $msix_arch\AppxManifest.xml

        ## Match supported filetypes
        $file_types = Get-Content 'build\windows\installer\data_associations.list'       | Foreach-Object {"              <uap:FileType>." + $_} |
                      Foreach-Object {$_ +  "</uap:FileType>"}                           | Where-Object {$_ -notmatch 'xcf'}
        (Get-Content $msix_arch\AppxManifest.xml) | Foreach-Object {$_ -replace "@FILE_TYPES@","$file_types"}  |
        Set-Content $msix_arch\AppxManifest.xml


        # 2. CREATE ASSETS

        ## Copy pre-generated icons to each msix_arch
        $icons_path = "$build_dir\build\windows\store\Assets"
        if (Test-Path "$icons_path")
          {
            New-Item $msix_arch\Assets -ItemType Directory
            Copy-Item "$icons_path\*.png" $msix_arch\Assets\ -Recurse
          }
        else
          {
            "(ERROR): MS Store icons not found. You can build them with '-Dms-store=true'"
            exit 1
          }

        ## Generate resources.pri
        Set-Location $msix_arch
        makepri createconfig /cf priconfig.xml /dq lang-en-US /pv 10.0.0
        Set-Location ..\
        makepri new /pr $msix_arch /cf $msix_arch\priconfig.xml /of $msix_arch
        Remove-Item $msix_arch\priconfig.xml


        # 3. COPY GIMP FILES
        $vfs = "$msix_arch\VFS\ProgramFilesX64\GIMP"

        ## Copy files into VFS folder (to support external 3P plug-ins)
        Copy-Item "$bundle" "$vfs" -Recurse -Force

        ## Remove uneeded files (to match the Inno Windows Installer artifact)
        Get-ChildItem "$vfs" -Recurse -Include (".gitignore", "gimp.cmd") | Remove-Item -Recurse

        ## Disable Update check (ONLY FOR RELEASES)
        if ($CI_COMMIT_TAG -or ($GIMP_CI_MS_STORE -eq 'MSIXUPLOAD'))
          {
            Add-Content "$vfs\share\gimp\*\gimp-release" 'check-update=false'
          }

        ## Remove uncompliant files (to avoid WACK/'signtool' issues)
        Get-ChildItem "$vfs" -Recurse -Include ("*.debug", "*.tar") | Remove-Item -Recurse


        # 4. MAKE .MSIX AND CORRESPONDING .APPXSYM
        $MSIX_ARTIFACT = "${IDENTITY_NAME}_${GIMP_VERSION}.0_$msix_arch.msix"
        $APPXSYM = $MSIX_ARTIFACT -replace '.msix','.appxsym'

        ## Make .appxsym for each msix_arch (ONLY FOR RELEASES)
        #if ($CI_COMMIT_TAG -or ($GIMP_CI_MS_STORE -eq 'MSIXUPLOAD'))
        #  {
        #    Get-ChildItem $msix_arch -Filter *.pdb -Recurse |
        #    Compress-Archive -DestinationPath "${IDENTITY_NAME}_${GIMP_VERSION}.0_$msix_arch.zip"
        #    Get-ChildItem *.zip | Rename-Item -NewName $APPXSYM
        #    Get-ChildItem $msix_arch -Include *.pdb -Recurse -Force | Remove-Item -Recurse -Force
        #  }

        ## Make .msix from each msix_arch
        makeappx pack /d $msix_arch /p $MSIX_ARTIFACT
        Remove-Item $msix_arch/ -Recurse
      } #END of 'if (Test-Path...'
  } #END of 'foreach ($msix_arch...'


# 5. MAKE .MSIXBUNDLE AND SUBSEQUENT .MSIXUPLOAD
if ((Get-ChildItem *.msix -Recurse).Count -gt 1)
  {
    ## Make .msixbundle with all archs
    ## (This is needed not only for easier multi-arch testing but
    ##  also to make sure against Partner Center getting confused)
    $MSIX_ARTIFACT = "${IDENTITY_NAME}_${GIMP_VERSION}.0_neutral.msixbundle"
    New-Item _TempOutput -ItemType Directory
    Move-Item *.msix _TempOutput/
    makeappx bundle /bv "${GIMP_VERSION}.0" /d _TempOutput /p $MSIX_ARTIFACT
    Remove-Item _TempOutput/ -Recurse

    ## Make .msixupload (ONLY FOR RELEASES)
    if ($CI_COMMIT_TAG -or ($GIMP_CI_MS_STORE -eq 'MSIXUPLOAD'))
      {
        $MSIX_ARTIFACT = "${IDENTITY_NAME}_${GIMP_VERSION}.0_x64_arm64_bundle.msixupload"
        Get-ChildItem *.msixbundle | ForEach-Object { Compress-Archive -Path "$($_.Basename).msixbundle" -DestinationPath "$($_.Basename).zip" }
        Get-ChildItem *.zip | Rename-Item -NewName $MSIX_ARTIFACT
        #Get-ChildItem *.appxsym | Remove-Item -Recurse -Force
        Get-ChildItem *.msixbundle | Remove-Item -Recurse -Force
      }
  }


# 5. SIGN .MSIX OR .MSIXBUNDLE (FOR TESTING ONLY) AND DO OTHER STUFF
if (-not $CI_COMMIT_TAG -and ($GIMP_CI_MS_STORE -ne 'MSIXUPLOAD') -and ($MSIX_ARTIFACT -notlike "*msixupload"))
 {
  signtool sign /fd sha256 /a /f build\windows\store\pseudo-gimp.pfx /p eek $MSIX_ARTIFACT
  Copy-Item build\windows\store\pseudo-gimp.pfx .\ -Recurse
 }

if ($GITLAB_CI)
  {
    # GitLab doesn't support wildcards when using "expose_as" so let's move to a dir
    New-Item build\windows\store\_Output -ItemType Directory
    Move-Item $MSIX_ARTIFACT build\windows\store\_Output
    Get-ChildItem pseudo-gimp.pfx | Move-Item -Destination build\windows\store\_Output
  }

Remove-Item .gitignore
Rename-Item .gitignore.bak .gitignore
