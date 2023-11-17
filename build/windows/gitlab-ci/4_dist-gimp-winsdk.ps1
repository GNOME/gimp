#!/usr/bin/env pwsh

# Parameters
param ($gimp_version,
       $gimp_app_version,
       $arch_a64 = 'gimp-a64',
       $arch_x64 = 'gimp-x64')


# Needed tools from Windows SDK
Set-Alias -Name 'makepri' -Value "C:\Program Files (x86)\Windows Kits\10\bin\10.0.22621.0\x64\makepri.exe"
Set-Alias -Name 'makeappx' -Value 'C:\Program Files (x86)\Windows Kits\10\App Certification Kit\MakeAppx.exe'
Set-Alias -Name 'signtool' -Value 'C:\Program Files (x86)\Windows Kits\10\bin\10.0.22621.0\x64\signtool.exe'


# Global variables

## GIMP version (major.minor.micro)
if (-Not $gimp_version)
  {
    $config_path = Resolve-Path -Path "_build-*\config.h" | Select-Object -ExpandProperty Path
    $gimp_version = Get-Content -Path "$config_path"         | Select-String 'GIMP_VERSION'        |
    Foreach-Object {$_ -replace '#define GIMP_VERSION "',''} | Foreach-Object {$_ -replace '"',''}
  }

## GIMP app version (major.minor)
if (-Not $gimp_app_version)
  {
    $config_path = Resolve-Path -Path "_build-*\config.h" | Select-Object -ExpandProperty Path
    $gimp_app_version = Get-Content -Path "$config_path"         | Select-String 'GIMP_APP_VERSION "'  |
    Foreach-Object {$_ -replace '#define GIMP_APP_VERSION "',''} | Foreach-Object {$_ -replace '"',''}
  }

## GIMP arch folders
$vfs_a64 = "$arch_a64\VFS\ProgramFilesX64\GIMP ${gimp_app_version}"
$vfs_x64 = "$arch_x64\VFS\ProgramFilesX64\GIMP ${gimp_app_version}"
$archsArray = "$arch_a64","$arch_x64"
$vfsArray = "$vfs_a64","$vfs_x64"

Set-Location build\windows\store\
New-Item -Path "." -Name ".gitignore" -ItemType "File" -Force
Set-Content ".gitignore" "$arch_a64`n$arch_x64`n_TempOutput`n_Output"


# 1. CONFIGURE MANIFEST
function Configure-Arch ([string]$arch, [string]$arch_msix)
{
  New-Item -ItemType Directory -Path $arch
  Copy-Item AppxManifest.xml $arch

  ## Set GIMP version
  (Get-Content -Path "$arch\AppxManifest.xml") | Foreach-Object {$_ -replace "@GIMP_VERSION@","$gimp_version"} |
  Set-Content -Path "$arch\AppxManifest.xml"

  ## Set GIMP app version
  (Get-Content -Path "$arch\AppxManifest.xml") | Foreach-Object {$_ -replace "@GIMP_APP_VERSION@","$gimp_app_version"} |
  Set-Content -Path "$arch\AppxManifest.xml"

  ## Set arch
  (Get-Content -Path "$arch\AppxManifest.xml") | Foreach-Object {$_ -replace "neutral","$arch_msix"} |
  Set-Content -Path "$arch\AppxManifest.xml"

  ## Match supported filetypes
  $file_types = Get-Content -Path '..\installer\data_associations.list' | Foreach-Object {"              <uap:FileType>." + $_} |
  Foreach-Object {$_ +  "</uap:FileType>"} |
  Where-Object {$_ -notmatch 'xcf'}
  (Get-Content -Path "$arch\AppxManifest.xml") | Foreach-Object {$_ -replace "@FILE_TYPES@","$file_types"}  |
  Set-Content -Path "$arch\AppxManifest.xml"
}

Configure-Arch "$arch_a64" 'arm64'
Configure-Arch "$arch_x64" 'x64'


# 2. CREATE ASSETS

## Copy pre-generated icons to each arch
$icons_path = Resolve-Path -Path "..\..\..\_build-*\build\windows\store\Assets"
if (Test-Path -Path "$icons_path")
  {
    foreach ($arch in $archsArray)
      {
        New-Item -ItemType Directory -Path "$arch\Assets\"
        Copy-Item -Path "$icons_path\*.png" -Destination "$arch\Assets\" -Recurse
      }
  }
else
  {
    "MS Store icons not found. You can generate them adding '-Dms-store=true' option"
    "at meson configure time."
    exit 1
  }

## Generate resources.pri
foreach ($arch in $archsArray)
  {
    Set-Location $arch
    makepri createconfig /cf priconfig.xml /dq lang-en-US /pv 10.0.0
    Set-Location ..\
    makepri new /pr $arch /cf $arch\priconfig.xml /of $arch
    Remove-Item "$arch\priconfig.xml"
  }


