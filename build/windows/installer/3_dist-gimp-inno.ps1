#!/usr/bin/env pwsh

# Parameters
param ($revision = "$GIMP_CI_WIN_INSTALLER",
       $BUILD_DIR,
       $GIMP_BASE = "$PWD",
       $GIMP32 = 'gimp-mingw32',
       $GIMP64 = 'gimp-clang64',
       $GIMPA64 = 'gimp-clangarm64')

# Ensure the script work properly
$ErrorActionPreference = 'Stop'
$PSNativeCommandUseErrorActionPreference = $false #to ensure error catching as in pre-7.4 PS
if (-not (Test-Path build\windows\installer) -and -not (Test-Path 3_dist-gimp-inno.ps1 -Type Leaf) -or $PSScriptRoot -notlike "*build\windows\installer*")
  {
    Write-Host '(ERROR): Script called from wrong dir. Please, call the script from gimp source.' -ForegroundColor Red
    exit 1
  }
elseif (Test-Path 3_dist-gimp-inno.ps1 -Type Leaf)
  {
    Set-Location ..\..\..
  }
if (-not $GITLAB_CI)
  {
    $PARENT_DIR = '..\'
  }


# This script needs a bit of Python to work
#FIXME: Restore the condition when TWAIN 32-bit support is dropped
#if (-not (Get-Command "python" -ErrorAction SilentlyContinue) -or "$(Get-Command "python" -ErrorAction SilentlyContinue | Select-Object -ExpandProperty Source)" -like '*WindowsApps*')
#  {
    Invoke-Expression ((Get-Content build\windows\1_build-deps-msys2.ps1 | Select-String 'MSYS_ROOT\)' -Context 0,12) -replace '> ','')
    $env:PATH = "$env:MSYS_ROOT/$env:MSYSTEM_PREFIX/bin;$env:PATH"
#  }


# 1. GET INNO
Write-Output "$([char]27)[0Ksection_start:$(Get-Date -UFormat %s -Millisecond 0):installer_tlkt$([char]13)$([char]27)[0KChecking Inno installation"

## Download Inno
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
        Remove-Item "$(Get-ItemProperty Registry::'HKEY_CURRENT_USER\Software\Microsoft\Windows\CurrentVersion\Uninstall\Inno Setup*' -ErrorAction SilentlyContinue | Select-Object -ExpandProperty InstallLocation)Languages" -Recurse -Force -ErrorAction SilentlyContinue
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
Write-Output "$([char]27)[0Ksection_end:$(Get-Date -UFormat %s -Millisecond 0):installer_tlkt$([char]13)$([char]27)[0K"


# 2. GET GLOBAL INFO
Write-Output "$([char]27)[0Ksection_start:$(Get-Date -UFormat %s -Millisecond 0):installer_info$([char]13)$([char]27)[0KGetting installer global info"
if (-not $BUILD_DIR)
  {
    $BUILD_DIR = Get-ChildItem _build* | Select-Object -First 1
  }
$CONFIG_PATH = "$BUILD_DIR\config.h"
if (-not (Test-Path "$CONFIG_PATH"))
  {
    Write-Host "(ERROR): config.h file not found. You can run 'build\windows\2_build-gimp-msys2.ps1' or configure GIMP to generate it.'" -ForegroundColor red
    exit 1
  }
ForEach ($line in $(Select-String 'define' $CONFIG_PATH -AllMatches))
  {
    Invoke-Expression $($line -replace '^.*#' -replace 'define ','$' -replace ' ','=')
  }
#Meson don't support C++ style comments. See: https://github.com/mesonbuild/meson/issues/14260
$CLEANCONFIG_PATH ="$(Split-Path -Parent $CONFIG_PATH)\config_clean.h"
if (Test-Path $CLEANCONFIG_PATH)
  {
    Remove-Item $CLEANCONFIG_PATH -Force
  }
(Get-Content $CONFIG_PATH -Raw) -replace '/\*[\s\S]*?\*/', '' | Set-Content $CLEANCONFIG_PATH

## Get CUSTOM_GIMP_VERSION (GIMP version as we display for users in installer)
$CUSTOM_GIMP_VERSION = $GIMP_VERSION
if ($revision -notmatch '[1-9]' -or $CI_PIPELINE_SOURCE -eq 'schedule')
  {
    $revision = '0'
  }
