#!/usr/bin/env pwsh

# Parameters
param ($revision = '0',
       $GIMP_BASE = "$PWD",
       $BUILD_DIR = "$GIMP_BASE\_build",
       $GIMP32 = 'gimp-x86',
       $GIMP64 = 'gimp-x64',
       $GIMPA64 = 'gimp-a64')

# This script needs a bit of MSYS2 to work
$Env:CHERE_INVOKING = "yes"


# 1. GET INNO
Write-Output "(INFO): installing Inno"

## Download Inno
## (We need to ensure that TLS 1.2 is enabled because of some runners)
[Net.ServicePointManager]::SecurityProtocol = [Net.SecurityProtocolType]::Tls12
Invoke-WebRequest https://jrsoftware.org/download.php/is.exe -OutFile ..\is.exe

## Install or Update Inno
..\is.exe /VERYSILENT /SUPPRESSMSGBOXES /CURRENTUSER /SP- #/LOG="..\innosetup.log"
Wait-Process is
$inno_version = Get-ItemProperty (Resolve-Path Registry::'HKEY_CURRENT_USER\Software\Microsoft\Windows\CurrentVersion\Uninstall\Inno Setup*') | Select-Object -ExpandProperty DisplayVersion
#$inno_version = (Get-Item ..\is.exe).VersionInfo.ProductVersion
Remove-Item ..\is.exe
Write-Output "(INFO): Installed Inno: $inno_version"

## Get Inno install path
$INNO_PATH = Get-ItemProperty (Resolve-Path Registry::'HKEY_CURRENT_USER\Software\Microsoft\Windows\CurrentVersion\Uninstall\Inno Setup*') | Select-Object -ExpandProperty InstallLocation
Set-Alias iscc "$INNO_PATH\iscc.exe"

## Get Inno install path (fallback)
#$log = Get-Content ..\innosetup.log | Select-String ISCC.exe
#pattern = '(?<=filename: ).+?(?=\\ISCC.exe)'
#$INNO_PATH = [regex]::Matches($log, $pattern).Value


# 2. GET GLOBAL INFO
$CONFIG_PATH = "$BUILD_DIR\config.h"
if (-not (Test-Path "$CONFIG_PATH"))
  {
    Write-Host "(ERROR): config.h file not found. You can run 'build/windows/2_build-gimp-msys2.sh --relocatable' or configure GIMP with 'meson setup' on some MSYS2 shell to generate it.'" -ForegroundColor red
    exit 1
  }

## Get AppVer (GIMP version as we use on Inno)
$gimp_version = Get-Content "$CONFIG_PATH"                               | Select-String 'GIMP_VERSION'        |
                Foreach-Object {$_ -replace '#define GIMP_VERSION "',''} | Foreach-Object {$_ -replace '"',''}
$APPVER = $gimp_version

if ($GIMP_CI_WIN_INSTALLER -and $GIMP_CI_WIN_INSTALLER -match '[0-9]')
  {
    Write-Host "(WARNING): The revision is being made on CI, more updated deps than necessary may be packaged." -ForegroundColor yellow
    $revision = $GIMP_CI_WIN_INSTALLER
  }

if ($revision -ne '0')
  {
    $APPVER = "$gimp_version.$revision"
  }

Write-Output "(INFO): GIMP version: $APPVER"

## FIXME: Our Inno scripts can't construct an one-arch installer
if (-not (Test-Path "$GIMP32"))
  {
    Write-Host "(ERROR): $GIMP32 bundle not found. You need all the three archs bundles to make the installer." -ForegroundColor red
  }
if (-not (Test-Path "$GIMP64"))
  {
    Write-Host "(ERROR): $GIMP64 bundle not found. You need all the three archs bundles to make the installer." -ForegroundColor red
  }
if (-not (Test-Path "$GIMPA64"))
  {
    Write-Host "(ERROR): $GIMPA64 bundle not found. You need all the three archs bundles to make the installer." -ForegroundColor red
  }
if ((-not (Test-Path "$GIMP32")) -or (-not (Test-Path "$GIMP64")) -or (-not (Test-Path "$GIMPA64")))
  {
    exit 1
  }

Write-Output "(INFO): Arch: universal (x86, x64 and arm64)"


# 3. PREPARE INSTALLER "SOURCE"
if (-not (Test-Path "$BUILD_DIR\build\windows\installer"))
  {
    Write-Host "(ERROR): Installer assets not found. You can run 'build/windows/2_build-gimp-msys2.sh --relocatable' or configure GIMP with '-Dwindows-installer=true' on some MSYS2 shell to build them." -ForegroundColor red
    exit 1
  }

## Download Official translations not present in a Inno release yet
#Write-Output "(INFO): downloading Official Inno lang files (not present in a release yet)"
#function download_lang_official ([string]$langfile)
#{
#  Invoke-WebRequest -URI "https://raw.githubusercontent.com/jrsoftware/issrc/main/Files/Languages/${langfile}" -OutFile "$INNO_PATH/Languages/${langfile}"
#}

## Download unofficial translations (of unknown quality and maintenance)
## Cf. https://jrsoftware.org/files/istrans/
Write-Output "(INFO): downloading unofficial Inno lang files"
New-Item "$INNO_PATH/Languages/Unofficial/" -ItemType Directory -Force | Out-Null

$xmlObject = New-Object XML
$xmlObject.Load("$PWD\build\windows\installer\lang\iso_639_custom.xml")
$langsArray = $xmlObject.iso_639_entries.iso_639_entry |
              Select-Object -ExpandProperty inno_code  | Where-Object { $_ -like "*Unofficial*" }
