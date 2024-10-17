#!/usr/bin/env pwsh

if (-not $GITLAB_CI)
  {
    # Make the script work locally
    if (-not (Test-Path build\windows) -and -not (Test-Path 2_build-gimp-msys2.ps1 -Type Leaf) -or $PSScriptRoot -notlike "*build\windows*")
      {
        Write-Host '(ERROR): Script called from wrong dir. Please, read: https://developer.gimp.org/core/setup/build/windows/' -ForegroundColor Red
        exit 1
      }
    elseif (Test-Path 2_build-gimp-msys2.ps1 -Type Leaf)
      {
        Set-Location ..\..
      }
    git submodule update --init
  }


# Install the required (pre-built) packages for babl, GEGL and GIMP (again)
Invoke-Expression ((Get-Content build\windows\1_build-deps-msys2.ps1 | Select-String 'MSYS2_PREFIX =' -Context 0,17) -replace '> ','')

if ($GITLAB_CI)
  {
    Invoke-Expression ((Get-Content build\windows\1_build-deps-msys2.ps1 | Select-String 'PACMAN_CONF =' -Context 0,7) -replace '> ','')
  }


# Prepare env
if (-not $GITLAB_CI -and -not $GIMP_PREFIX)
  {
    #FIXME:'gimpenv' have buggy code about Windows paths
    $GIMP_PREFIX = "$PWD\..\_install".Replace('\', '/')
  }

$env:Path = $env:Path + ";$GIMP_PREFIX/bin"
$env:PKG_CONFIG_PATH = "$MSYS2_PREFIX/$MSYSTEM_PREFIX/lib/pkgconfig;$MSYS2_PREFIX/$MSYSTEM_PREFIX/share/pkgconfig;$GIMP_PREFIX/lib/pkgconfig"
$env:XDG_DATA_DIRS = "$MSYS2_PREFIX/$MSYSTEM_PREFIX/share;$GIMP_PREFIX/share"
$env:GI_TYPELIB_PATH = "$MSYS2_PREFIX/$MSYSTEM_PREFIX/lib/girepository-1.0;$GIMP_PREFIX/lib/girepository-1.0"


# Build GIMP
#$pkgconfig_path = (Get-ChildItem $MSYS2_PREFIX/lib/pkgconfig -Filter gio*.pc -Recurse | Select-Object -ExpandProperty Name)
#foreach ($pc in $pkgconfig_path)
#  {
#    (Get-Content $MSYS2_PREFIX/lib/pkgconfig/$pc) | Foreach-Object {$_ -replace "prefix=/clang64","prefix=$MSYS2_PREFIX/$MSYSTEM_PREFIX"} |
#     Set-Content $MSYS2_PREFIX/lib/pkgconfig/$pc
#  }

if (-not (Test-Path _build\build.ninja -Type Leaf))
  {
    New-Item _build -ItemType Directory -Force
    #https://gitlab.gnome.org/GNOME/gimp/-/issues/11200
    #https://gitlab.gnome.org/GNOME/gimp/-/issues/5891
    meson setup _build -Dprefix="$GIMP_PREFIX" -Djavascript=disabled -Dvala=disabled `
                       -Ddirectx-sdk-dir="$MSYS2_PREFIX/$MSYSTEM_PREFIX" -Denable-default-bin=enabled `
                       -Dbuild-id='org.gimp.GIMP_official' $INSTALLER_OPTION $STORE_OPTION
  }
Set-Location _build
ninja
ninja install
ccache --show-stats
Set-Location ..
