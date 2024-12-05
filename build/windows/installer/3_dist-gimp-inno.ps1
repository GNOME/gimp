#!/usr/bin/env pwsh

# Parameters
param ($revision = "$GIMP_CI_WIN_INSTALLER",
       $GIMP_BASE = "$PWD",
       $BUILD_DIR = "$GIMP_BASE\_build",
       $GIMP32 = 'gimp-x86',
       $GIMP64 = 'gimp-x64',
       $GIMPA64 = 'gimp-a64')

$ErrorActionPreference = 'Stop'
$PSNativeCommandUseErrorActionPreference = $true


# This script needs a bit of MSYS2 to work
Invoke-Expression ((Get-Content build\windows\1_build-deps-msys2.ps1 | Select-String 'MSYS2_PREFIX =' -Context 0,15) -replace '> ','')


# 1. GET INNO

## Download Inno
Write-Output '(INFO): checking Inno version'
## (We need to ensure that TLS 1.2 is enabled because of some runners)
[Net.ServicePointManager]::SecurityProtocol = [Net.SecurityProtocolType]::Tls12
Invoke-WebRequest https://jrsoftware.org/download.php/is.exe -OutFile ..\is.exe
$inno_version_downloaded = (Get-Item ..\is.exe).VersionInfo.ProductVersion -replace ' ',''

## Install or Update Inno
$broken_inno = Get-ChildItem $Env:Tmp -Filter *.isl.bak -ErrorAction SilentlyContinue
$inno_version = Get-ItemProperty Registry::'HKEY_CURRENT_USER\Software\Microsoft\Windows\CurrentVersion\Uninstall\Inno Setup*' -ErrorAction SilentlyContinue | Select-Object -ExpandProperty DisplayVersion
if ("$broken_inno" -or "$inno_version" -ne "$inno_version_downloaded")
  {
    if ("$broken_inno")
      {
        Write-Output '(INFO): repairing Inno'
        $broken_inno | Remove-Item -Recurse
      }
    elseif ("$inno_version" -notlike "*.*")
      {
        Write-Output '(INFO): installing Inno'
      }
    else
      {
        Write-Output "(INFO): updating Inno from $inno_version to $inno_version_downloaded"
      }
    ..\is.exe /VERYSILENT /SUPPRESSMSGBOXES /CURRENTUSER /SP- #/LOG="..\innosetup.log"
    Wait-Process is
  }
Remove-Item ..\is.exe
Write-Output "(INFO): Installed Inno: $inno_version_downloaded"

## Get Inno install path
$INNO_PATH = Get-ItemProperty (Resolve-Path Registry::'HKEY_CURRENT_USER\Software\Microsoft\Windows\CurrentVersion\Uninstall\Inno Setup*') | Select-Object -ExpandProperty InstallLocation
#$INNO_PATH = [regex]::Matches((Get-Content ..\innosetup.log | Select-String ISCC.exe), '(?<=filename: ).+?(?=\\ISCC.exe)').Value
Set-Alias iscc "$INNO_PATH\iscc.exe"


# 2. GET GLOBAL INFO
$CONFIG_PATH = "$BUILD_DIR\config.h"
if (-not (Test-Path "$CONFIG_PATH"))
  {
    Write-Host "(ERROR): config.h file not found. You can run 'build\windows\2_build-gimp-msys2.ps1' or configure GIMP to generate it.'" -ForegroundColor red
    exit 1
  }

## Get CUSTOM_GIMP_VERSION (GIMP version as we display for users in installer)
$CUSTOM_GIMP_VERSION = Get-Content "$CONFIG_PATH"                               | Select-String 'GIMP_VERSION'        |
                       Foreach-Object {$_ -replace '#define GIMP_VERSION "',''} | Foreach-Object {$_ -replace '"',''}
if ($revision -notmatch '[1-9]' -or $CI_PIPELINE_SOURCE -eq 'schedule')
  {
    $revision = '0'
  }
else
  {
    $CUSTOM_GIMP_VERSION = "$CUSTOM_GIMP_VERSION.$revision"
  }
Write-Output "(INFO): GIMP version: $CUSTOM_GIMP_VERSION"

## FIXME: Our Inno scripts can't construct an one-arch installer
$supported_archs = "$GIMP32","$GIMP64","$GIMPA64"
foreach ($bundle in $supported_archs)
  {
    if (-not (Test-Path "$bundle"))
      {
        Write-Host "(ERROR): $bundle bundle not found. You need all the three archs bundles to make the installer." -ForegroundColor red
      }
  }
if ((-not (Test-Path "$GIMP32")) -or (-not (Test-Path "$GIMP64")) -or (-not (Test-Path "$GIMPA64")))
  {
    exit 1
  }
Write-Output "(INFO): Arch: universal (x86, x64 and arm64)"


