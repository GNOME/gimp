#!/usr/bin/env pwsh

# Parameters
param ($revision = "$GIMP_CI_WIN_INSTALLER",
       $BUILD_DIR,
       $ARM64_BUNDLE = "$PWD\gimp-clangarm64",
       $X64_BUNDLE = "$PWD\gimp-clang64",
       $X86_BUNDLE = "$PWD\gimp-mingw32")

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


# 1. GET INNO
Write-Output "$([char]27)[0Ksection_start:$(Get-Date -UFormat %s -Millisecond 0):installer_tlkt$([char]13)$([char]27)[0KChecking Inno installation"
## Install or Update Inno (if needed)
## (We need to ensure that TLS 1.2 is enabled because of some runners)
[Net.ServicePointManager]::SecurityProtocol = [Net.SecurityProtocolType]::Tls12
Invoke-WebRequest https://files.jrsoftware.org/is/6/innosetup-6.5.4.exe -OutFile ..\is.exe
$inno_version_downloaded = (Get-Item ..\is.exe).VersionInfo.ProductVersion -replace ' ',''
$broken_inno = Get-ChildItem $env:TMP -Filter *.isl.bak -ErrorAction SilentlyContinue
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
$INNO_PATH = Get-ItemProperty (Resolve-Path Registry::'HKEY_CURRENT_USER\Software\Microsoft\Windows\CurrentVersion\Uninstall\Inno Setup*') | Select-Object -ExpandProperty InstallLocation
#$INNO_PATH = [regex]::Matches((Get-Content ..\innosetup.log | Select-String ISCC.exe), '(?<=filename: ).+?(?=\\ISCC.exe)').Value
Set-Alias iscc "$INNO_PATH\iscc.exe"

## This script needs a bit of Python to work
#FIXME: Restore the condition when TWAIN 32-bit support is dropped
#if (-not (Get-Command "python" -ErrorAction SilentlyContinue) -or "$(Get-Command "python" -ErrorAction SilentlyContinue | Select-Object -ExpandProperty Source)" -like '*WindowsApps*')
#  {
    Invoke-Expression ((Get-Content build\windows\1_build-deps-msys2.ps1 | Select-String 'MSYS_ROOT\)' -Context 0,12) -replace '> ','')
    $env:PATH = "$env:MSYS_ROOT/$env:MSYSTEM_PREFIX/bin;$env:PATH"
#  }
Write-Output "(INFO): Installed Inno: $inno_version_downloaded | Installed Python: $((python --version) -replace '[^0-9.]', '')"
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
#FIXME: Meson don't support C++ style comments. See: https://github.com/mesonbuild/meson/issues/14260
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

## Autodetects what arch bundles will be packaged
# (The .iss script supports creating both an installer per arch or an universal installer with all arches)
if (-not (Test-Path "$ARM64_BUNDLE") -and -not (Test-Path "$X64_BUNDLE") -and -not (Test-Path "$X86_BUNDLE"))
  {
    Write-Host "(ERROR): No bundle found. You can tweak 'build/windows/2_build-gimp-msys2.ps1' or configure GIMP with '-Dwindows-installer=true' to make one." -ForegroundColor red
    exit 1
  }
elseif ((Test-Path "$ARM64_BUNDLE") -and -not (Test-Path "$X64_BUNDLE") -and -not (Test-Path "$X86_BUNDLE"))
  {
    Write-Output "(INFO): Arch: arm64"
    $supported_archs=@("-DARM64_BUNDLE=$ARM64_BUNDLE")
  }
elseif ((Test-Path "$ARM64_BUNDLE") -and (Test-Path "$X64_BUNDLE") -and (Test-Path "$X86_BUNDLE"))
  {
    Write-Output "(INFO): Arch: arm64, x64 and x86"
    $supported_archs=@("-DARM64_BUNDLE=$ARM64_BUNDLE", "-DX64_BUNDLE=$X64_BUNDLE", "-DX86_BUNDLE=$X86_BUNDLE")
  }
elseif (-not (Test-Path "$ARM64_BUNDLE") -and (Test-Path "$X64_BUNDLE"))
  {
    Write-Output "(INFO): Arch: x64"
    $supported_archs=@("-DX64_BUNDLE=$X64_BUNDLE")
  }
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
  git clone --depth 1 https://github.com/jrsoftware/issrc.git "$env:TMP\issrc"
  foreach ($langfile in $langsArray)
    {
      $langfilePath = "$INNO_PATH\$langfile"
      if ($langfile -ne '' -and -not (Test-Path "$langfilePath" -Type Leaf))
        {
          Write-Output "(INFO): temporarily installing $($langfilePath -replace '\\\\','\')"
          New-Item $(Split-Path -Parent $langfilePath) -ItemType Directory -Force | Out-Null
          Copy-Item "$env:TMP\issrc\Files\$langfile" "$langfilePath" -Force
        }
    }
  Remove-Item "$env:TMP\issrc" -Recurse -Force
}
download_langs $langsArray_Official
download_langs $langsArray_unofficial
### Patch 'AppVer*' against Inno pervasive behavior: https://groups.google.com/g/innosetup/c/w0sebw5YAeg
function fix_msg ([array]$langsArray, [string]$AppVer)
{
  foreach ($langfile in $langsArray)
    {
      $langfilePath = "$INNO_PATH\$langfile" -replace '\\\\','\'
      if ($AppVer -ne 'revert')
        {
          Copy-Item "$langfilePath" "$env:TMP\$(Split-Path $langfile -Leaf).bak" -Force
          #Prefer Python since PowerShell/.NET doesn't handle well files with different encodings
          python build\windows\installer\lang\fix_msg.py "$langfilePath" $AppVer
        }
      else #($AppVer -eq 'revert')
        {
          Move-Item "$env:TMP\$(Split-Path $langfile -Leaf).bak" "$langfilePath" -Force
        }
    }
}
fix_msg 'Default.isl' $CUSTOM_GIMP_VERSION
fix_msg $langsArray_Official $CUSTOM_GIMP_VERSION
fix_msg $langsArray_unofficial $CUSTOM_GIMP_VERSION
Write-Output "$([char]27)[0Ksection_end:$(Get-Date -UFormat %s -Millisecond 0):installer_source$([char]13)$([char]27)[0K"


