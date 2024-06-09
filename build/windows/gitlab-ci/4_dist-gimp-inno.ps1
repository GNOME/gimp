#!/usr/bin/env pwsh

# Parameters
param ($GIMP_VERSION,
       $REVISION,
       $GIMP_APP_VERSION,
       $GIMP_API_VERSION,
       $GIMP_BASE = '..\..\..\',
       $GIMP32 = 'gimp-x86',
       $GIMP64 = 'gimp-x64',
       $GIMPA64 = 'gimp-a64')

if ((-Not $GIMP_VERSION) -or (-Not $GIMP_APP_VERSION) -or (-Not $GIMP_API_VERSION))
  {
    $CONFIG_PATH = Resolve-Path -Path "_build-*\config.h" | Select-Object -ExpandProperty Path
  }

if (-Not $GIMP_VERSION)
  {
    $GIMP_VERSION = Get-Content -Path "$CONFIG_PATH"                         | Select-String 'GIMP_VERSION'        |
                    Foreach-Object {$_ -replace '#define GIMP_VERSION "',''} | Foreach-Object {$_ -replace '"',''}
  }

if (-Not $REVISION)
  {
    $REVISION = "0"
  }

if (-Not $GIMP_APP_VERSION)
  {
    $GIMP_APP_VERSION = Get-Content -Path "$CONFIG_PATH"                             | Select-String 'GIMP_APP_VERSION "'  |
                        Foreach-Object {$_ -replace '#define GIMP_APP_VERSION "',''} | Foreach-Object {$_ -replace '"',''}
  }

if (-Not $GIMP_API_VERSION)
  {
    $GIMP_API_VERSION = Get-Content -Path "$CONFIG_PATH"                                   | Select-String 'GIMP_PKGCONFIG_VERSION' |
                        Foreach-Object {$_ -replace '#define GIMP_PKGCONFIG_VERSION "',''} | Foreach-Object {$_ -replace '"',''}
  }


# This is needed for some machines without TLS 1.2 enabled connecting to the internet
[Net.ServicePointManager]::SecurityProtocol = [Net.SecurityProtocolType]::Tls12, [Net.SecurityProtocolType]::Tls13

# Install or Update Inno Setup
Invoke-WebRequest -URI "https://jrsoftware.org/download.php/is.exe" -OutFile "is.exe"
.\is.exe /SILENT /SUPPRESSMSGBOXES /CURRENTUSER /SP- /LOG="innosetup.log"
Wait-Process is

# Get Inno install path
$log = Get-Content -Path 'innosetup.log' | Select-String 'ISCC.exe'
$pattern = '(?<=filename: ).+?(?=\\ISCC.exe)'
$INNOPATH = [regex]::Matches($log, $pattern).Value

# Get Inno install path (fallback)
#$INNOPATH = Get-ItemProperty -Path Registry::'HKEY_CURRENT_USER\Software\Microsoft\Windows\CurrentVersion\Uninstall\Inno Setup 6_is1' |
#Select-Object -ExpandProperty "InstallLocation"
# if ($INNOPATH -eq "")
#   {
#     $INNOPATH = Get-ItemProperty -Path Registry::'HKLM\Software\Microsoft\Windows\CurrentVersion\Uninstall\Inno Setup 6_is1' |
#     Select-Object -ExpandProperty "InstallLocation"
#   }


# Download Official translations (not present in a Inno release yet)
function download_lang_official ([string]$langfile)
{
  if (Test-Path -Path "$langfile" -PathType Leaf)
    {
      Remove-Item -Path "$langfile" -Force
    }
  Invoke-WebRequest -URI "https://raw.githubusercontent.com/jrsoftware/issrc/main/Files/Languages/${langfile}" -OutFile "$INNOPATH/Languages/${langfile}"
}

#New-Item -ItemType Directory -Path "$INNOPATH/Languages/" -Force

# Download Unofficial translations (of unknown quality and maintenance)
# Cf. https://jrsoftware.org/files/istrans/
function download_lang ([string]$langfile)
{
  if (Test-Path -Path "$langfile" -PathType Leaf)
    {
      Remove-Item -Path "$langfile" -Force
    }
  Invoke-WebRequest -URI "https://raw.githubusercontent.com/jrsoftware/issrc/main/Files/Languages/Unofficial/${langfile}" -OutFile "$INNOPATH/Languages/Unofficial/${langfile}"
}

New-Item -ItemType Directory -Path "$INNOPATH/Languages/Unofficial/" -Force
download_lang Basque.isl
download_lang Belarusian.isl
download_lang ChineseSimplified.isl
download_lang ChineseTraditional.isl
download_lang EnglishBritish.isl
download_lang Esperanto.isl
download_lang Galician.isl
download_lang Georgian.isl
download_lang Greek.isl
download_lang Indonesian.isl
download_lang Latvian.isl
download_lang Lithuanian.isl
download_lang Malaysian.isl
download_lang Marathi.islu
download_lang Romanian.isl
download_lang Swedish.isl
download_lang Vietnamese.isl


$gen_path = Resolve-Path -Path "_build-*\build\windows\installer" | Select-Object -ExpandProperty Path
if (Test-Path -Path $gen_path)
  {
    # Copy generated language files into the source directory
    Copy-Item $gen_path\lang\*isl build\windows\installer\lang\

    # Copy generated images into the source directory
    Copy-Item $gen_path\*bmp build\windows\installer\
  }


# Construct now the installer
Set-Location build/windows/installer
Set-Alias -Name 'iscc' -Value "${INNOPATH}\iscc.exe"
iscc -DGIMP_VERSION="$GIMP_VERSION" -DREVISION="$REVISION" -DGIMP_APP_VERSION="$GIMP_APP_VERSION" -DGIMP_API_VERSION="$GIMP_API_VERSION" -DGIMP_DIR="$GIMP_BASE" -DDIR32="$GIMP32" -DDIR64="$GIMP64" -DDIRA64="$GIMPA64" -DDEPS_DIR="$GIMP_BASE" -DDDIR32="$GIMP32" -DDDIR64="$GIMP64" -DDDIRA64="$GIMPA64" -DDEBUG_SYMBOLS -DLUA -DPYTHON base_gimp3264.iss

# Test if the installer was created and return success/failure.
if (Test-Path -Path "_Output/gimp-${GIMP_VERSION}-setup.exe" -PathType Leaf)
  {
    Set-Location _Output/
    $INSTALLER="gimp-${GIMP_VERSION}-setup.exe"
    Get-FileHash $INSTALLER -Algorithm SHA256 | Out-File -FilePath "${INSTALLER}.SHA256SUMS"
    Get-FileHash $INSTALLER -Algorithm SHA512 | Out-File -FilePath "${INSTALLER}.SHA512SUMS"

    # Return to git golder
    Set-Location ..\..\..\..\

    exit 0
  }
else
  {
    # Return to git golder
    Set-Location ..\..\..\

    exit 1
  }
