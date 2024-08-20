#!/usr/bin/env pwsh

# Parameters
param ($revision = '0',
       $build_dir = '_build',
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
Write-Output "(INFO): Installed WinSDK: $win_sdk_path"

# Needed tools from Windows SDK
Set-Alias 'makepri' "$win_sdk_path\makepri.exe"
Set-Alias 'makeappx' 'C:\Program Files (x86)\Windows Kits\10\App Certification Kit\MakeAppx.exe'
Set-Alias 'signtool' "$win_sdk_path\signtool.exe"


# Global variables
$config_path = "$build_dir\config.h"
if (-not (Test-Path "$config_path"))
  {
    Write-Host "(ERROR): config.h file not found. You can run 'build/windows/2_build-gimp-msys2.sh --relocatable' or configure GIMP with 'meson setup' on some MSYS2 shell to generate it." -ForegroundColor red
    exit 1
  }

## Figure out if GIMP is unstable (dev)
$gimp_unstable = Get-Content "$config_path"                               | Select-String 'GIMP_UNSTABLE' |
                 Foreach-Object {$_ -replace '#define GIMP_UNSTABLE ',''}
if ($gimp_unstable -eq '1')
  {
    $dev = '1'
  }

$gimp_version = Get-Content "$config_path"                               | Select-String 'GIMP_VERSION'        |
                Foreach-Object {$_ -replace '#define GIMP_VERSION "',''} | Foreach-Object {$_ -replace '"',''}
if ($gimp_version -match 'RC[0-9]')
  {
    $dev = 'rc'
  }

## Get Identity Name (the dir shown in Explorer)
if ($dev)
  {
    $IDENTITY_NAME="GIMP.GIMPPreview"
  }
else
  {
    $IDENTITY_NAME="GIMP.GIMP"
  }

## Get GIMP app version (major.minor)
$GIMP_APP_VERSION = Get-Content "$config_path"                                   | Select-String 'GIMP_APP_VERSION "'  |
                    Foreach-Object {$_ -replace '#define GIMP_APP_VERSION "',''} | Foreach-Object {$_ -replace '"',''}
$major = ($GIMP_APP_VERSION.Split('.'))[0]
$minor = ($GIMP_APP_VERSION.Split('.'))[-1]

## Get GIMP micro version
$micro = ($gimp_version.Split('.'))[-1]
if ($dev -eq 'rc')
  {
    $micro = $micro -replace ".{4}$"
  }

$micro_digit = $micro
if ($micro -eq '0')
  {
    $micro_digit = ''
  }

## Get GIMP revision
if ($GIMP_CI_MS_STORE -like 'MSIXUPLOAD_*')
  {
    Write-Host "(WARNING): The revision is being made on CI, more updated deps than necessary may be packaged." -ForegroundColor yellow
    $revision = $GIMP_CI_MS_STORE -replace 'MSIXUPLOAD_',''
  }

if ($revision -ne '0')
  {
    $revision_text = ", revision: $revision"
  }

## Get custom GIMP version (major.minor.micro+revision.0), a compliant way to publish to Partner Center:
## https://learn.microsoft.com/en-us/windows/apps/publish/publish-your-app/msix/app-package-requirements)
$CUSTOM_GIMP_VERSION = "$GIMP_APP_VERSION.${micro_digit}${revision}.0"

Write-Output "(INFO): Identity: $IDENTITY_NAME | Version: $CUSTOM_GIMP_VERSION (major: $major, minor: $minor, micro: ${micro}${revision_text})"


# Autodetects what arch bundles will be packaged
if (-not (Test-Path .gitignore.bak -Type Leaf))
  {
    Copy-Item .gitignore .gitignore.bak
  }
if (-not (Test-Path "$a64_bundle") -and -not (Test-Path "$x64_bundle"))
  {
    Write-Host "(ERROR): No bundle found. You can run 'build/windows/2_build-gimp-msys2.sh --relocatable' on some MSYS2 shell to make one." -ForegroundColor red
    exit 1
  }
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

        $ig_content = "`n$msix_arch`n*.appxsym`n*.zip"
        if (Test-Path .gitignore -Type Leaf)
          {
            Add-Content .gitignore "$ig_content"
          }
        else
          {
            New-Item .gitignore
            Set-Content .gitignore "$ig_content"
          }

        Write-Output "(INFO): Arch: $msix_arch"
        if (Test-Path $msix_arch)
          {
            Remove-Item $msix_arch/ -Recurse
          }
        New-Item $msix_arch -ItemType Directory | Out-Null


        # 1. CONFIGURE MANIFEST
        Write-Output "(INFO): configuring AppxManifest.xml for $msix_arch"
        Copy-Item build\windows\store\AppxManifest.xml $msix_arch

        ## Set Identity Name
        (Get-Content $msix_arch\AppxManifest.xml) | Foreach-Object {$_ -replace "@IDENTITY_NAME@","$IDENTITY_NAME"} |
        Set-Content $msix_arch\AppxManifest.xml

        ## Set Display Name (the name shown in MS Store)
        if ($dev)
          {
            $display_name='GIMP (Preview)'
          }
        else
          {
            $display_name='GIMP'
          }
        (Get-Content $msix_arch\AppxManifest.xml) | Foreach-Object {$_ -replace "@DISPLAY_NAME@","$display_name"} |
        Set-Content $msix_arch\AppxManifest.xml

        ## Set custom GIMP version (major.minor.micro+revision.0)
        (Get-Content $msix_arch\AppxManifest.xml) | Foreach-Object {$_ -replace "@CUSTOM_GIMP_VERSION@","$CUSTOM_GIMP_VERSION"} |
        Set-Content $msix_arch\AppxManifest.xml

        ## Set GIMP app version (major.minor)
        (Get-Content $msix_arch\AppxManifest.xml) | Foreach-Object {$_ -replace "@GIMP_APP_VERSION@","$GIMP_APP_VERSION"} |
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
        $icons_path = "$build_dir\build\windows\store\Assets"
        if (-not (Test-Path "$icons_path"))
          {
            Write-Host "(ERROR): MS Store icons not found. You can run 'build/windows/2_build-gimp-msys2.sh --relocatable' or configure GIMP with '-Dms-store=true' on some MSYS2 shell to build them." -ForegroundColor red
            exit 1
          }
        Write-Output "(INFO): generating resources.pri from $icons_path"

        ## Copy pre-generated icons to each msix_arch
        New-Item $msix_arch\Assets -ItemType Directory | Out-Null
        Copy-Item "$icons_path\*.png" $msix_arch\Assets\ -Recurse

        ## Generate resources.pri
        Set-Location $msix_arch
        makepri createconfig /cf priconfig.xml /dq lang-en-US /pv 10.0.0 | Out-File ..\winsdk.log
        Set-Location ..\
        makepri new /pr $msix_arch /cf $msix_arch\priconfig.xml /of $msix_arch | Out-File winsdk.log -Append
        Remove-Item $msix_arch\priconfig.xml


        # 3. COPY GIMP FILES
        Write-Output "(INFO): copying files from $bundle bundle"
        $vfs = "$msix_arch\VFS\ProgramFilesX64\GIMP"

        ## Copy files into VFS folder (to support external 3P plug-ins)
        Copy-Item "$bundle" "$vfs" -Recurse -Force

        ## Remove uneeded files (to match the Inno Windows Installer artifact)
        Get-ChildItem "$vfs" -Recurse -Include (".gitignore", "gimp.cmd") | Remove-Item -Recurse

        ## Set revision (on GIMP about dialog)
        (Get-Content "$vfs\share\gimp\*\gimp-release") | Foreach-Object {$_ -replace "revision=0","revision=$revision"} |
        Set-Content "$vfs\share\gimp\*\gimp-release"

        ## Disable Update check (ONLY FOR RELEASES)
        if ($CI_COMMIT_TAG -or ($GIMP_CI_MS_STORE -like 'MSIXUPLOAD*'))
          {
            Add-Content "$vfs\share\gimp\*\gimp-release" 'check-update=false'
          }

        ## Remove uncompliant files (to avoid WACK/'signtool' issues)
        Get-ChildItem "$vfs" -Recurse -Include ("*.debug", "*.tar") | Remove-Item -Recurse


        # 4. MAKE .MSIX AND CORRESPONDING .APPXSYM

        ## Make .appxsym for each msix_arch (ONLY FOR RELEASES)
        $APPXSYM = "${IDENTITY_NAME}_${CUSTOM_GIMP_VERSION}_$msix_arch.appxsym"
        #if ($CI_COMMIT_TAG -or ($GIMP_CI_MS_STORE -like 'MSIXUPLOAD*'))
        #  {
        #    Get-ChildItem $msix_arch -Filter *.pdb -Recurse |
        #    Compress-Archive -DestinationPath "${IDENTITY_NAME}_${CUSTOM_GIMP_VERSION}_$msix_arch.zip"
        #    Get-ChildItem *.zip | Rename-Item -NewName $APPXSYM
        #    Get-ChildItem $msix_arch -Include *.pdb -Recurse -Force | Remove-Item -Recurse -Force
        #  }

        ## Make .msix from each msix_arch
        $MSIX_ARTIFACT = $APPXSYM -replace '.appxsym','.msix'
        if (-not ((Test-Path $a64_bundle) -and (Test-Path $x64_bundle)))
          {
            Write-Output "(INFO): packaging $MSIX_ARTIFACT (for testing purposes)"
          }
        else
          {
            Write-Output "(INFO): packaging temporary $MSIX_ARTIFACT"
          }
        makeappx pack /d $msix_arch /p $MSIX_ARTIFACT /o | Out-File winsdk.log -Append
        Remove-Item $msix_arch/ -Recurse
      } #END of 'if (Test-Path...'
  } #END of 'foreach ($msix_arch...'