# 4. PREPARE GIMP FILES
# (Most of the file processing and special-casing is done on *gimp3264.iss [Files] section.
#  The resulting .exe installer content should be identical to the .msix and vice-versa)

## Get 32-bit TWAIN deps list
if (Test-Path "$X86_BUNDLE")
  {
    Write-Output "$([char]27)[0Ksection_start:$(Get-Date -UFormat %s -Millisecond 0):installer_files[collapsed=true]$([char]13)$([char]27)[0KGenerating 32-bit TWAIN dependencies list"
    $twain_list_file = 'build\windows\installer\base_twain32on64.list'
    Copy-Item $twain_list_file "$twain_list_file.bak"
    $twain_list = (python build\windows\2_bundle-gimp-uni_dep.py --debug debug-only $(Resolve-Path $X86_BUNDLE/lib/gimp/*/plug-ins/twain/twain.exe) $env:MSYS_ROOT/mingw32/ $X86_BUNDLE/ 32 |
                  Select-String 'Installed' -CaseSensitive -Context 0,1000) -replace "  `t- ",'bin\'
    (Get-Content $twain_list_file) | Foreach-Object {$_ -replace "@DEPS_GENLIST@","$twain_list"} | Set-Content $twain_list_file
    (Get-Content $twain_list_file) | Select-string 'Installed' -notmatch | Set-Content $twain_list_file
    Write-Output "$([char]27)[0Ksection_end:$(Get-Date -UFormat %s -Millisecond 0):installer_files$([char]13)$([char]27)[0K"
  }


# 5. COMPILE .EXE INSTALLER
$INSTALLER="gimp-${CUSTOM_GIMP_VERSION}-setup.exe"
Write-Output "$([char]27)[0Ksection_start:$(Get-Date -UFormat %s -Millisecond 0):installer_making[collapsed=true]$([char]13)$([char]27)[0KConstructing $INSTALLER installer"
Set-Location build\windows\installer
iscc -DREVISION="$revision" -DBUILD_DIR="$BUILD_DIR" $supported_archs -DDEBUG_SYMBOLS -DPYTHON base_gimp3264.iss | Out-Null; if ("$LASTEXITCODE" -gt '0') { exit 1 }
Set-Location ..\..\..
Write-Output "$([char]27)[0Ksection_end:$(Get-Date -UFormat %s -Millisecond 0):installer_making$([char]13)$([char]27)[0K"

## Revert change done in TWAIN list
if ($twain_list_file)
  {
    Remove-Item $twain_list_file
    Move-Item "$twain_list_file.bak" $twain_list_file
  }
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
    [System.IO.File]::WriteAllText("$PWD\$INSTALLER.SHA256SUMS", "$sha256 *$INSTALLER", $Utf8NoBomEncoding)
    #Set-Content $INSTALLER.SHA256SUMS "$sha256 *$INSTALLER" -Encoding utf8NoBOM -NoNewline
  }
$sha512 = (Get-FileHash $INSTALLER -Algorithm SHA512 | Select-Object -ExpandProperty Hash).ToLower()
Write-Output "(INFO): $INSTALLER SHA-512: $sha512"
if ($GIMP_RELEASE -and -not $GIMP_IS_RC_GIT)
  {
    [System.IO.File]::WriteAllText("$PWD\$INSTALLER.SHA512SUMS", "$sha512 *$INSTALLER", $Utf8NoBomEncoding)
    #Set-Content $INSTALLER.SHA512SUMS "$sha512 *$INSTALLER" -Encoding utf8NoBOM -NoNewline
  }
Write-Output "$([char]27)[0Ksection_end:$(Get-Date -UFormat %s -Millisecond 0):installer_trust$([char]13)$([char]27)[0K"


if ($GITLAB_CI)
  {
    # GitLab doesn't support wildcards when using "expose_as" so let's move to a dir
    $output_dir = "$PWD\build\windows\installer\_Output"
    New-Item $output_dir -ItemType Directory | Out-Null
    Move-Item $INSTALLER* $output_dir
  }