# 3. PREPARE INSTALLER "SOURCE"

## Custom installer strings translations and other assets
## (They are loaded with '-DBUILD_DIR')
if (-not (Test-Path "$BUILD_DIR\build\windows\installer"))
  {
    Write-Host "(ERROR): Installer assets not found. You can tweak 'build\windows\2_build-gimp-msys2.ps1' or configure GIMP with '-Dwindows-installer=true' to build them." -ForegroundColor red
    exit 1
  }

## Complete Inno source with not released translations
## Cf. https://jrsoftware.org/files/istrans/
Write-Output "(INFO): temporarily installing additional Inno lang files"
$xmlObject = New-Object XML
$xmlObject.Load("$PWD\build\windows\installer\lang\iso_639_custom.xml")
function download_langs ([array]$langsArray)
{
  foreach ($langfile in $langsArray)
    {
      if ($langfile -ne '' -and -not (Test-Path "$INNO_PATH\$langfile" -Type Leaf))
        {
          $langfileUnix = $langfile.Replace('\\', '/')
          Invoke-WebRequest https://raw.githubusercontent.com/jrsoftware/issrc/main/Files/$langfileUnix -OutFile "$INNO_PATH\$langfile"
        }
    }
}
### Official translations not present in a Inno release yet
$langsArray_Official = $xmlObject.iso_639_entries.iso_639_entry | Select-Object -ExpandProperty inno_code     |
                       Where-Object { $_ -like "*Languages*" }  | Where-Object { $_ -notlike "*Unofficial*" }
download_langs $langsArray_Official
### unofficial translations (of unknown quality and maintenance)
New-Item "$INNO_PATH\Languages\Unofficial" -ItemType Directory -Force | Out-Null
$langsArray_unofficial = $xmlObject.iso_639_entries.iso_639_entry | Select-Object -ExpandProperty inno_code |
                         Where-Object { $_ -like "*Unofficial*" }
download_langs $langsArray_unofficial

## Patch 'AppVer*' against Inno pervasive behavior: https://groups.google.com/g/innosetup/c/w0sebw5YAeg
Write-Output "(INFO): temporarily patching all Inno lang files with $CUSTOM_GIMP_VERSION"
function fix_msg ([string]$langsdir, [string]$AppVer)
{
  $langsArray_local = Get-ChildItem $langsdir -Filter *.isl -Name
  foreach ($langfile in $langsArray_local)
    {
      $langfilePath = "$langsdir\$langfile"

      if ($AppVer -ne 'revert')
        {
          Copy-Item "$langfilePath" "$Env:Tmp\$langfile.bak" -Force

          #Prefer MSYS2 since PowerShell/.NET doesn't handle well files with mixed encodings
          $langfilePathUnix = "$langfilePath" -replace '\\','/' -replace '//','/'
          bash build/windows/installer/lang/fix_msg.sh "$langfilePathUnix" $AppVer

          #$msg = Get-Content $langfilePath
          #$linenumber = $msg | Select-String 'SetupWindowTitle' | Select-Object -ExpandProperty LineNumber
          #$msg | ForEach-Object { If ($_.ReadCount -eq $linenumber) {$_ -Replace "%1", "%1 $AppVer"} Else {$_} } |
          #       Set-Content "$langfilePath" -Encoding UTF8
          #$msg = Get-Content $langfilePath
          #$linenumber = $msg | Select-String 'UninstallAppFullTitle' | Select-Object -ExpandProperty LineNumber
          #$msg | ForEach-Object { If ($_.ReadCount -eq $linenumber) {$_ -Replace "%1", "%1 $AppVer"} Else {$_} } |
          #       Set-Content "$langfilePath" -Encoding UTF8
        }

      else #($AppVer -eq 'revert')
        {
          Move-Item "$Env:Tmp\$langfile.bak" "$langfilePath" -Force
        }
    }
}
fix_msg "$INNO_PATH" $CUSTOM_GIMP_VERSION
fix_msg "$INNO_PATH\Languages" $CUSTOM_GIMP_VERSION
fix_msg "$INNO_PATH\Languages\Unofficial" $CUSTOM_GIMP_VERSION


# 4. PREPARE GIMP FILES

## Get GIMP versions used in some versioned files and dirs
$gimp_version = Get-Content "$CONFIG_PATH"                               | Select-String 'GIMP_VERSION'        |
                Foreach-Object {$_ -replace '#define GIMP_VERSION "',''} | Foreach-Object {$_ -replace '"',''} |
                Foreach-Object {$_ -replace '(.+?)-.+','$1'}
$gimp_app_version = Get-Content "$CONFIG_PATH"                                   | Select-String 'GIMP_APP_VERSION "'  |
                    Foreach-Object {$_ -replace '#define GIMP_APP_VERSION "',''} | Foreach-Object {$_ -replace '"',''}