# 5. MAKE .MSIXBUNDLE AND SUBSEQUENT .MSIXUPLOAD
if (((Test-Path $a64_bundle) -and (Test-Path $x64_bundle)) -and (Get-ChildItem *.msix -Recurse).Count -gt 1)
  {
    ## Make .msixbundle with all archs
    ## (This is needed not only for easier multi-arch testing but
    ##  also to make sure against Partner Center getting confused)
    $MSIX_ARTIFACT = "${IDENTITY_NAME}_${CUSTOM_GIMP_VERSION}_neutral.msixbundle"
    if (-not $CI_COMMIT_TAG -and ($GIMP_CI_MS_STORE -notlike 'MSIXUPLOAD*'))
      {
        Write-Output "(INFO): packaging $MSIX_ARTIFACT (for testing purposes)"
      }
    else
      {
        Write-Output "(INFO): packaging temporary $MSIX_ARTIFACT"
      }
    New-Item _TempOutput -ItemType Directory | Out-Null
    Move-Item *.msix _TempOutput/
    makeappx bundle /bv "${CUSTOM_GIMP_VERSION}" /d _TempOutput /p $MSIX_ARTIFACT /o | Out-File winsdk.log -Append
    Remove-Item _TempOutput/ -Recurse

    ## Make .msixupload (ONLY FOR RELEASES)
    if ($CI_COMMIT_TAG -or ($GIMP_CI_MS_STORE -like 'MSIXUPLOAD*'))
      {
        $MSIX_ARTIFACT = "${IDENTITY_NAME}_${CUSTOM_GIMP_VERSION}_x64_arm64_bundle.msixupload"
        Write-Output "(INFO): making $MSIX_ARTIFACT"
        Get-ChildItem *.msixbundle | ForEach-Object { Compress-Archive -Path "$($_.Basename).msixbundle" -DestinationPath "$($_.Basename).zip" }
        Get-ChildItem *.zip | Rename-Item -NewName $MSIX_ARTIFACT
        #Get-ChildItem *.appxsym | Remove-Item -Recurse -Force
        Get-ChildItem *.msixbundle | Remove-Item -Recurse -Force
      }
    #FIXME: .msixupload should be published automatically
    #https://gitlab.gnome.org/GNOME/gimp/-/issues/11397
  }