else
  {
    $CUSTOM_GIMP_VERSION = "$CUSTOM_GIMP_VERSION-$revision"
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
Write-Output "$([char]27)[0Ksection_end:$(Get-Date -UFormat %s -Millisecond 0):installer_info$([char]13)$([char]27)[0K"


# 3. PREPARE INSTALLER "SOURCE"
Write-Output "$([char]27)[0Ksection_start:$(Get-Date -UFormat %s -Millisecond 0):installer_source[collapsed=true]$([char]13)$([char]27)[0KMaking installer assets"

## Custom installer strings translations and other assets
## (They are loaded with '-DBUILD_DIR')
if (-not (Test-Path "$BUILD_DIR\build\windows\installer"))
  {
    Write-Host "(ERROR): Installer assets not found. You can tweak 'build\windows\2_build-gimp-msys2.ps1' or configure GIMP with '-Dwindows-installer=true' to build them." -ForegroundColor red
    exit 1
  }

## Inno installer strings translations
## NOTE: All the maintenance process is done only in 'iso_639_custom.xml' file
$xmlObject = New-Object XML
$xmlObject.Load("$PWD\build\windows\installer\lang\iso_639_custom.xml")
$langsArray_Official = $xmlObject.iso_639_entries.iso_639_entry | Select-Object -ExpandProperty inno_code     |
                       Where-Object { $_ -like "*Languages*" }  | Where-Object { $_ -notlike "*Unofficial*" }
$langsArray_unofficial = $xmlObject.iso_639_entries.iso_639_entry | Select-Object -ExpandProperty inno_code |
                         Where-Object { $_ -like "*Unofficial*" }
### Complete Inno source with not released translations: https://jrsoftware.org/files/istrans/
function download_langs ([array]$langsArray)
{
  foreach ($langfile in $langsArray)
    {
      $langfilePath = "$INNO_PATH\$langfile"
      if ($langfile -ne '' -and -not (Test-Path "$langfilePath" -Type Leaf))
        {
          Write-Output "(INFO): temporarily installing $($langfilePath -replace '\\\\','\')"
          Copy-Item "${PARENT_DIR}issrc\Files\$langfile" "$langfilePath" -Force
        }
    }
}
git clone --depth 1 https://github.com/jrsoftware/issrc.git "${PARENT_DIR}issrc"
download_langs $langsArray_Official
New-Item "$INNO_PATH\Languages\Unofficial" -ItemType Directory -Force | Out-Null
download_langs $langsArray_unofficial
Remove-Item "${PARENT_DIR}issrc" -Recurse -Force
### Patch 'AppVer*' against Inno pervasive behavior: https://groups.google.com/g/innosetup/c/w0sebw5YAeg
function fix_msg ([array]$langsArray, [string]$AppVer)
{
  foreach ($langfile in $langsArray)
    {
      $langfilePath = "$INNO_PATH\$langfile" -replace '\\\\','\'

      if ($AppVer -ne 'revert')
        {
          Copy-Item "$langfilePath" "$Env:Tmp\$(Split-Path $langfile -Leaf).bak" -Force

          #Prefer Python since PowerShell/.NET doesn't handle well files with different encodings
          python build\windows\installer\lang\fix_msg.py "$langfilePath" $AppVer
        }

      else #($AppVer -eq 'revert')
        {
          Move-Item "$Env:Tmp\$(Split-Path $langfile -Leaf).bak" "$langfilePath" -Force
        }
    }
}
fix_msg 'Default.isl' $CUSTOM_GIMP_VERSION
fix_msg $langsArray_Official $CUSTOM_GIMP_VERSION
fix_msg $langsArray_unofficial $CUSTOM_GIMP_VERSION
Write-Output "$([char]27)[0Ksection_end:$(Get-Date -UFormat %s -Millisecond 0):installer_source$([char]13)$([char]27)[0K"


# 4. PREPARE GIMP FILES
# (Most of the file processing and special-casing is done on *gimp3264.iss [Files] section)

## Get 32-bit TWAIN deps list
Write-Output "$([char]27)[0Ksection_start:$(Get-Date -UFormat %s -Millisecond 0):installer_files[collapsed=true]$([char]13)$([char]27)[0KGenerating 32-bit TWAIN dependencies list"
$twain_list_file = 'build\windows\installer\base_twain32on64.list'
Copy-Item $twain_list_file "$twain_list_file.bak"
$twain_list = (python build\windows\2_bundle-gimp-uni_dep.py --debug debug-only $(Resolve-Path $GIMP32/lib/gimp/*/plug-ins/twain/twain.exe) $MSYS_ROOT/mingw32/ $GIMP32/ 32 |
              Select-String 'Installed' -CaseSensitive -Context 0,1000) -replace "  `t- ",'bin\'
(Get-Content $twain_list_file) | Foreach-Object {$_ -replace "@DEPS_GENLIST@","$twain_list"} | Set-Content $twain_list_file
(Get-Content $twain_list_file) | Select-string 'Installed' -notmatch | Set-Content $twain_list_file
Write-Output "$([char]27)[0Ksection_end:$(Get-Date -UFormat %s -Millisecond 0):installer_files$([char]13)$([char]27)[0K"


# 5. COMPILE .EXE INSTALLER
$INSTALLER="gimp-${CUSTOM_GIMP_VERSION}-setup.exe"
Write-Output "$([char]27)[0Ksection_start:$(Get-Date -UFormat %s -Millisecond 0):installer_making[collapsed=true]$([char]13)$([char]27)[0KConstructing $INSTALLER installer"
Set-Location build\windows\installer
iscc -DREVISION="$revision" -DBUILD_DIR="$BUILD_DIR" -DGIMP_DIR="$GIMP_BASE" -DDIR32="$GIMP32" -DDIR64="$GIMP64" -DDIRA64="$GIMPA64" -DDEPS_DIR="$GIMP_BASE" -DDDIR32="$GIMP32" -DDDIR64="$GIMP64" -DDDIRA64="$GIMPA64" -DDEBUG_SYMBOLS -DPYTHON base_gimp3264.iss | Out-Null; if ("$LASTEXITCODE" -gt '0') { exit 1 }
Set-Location $GIMP_BASE
Write-Output "$([char]27)[0Ksection_end:$(Get-Date -UFormat %s -Millisecond 0):installer_making$([char]13)$([char]27)[0K"

## Revert change done in TWAIN list
Remove-Item $twain_list_file
Move-Item "$twain_list_file.bak" $twain_list_file
## Clean changes in Inno installation
fix_msg 'Default.isl' revert
fix_msg $langsArray_Official revert
fix_msg $langsArray_unofficial revert
## We delete only unofficial langs because the downloaded official ones will be kept by Inno updates
Remove-Item "$INNO_PATH\Languages\Unofficial" -Recurse -Force


# 6. GENERATE CHECKSUMS IN GNU FORMAT
Write-Output "$([char]27)[0Ksection_start:$(Get-Date -UFormat %s -Millisecond 0):installer_trust[collapsed=true]$([char]13)$([char]27)[0KChecksumming $INSTALLER"
## (We use .NET directly because 'sha*sum' does NOT support BOM from pre-PS6 'Set-Content')
$Utf8NoBomEncoding = New-Object -TypeName System.Text.UTF8Encoding -ArgumentList $False
$sha256 = (Get-FileHash $INSTALLER -Algorithm SHA256 | Select-Object -ExpandProperty Hash).ToLower()
Write-Output "(INFO): $INSTALLER SHA-256: $sha256"
if ($GIMP_RELEASE -and -not $GIMP_IS_RC_GIT)
  {
    [System.IO.File]::WriteAllText("$GIMP_BASE\$INSTALLER.SHA256SUMS", "$sha256 *$INSTALLER", $Utf8NoBomEncoding)
    #Set-Content $INSTALLER.SHA256SUMS "$sha256 *$INSTALLER" -Encoding utf8NoBOM -NoNewline
  }
$sha512 = (Get-FileHash $INSTALLER -Algorithm SHA512 | Select-Object -ExpandProperty Hash).ToLower()
Write-Output "(INFO): $INSTALLER SHA-512: $sha512"
if ($GIMP_RELEASE -and -not $GIMP_IS_RC_GIT)
  {
    [System.IO.File]::WriteAllText("$GIMP_BASE\$INSTALLER.SHA512SUMS", "$sha512 *$INSTALLER", $Utf8NoBomEncoding)
    #Set-Content $INSTALLER.SHA512SUMS "$sha512 *$INSTALLER" -Encoding utf8NoBOM -NoNewline
  }
Write-Output "$([char]27)[0Ksection_end:$(Get-Date -UFormat %s -Millisecond 0):installer_trust$([char]13)$([char]27)[0K"


if ($GITLAB_CI)
  {
    # GitLab doesn't support wildcards when using "expose_as" so let's move to a dir
    $output_dir = "$GIMP_BASE\build\windows\installer\_Output"
    New-Item $output_dir -ItemType Directory | Out-Null
    Move-Item $GIMP_BASE\$INSTALLER* $output_dir
  }
