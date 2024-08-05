#!/usr/bin/env pwsh

# Parameters
param ($GIMP_VERSION,
       $REVISION,
       $GIMP_APP_VERSION,
       $GIMP_API_VERSION,
       $BUILD_DIR = '..\..\..\_build',
       $GIMP_BASE = '..\..\..\',
       $GIMP32 = 'gimp-x86',
       $GIMP64 = 'gimp-x64',
       $GIMPA64 = 'gimp-a64')

if ((-Not $GIMP_VERSION) -or (-Not $GIMP_APP_VERSION) -or (-Not $GIMP_API_VERSION))
  {
    $CONFIG_PATH = '_build\config.h'
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
Invoke-WebRequest -URI "https://jrsoftware.org/download.php/is.exe" -OutFile "..\is.exe"
..\is.exe /VERYSILENT /SUPPRESSMSGBOXES /CURRENTUSER /SP- /LOG="..\innosetup.log"
Wait-Process is

# Get Inno install path
$log = Get-Content -Path '..\innosetup.log' | Select-String 'ISCC.exe'
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
New-Item -ItemType Directory -Path "$INNOPATH/Languages/Unofficial/" -Force

$xmlObject = New-Object XML
$xmlObject.Load("$PWD\build\windows\installer\lang\iso_639_custom.xml")
$langsArray = $xmlObject.iso_639_entries.iso_639_entry |
              Select-Object -ExpandProperty inno_code  | Where-Object { $_ -like "*Unofficial*" }

foreach ($langfile in $langsArray)
  {
    $langfileUnix = $langfile.Replace('\\', '/')
    Invoke-WebRequest -URI "https://raw.githubusercontent.com/jrsoftware/issrc/main/Files/$langfileUnix" -OutFile "$INNOPATH/$langfileUnix"
  }


# Patch 'AppVerName' against Inno pervasive behavior: https://groups.google.com/g/innosetup/c/w0sebw5YAeg
if ($REVISION -eq '0')
  {
    $AppVer = $GIMP_VERSION
  }
else
  {
    $AppVer = "$GIMP_VERSION.$REVISION"
  }

function fix_msg ([string]$langsdir)
{
  #Prefer MSYS2 since PowerShell/.NET doesn't handle well files with mixed encodings
  $GIMP_DIR = $PWD
  Copy-Item build/windows/installer/lang/fix_msg.sh $langsdir
  Set-Location $langsdir
  (Get-Content fix_msg.sh) | Foreach-Object {$_ -replace "AppVer","$AppVer"} |
  Set-Content fix_msg.sh
  $Env:CHERE_INVOKING = "yes"
  C:\msys64\usr\bin\bash -lc "bash fix_msg.sh"
  Remove-Item fix_msg.sh
  Set-Location $GIMP_DIR

  #$langsArray_local = Get-ChildItem *.isl -Name
  #foreach ($langfile in $langsArray_local)
    #{
    #  $msg = Get-Content $langfile
    #  $linenumber = $msg | Select-String 'SetupWindowTitle' | Select-Object -ExpandProperty LineNumber
    #  $msg | ForEach-Object { If ($_.ReadCount -eq $linenumber) {$_ -Replace "%1", "%1 $AppVer"} Else {$_} } |
    #         Set-Content -Path "$langfile" -Encoding UTF8
    #  $msg = Get-Content $langfile
    #  $linenumber = $msg | Select-String 'UninstallAppFullTitle' | Select-Object -ExpandProperty LineNumber
    #  $msg | ForEach-Object { If ($_.ReadCount -eq $linenumber) {$_ -Replace "%1", "%1 $AppVer"} Else {$_} } |
    #         Set-Content -Path "$langfile" -Encoding UTF8
    #}
}

fix_msg $INNOPATH
fix_msg $INNOPATH\Languages
fix_msg $INNOPATH\Languages\Unofficial


# Construct now the installer
Set-Location build/windows/installer
Set-Alias -Name 'iscc' -Value "${INNOPATH}\iscc.exe"
iscc -DGIMP_VERSION="$GIMP_VERSION" -DREVISION="$REVISION" -DGIMP_APP_VERSION="$GIMP_APP_VERSION" -DGIMP_API_VERSION="$GIMP_API_VERSION" -DBUILD_DIR="$BUILD_DIR" -DGIMP_DIR="$GIMP_BASE" -DDIR32="$GIMP32" -DDIR64="$GIMP64" -DDIRA64="$GIMPA64" -DDEPS_DIR="$GIMP_BASE" -DDDIR32="$GIMP32" -DDDIR64="$GIMP64" -DDDIRA64="$GIMPA64" -DDEBUG_SYMBOLS -DLUA -DPYTHON base_gimp3264.iss

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