foreach ($langfile in $langsArray)
  {
    $langfileUnix = $langfile.Replace('\\', '/')
    Invoke-WebRequest https://raw.githubusercontent.com/jrsoftware/issrc/main/Files/$langfileUnix -OutFile "$INNO_PATH/$langfileUnix"
  }

## Patch 'AppVer*' against Inno pervasive behavior: https://groups.google.com/g/innosetup/c/w0sebw5YAeg
Write-Output "(INFO): patching Official and unofficial Inno lang files with $APPVER"
function fix_msg ([string]$langsdir)
{
  #Prefer MSYS2 since PowerShell/.NET doesn't handle well files with mixed encodings
  Copy-Item $GIMP_BASE/build/windows/installer/lang/fix_msg.sh $langsdir
  Set-Location $langsdir
  (Get-Content fix_msg.sh) | Foreach-Object {$_ -replace "AppVer","$APPVER"} |
  Set-Content fix_msg.sh
  C:\msys64\usr\bin\bash -lc "bash fix_msg.sh"
  Remove-Item fix_msg.sh
  Set-Location $GIMP_BASE

  #$langsArray_local = Get-ChildItem *.isl -Name
  #foreach ($langfile in $langsArray_local)
    #{
    #  $msg = Get-Content $langfile
    #  $linenumber = $msg | Select-String 'SetupWindowTitle' | Select-Object -ExpandProperty LineNumber
    #  $msg | ForEach-Object { If ($_.ReadCount -eq $linenumber) {$_ -Replace "%1", "%1 $AppVer"} Else {$_} } |
    #         Set-Content "$langfile" -Encoding UTF8
    #  $msg = Get-Content $langfile
    #  $linenumber = $msg | Select-String 'UninstallAppFullTitle' | Select-Object -ExpandProperty LineNumber
    #  $msg | ForEach-Object { If ($_.ReadCount -eq $linenumber) {$_ -Replace "%1", "%1 $AppVer"} Else {$_} } |
    #         Set-Content "$langfile" -Encoding UTF8
    #}
}

fix_msg $INNO_PATH
fix_msg $INNO_PATH\Languages
fix_msg $INNO_PATH\Languages\Unofficial


# 4. PREPARE GIMP FILES

## GIMP revision on about dialog
## FIXME: This should be done with Inno scripting
if ($GITLAB_CI)
  {
    $supported_archs = "$GIMP32","$GIMP64","$GIMPA64"
    foreach ($bundle in $supported_archs)
      {
        (Get-Content "$bundle\share\gimp\*\gimp-release") | Foreach-Object {$_ -replace "revision=0","revision=$revision"} |
        Set-Content "$bundle\share\gimp\*\gimp-release"
      }
  }

## FIXME: We can't do this on CI
if (-not $GITLAB_CI)
  {
    Write-Output "(INFO): extracting .debug symbols from bundles"

    #$Env:MSYSTEM = "MINGW32"
    #C:\msys64\usr\bin\bash -lc 'bash build/windows/installer/3_dist-gimp-inno_sym.sh' | Out-Null

    if ($Env:PROCESSOR_ARCHITECTURE -eq 'ARM64')
      {
        $Env:MSYSTEM = "CLANGARM64"
      }
    else
      {
        $Env:MSYSTEM = "CLANG64"
      }
    C:\msys64\usr\bin\bash -lc 'bash build/windows/installer/3_dist-gimp-inno_sym.sh' | Out-Null
  }


# 5. CONSTRUCT .EXE INSTALLER
$INSTALLER="gimp-${APPVER}-setup.exe"
Write-Output "(INFO): constructing $INSTALLER installer"

## Get GIMP versions used in some versioned files and dirs
$gimp_app_version = Get-Content "$CONFIG_PATH"                                   | Select-String 'GIMP_APP_VERSION "'  |
                    Foreach-Object {$_ -replace '#define GIMP_APP_VERSION "',''} | Foreach-Object {$_ -replace '"',''}

$gimp_api_version = Get-Content "$CONFIG_PATH"                                         | Select-String 'GIMP_PKGCONFIG_VERSION' |
                    Foreach-Object {$_ -replace '#define GIMP_PKGCONFIG_VERSION "',''} | Foreach-Object {$_ -replace '"',''}

## Compile installer
Set-Location build\windows\installer
iscc -DGIMP_VERSION="$gimp_version" -DREVISION="$revision" -DGIMP_APP_VERSION="$gimp_app_version" -DGIMP_API_VERSION="$gimp_api_version" -DBUILD_DIR="$BUILD_DIR" -DGIMP_DIR="$GIMP_BASE" -DDIR32="$GIMP32" -DDIR64="$GIMP64" -DDIRA64="$GIMPA64" -DDEPS_DIR="$GIMP_BASE" -DDDIR32="$GIMP32" -DDDIR64="$GIMP64" -DDDIRA64="$GIMPA64" -DDEBUG_SYMBOLS -DLUA -DPYTHON base_gimp3264.iss | Out-Null

## Test if the installer was created and return success/failure.
if (Test-Path "$GIMP_BASE\$INSTALLER" -PathType Leaf)
  {
    if ($GITLAB_CI)
      {
        New-Item _Output -ItemType Directory | Out-Null
        Move-Item $GIMP_BASE\$INSTALLER _Output

        if ($CI_COMMIT_TAG)
          {
            Write-Output "(INFO): generating checksums for $INSTALLER"
            Get-FileHash _Output\$INSTALLER -Algorithm SHA256 | Out-File _Output\$INSTALLER.SHA256SUMS
            Get-FileHash _Output\$INSTALLER -Algorithm SHA512 | Out-File _Output\$INSTALLER.SHA512SUMS
          }
      }

    # Return to git golder
    Set-Location $GIMP_BASE
  }
else
  {
    # Return to git golder
    Set-Location $GIMP_BASE

    exit 1
  }