$gimp_api_version = Get-Content "$CONFIG_PATH"                                         | Select-String 'GIMP_PKGCONFIG_VERSION' |
                    Foreach-Object {$_ -replace '#define GIMP_PKGCONFIG_VERSION "',''} | Foreach-Object {$_ -replace '"',''}

## GIMP revision on about dialog (this does the same as '-Drevision' build option)
## FIXME: This should be done with Inno scripting
foreach ($bundle in $supported_archs)
  {
    (Get-Content "$bundle\share\gimp\*\gimp-release") | Foreach-Object {$_ -replace "revision=0","revision=$revision"} |
    Set-Content "$bundle\share\gimp\*\gimp-release"
  }

## Split .debug symbols
Write-Output "(INFO): extracting .debug symbols from bundles"
bash build/windows/installer/3_dist-gimp-inno_sym.sh | Out-Null


# 5. CONSTRUCT .EXE INSTALLER
$INSTALLER="gimp-${CUSTOM_GIMP_VERSION}-setup.exe"
Write-Output "(INFO): constructing $INSTALLER installer"

## Compile installer
Set-Location build\windows\installer
if ($CUSTOM_GIMP_VERSION -match 'RC[1-9]')
  {
    $devel_warning='-DDEVEL_WARNING'
  }
iscc -DCUSTOM_GIMP_VERSION="$CUSTOM_GIMP_VERSION" -DGIMP_VERSION="$gimp_version" -DREVISION="$revision" -DGIMP_APP_VERSION="$gimp_app_version" -DGIMP_API_VERSION="$gimp_api_version" -DBUILD_DIR="$BUILD_DIR" -DGIMP_DIR="$GIMP_BASE" -DDIR32="$GIMP32" -DDIR64="$GIMP64" -DDIRA64="$GIMPA64" -DDEPS_DIR="$GIMP_BASE" -DDDIR32="$GIMP32" -DDDIR64="$GIMP64" -DDDIRA64="$GIMPA64" -DDEBUG_SYMBOLS -DPYTHON $devel_warning base_gimp3264.iss | Out-Null
Set-Location $GIMP_BASE

## Clean changes in the bundles and Inno installation
foreach ($bundle in $supported_archs)
  {
    (Get-Content "$bundle\share\gimp\*\gimp-release") | Foreach-Object {$_ -replace "revision=$revision","revision=0"} |
    Set-Content "$bundle\share\gimp\*\gimp-release"
  }
fix_msg "$INNO_PATH" revert
fix_msg "$INNO_PATH\Languages" revert
fix_msg "$INNO_PATH\Languages\Unofficial" revert
### We delete only unofficial langs because the downloaded official ones will be kept by Inno updates
Remove-Item "$INNO_PATH\Languages\Unofficial" -Recurse -Force


if ($GITLAB_CI)
  {
    # GitLab doesn't support wildcards when using "expose_as" so let's move to a dir
    $output_dir = "$GIMP_BASE\build\windows\installer\_Output"
    New-Item $output_dir -ItemType Directory | Out-Null
    Move-Item $GIMP_BASE\$INSTALLER $output_dir

    # Generate checksums in common "sha*sum" format
    if ($CI_COMMIT_TAG)
      {
        Write-Output "(INFO): generating checksums for $INSTALLER"
        # (We use .NET directly because 'sha*sum' does NOT support BOM from pre-PS6 'Set-Content')
        $Utf8NoBomEncoding = New-Object -TypeName System.Text.UTF8Encoding -ArgumentList $False
        [System.IO.File]::WriteAllText("$output_dir\$INSTALLER.SHA256SUMS", "$((Get-FileHash $output_dir\$INSTALLER -Algorithm SHA256 | Select-Object -ExpandProperty Hash).ToLower()) *$INSTALLER", $Utf8NoBomEncoding)
        #Set-Content $output_dir\$INSTALLER.SHA256SUMS "$((Get-FileHash $output_dir\$INSTALLER -Algorithm SHA256 | Select-Object -ExpandProperty Hash).ToLower()) *$INSTALLER" -Encoding utf8NoBOM -NoNewline
        [System.IO.File]::WriteAllText("$output_dir\$INSTALLER.SHA512SUMS", "$((Get-FileHash $output_dir\$INSTALLER -Algorithm SHA512 | Select-Object -ExpandProperty Hash).ToLower()) *$INSTALLER", $Utf8NoBomEncoding)
        #Set-Content $output_dir\$INSTALLER.SHA512SUMS "$((Get-FileHash $output_dir\$INSTALLER -Algorithm SHA512 | Select-Object -ExpandProperty Hash).ToLower()) *$INSTALLER" -Encoding utf8NoBOM -NoNewline
      }
  }