# 3. COPY GIMP FILES

## Copy files into VFS folder (to support external 3P plug-ins)
Copy-Item -Path "..\..\..\$arch_a64" -Destination "$vfs_a64" -Recurse
Copy-Item -Path "..\..\..\$arch_x64" -Destination "$vfs_x64" -Recurse

## Remove uneeded files (to match the Inno Windows Installer artifact)
$omissions = ("include\", "lib\gimp\${gimp_app_version}\plug-ins\test-sphere-v3\", "lib\gimp\${gimp_app_version}\plug-ins\ts-helloworld\", "share\gir-1.0\", "share\man\", "share\vala\", "gimp.cmd", "README.txt")
Set-Location $vfs_a64
Remove-Item $omissions -Recurse
Set-Location ..\..\..\..\$vfs_x64
Remove-Item $omissions -Recurse
Set-Location ..\..\..\..\

## Disable Update check (since the package is auto updated)
foreach ($vfs in $vfsArray)
  {
    Add-Content $vfs\share\gimp\$gimp_app_version\gimp-release "check-update=false"
  }

## Remove uncompliant files (to fix 'signtool' issues)
$corrections = ("*.debug", "*.tar")
foreach ($vfs in $vfsArray)
  {
    Get-ChildItem $vfs -Recurse -Include $corrections | Remove-Item -Recurse
  }


# 4. MAKE .MSIXUPLOAD (ONLY FOR RELEASES)
New-Item -ItemType Directory -Path "_TempOutput"
New-Item -ItemType Directory -Path "_Output"
$archsArray_limited = $archsArray -replace "gimp-", ""

## Make .appxsym for each arch
if ($CI_COMMIT_TAG -or ($GIMP_CI_MS_STORE -eq 'MSIXUPLOAD'))
  {
    foreach ($arch in $archsArray_limited)
      {
        Get-ChildItem -Path "gimp-$arch" -Filter "*.pdb" -Recurse |
        Compress-Archive -DestinationPath "_TempOutput\gimp-${gimp_version}_$arch.zip"
        Get-ChildItem "_TempOutput\*.zip" | Rename-Item -NewName { $_.Name -replace '.zip','.appxsym' }
      }
  }

## Make .msix for each arch (this is needed to make the .msixbundle too)
foreach ($arch in $archsArray_limited)
  {
    Get-ChildItem "gimp-$arch" -Include "*.pdb" -Recurse -Force | Remove-Item -Recurse -Force
    makeappx pack /d "gimp-$arch" /p "_TempOutput\gimp-${gimp_version}_$arch.msix"
    Remove-Item "gimp-$arch" -Recurse
  }

## Make .msixupload
if ($CI_COMMIT_TAG -or ($GIMP_CI_MS_STORE -eq 'MSIXUPLOAD'))
  {
    Compress-Archive -Path "_TempOutput\*" "_Output\gimp-${gimp_version}-store.zip"
    Get-ChildItem "_Output\*.zip" | Rename-Item -NewName { $_.Name -replace '.zip','.msixupload' }
    Get-ChildItem "_TempOutput\*.appxsym" | Remove-Item -Recurse -Force
  }


# 5. MAKE .MSIXBUNDLE

## Make .msixbundle with all archs
makeappx bundle /bv "${gimp_version}.0" /d "_TempOutput\" /p "_Output\gimp-${gimp_version}-local.msixbundle"
Remove-Item "_TempOutput\" -Recurse

## Sign .msixbundle
Copy-Item -Path "pseudo-gimp.pfx" -Destination "_Output\" -Recurse
SignTool sign /fd sha256 /a /f _Output\pseudo-gimp.pfx /p eek "_Output\gimp-${gimp_version}-local.msixbundle"


# 6. TEST .MSIXUPLOAD OR .MSIXBUNDLE
if ($CI_COMMIT_TAG -or ($GIMP_CI_MS_STORE -eq 'MSIXUPLOAD'))
  {
    $MSIX_ARTIFACT="gimp-${gimp_version}-store.msixupload"
  }
else
  {
    $MSIX_ARTIFACT="gimp-${gimp_version}-local.msixbundle"
  }

if (Test-Path -Path "_Output\${MSIX_ARTIFACT}" -PathType Leaf)
  {
    Set-Location _Output\
    Get-FileHash $MSIX_ARTIFACT -Algorithm SHA256 | Out-File -FilePath "${MSIX_ARTIFACT}.SHA256SUMS"
    Get-FileHash $MSIX_ARTIFACT -Algorithm SHA512 | Out-File -FilePath "${MSIX_ARTIFACT}.SHA512SUMS"

    ### Return to git golder
    Set-Location ..\..\..\..\

    exit 0
  }
else
  {
    ### Return to git golder
    Set-Location ..\..\..\

    exit 1
  }
