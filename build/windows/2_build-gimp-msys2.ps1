#!/usr/bin/env pwsh

# Ensure the script work properly
$ErrorActionPreference = 'Stop'
$PSNativeCommandUseErrorActionPreference = $false #to ensure error catching as in pre-7.4 PS
if (-not (Test-Path build\windows) -and -not (Test-Path 2_build-gimp-msys2.ps1 -Type Leaf) -or $PSScriptRoot -notlike "*build\windows*")
  {
    Write-Host '(ERROR): Script called from wrong dir. Please, read: https://developer.gimp.org/core/setup/build/windows/' -ForegroundColor Red
    exit 1
  }
elseif (Test-Path 2_build-gimp-msys2.ps1 -Type Leaf)
  {
    Set-Location ..\..
  }
if (-not $GITLAB_CI)
  {
    git submodule update --init

    $NON_RELOCATABLE_OPTION = '-Drelocatable-bundle=no'
  }


# Install the required (pre-built) packages for babl, GEGL and GIMP (again)
Invoke-Expression ((($((Get-Content build\windows\1_build-deps-msys2.ps1 | Select-String '-not \$env:VCPKG_ROOT' -Context 0,26).ToString().Split("`n") | Where-Object { $_.Trim() -ne '' } | ForEach-Object { $_ -replace '> ', '' })) -join "`n"))

if ($GITLAB_CI)
  {
    Invoke-Expression ((Get-Content build\windows\1_build-deps-msys2.ps1 | Select-String 'deps_install\[' -Context 0,15) -replace '> ','')
  }


# Prepare env
if (-not $GIMP_PREFIX)
  {
    $GIMP_PREFIX = "$PWD\..\_install"
  }
Invoke-Expression ((Get-Content .gitlab-ci.yml | Select-String 'win_environ\[' -Context 0,9) -replace '> ','' -replace '- ','')


# Build GIMP
Write-Output "$([char]27)[0Ksection_start:$(Get-Date -UFormat %s -Millisecond 0):gimp_build[collapsed=true]$([char]13)$([char]27)[0KBuilding GIMP"
if (-not (Test-Path _build-$(@($env:VCPKG_DEFAULT_TRIPLET,$env:MSYSTEM_PREFIX) | ?{$_} | select -First 1)\build.ninja -Type Leaf))
  {
    #FIXME: There is no GJS for Windows. See: https://gitlab.gnome.org/GNOME/gimp/-/issues/5891
    meson setup _build-$(@($env:VCPKG_DEFAULT_TRIPLET,$env:MSYSTEM_PREFIX) | ?{$_} | select -First 1) -Dprefix="$GIMP_PREFIX" $NON_RELOCATABLE_OPTION `
                $PKGCONF_RELOCATABLE_OPTION $INSTALLER_OPTION $STORE_OPTION `
                -Denable-default-bin=enabled -Dbuild-id='org.gimp.GIMP_official';
    if ("$LASTEXITCODE" -gt '0') { exit 1 }
  }
Set-Location _build-$(@($env:VCPKG_DEFAULT_TRIPLET,$env:MSYSTEM_PREFIX) | ?{$_} | select -First 1)
ninja; if ("$LASTEXITCODE" -gt '0') { exit 1 }
Write-Output "$([char]27)[0Ksection_end:$(Get-Date -UFormat %s -Millisecond 0):gimp_build$([char]13)$([char]27)[0K"


# Bundle GIMP
Write-Output "$([char]27)[0Ksection_start:$(Get-Date -UFormat %s -Millisecond 0):gimp_bundle[collapsed=true]$([char]13)$([char]27)[0K$(if (-not (Select-String -Quiet '-Dwindows-installer=true' meson-logs/meson-log.txt) -and -not (Select-String -Quiet '-Dms-store=true' meson-logs/meson-log.txt)) {'Installing GIMP as non-relocatable on GIMP_PREFIX'} else {'Creating bundle'})"
ninja install | Out-File ninja_install.log; if ("$LASTEXITCODE" -gt '0') { Get-Content ninja_install.log; exit 1 }; Remove-Item ninja_install.log
Set-Location ..
Write-Output "$([char]27)[0Ksection_end:$(Get-Date -UFormat %s -Millisecond 0):gimp_bundle$([char]13)$([char]27)[0K"