# 5. SIGN .MSIX OR .MSIXBUNDLE (FOR TESTING ONLY) AND DO OTHER STUFF
if (-not $CI_COMMIT_TAG -and ($GIMP_CI_MS_STORE -notlike 'MSIXUPLOAD*') -and ($MSIX_ARTIFACT -notlike "*msixupload"))
 {
  Write-Output "(INFO): signing $MSIX_ARTIFACT (for testing purposes)"
  signtool sign /fd sha256 /a /f build\windows\store\pseudo-gimp.pfx /p eek $MSIX_ARTIFACT | Out-File winsdk.log -Append
  Copy-Item build\windows\store\pseudo-gimp.pfx .\ -Recurse
 }

if ($GITLAB_CI)
  {
    # GitLab doesn't support wildcards when using "expose_as" so let's move to a dir
    New-Item build\windows\store\_Output -ItemType Directory | Out-Null
    Move-Item $MSIX_ARTIFACT build\windows\store\_Output
    if (-not $CI_COMMIT_TAG -and ($GIMP_CI_MS_STORE -notlike 'MSIXUPLOAD*'))
      {
        Get-ChildItem pseudo-gimp.pfx | Move-Item -Destination build\windows\store\_Output
      }
  }

Remove-Item .gitignore
Rename-Item .gitignore.bak .gitignore
