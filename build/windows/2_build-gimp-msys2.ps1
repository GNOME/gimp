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
#https://github.com/msys2/msys2-installer/issues/85
$MSYS2_PREFIX = 'C:/msys64'
if ((Get-WmiObject -Class Win32_ComputerSystem).SystemType -like 'ARM64*')
  {
    $MSYSTEM_PREFIX = 'clangarm64'
    $MINGW_PACKAGE_PREFIX = 'mingw-w64-clang-aarch64'
  }
elseif ((Get-WmiObject -Class Win32_ComputerSystem).SystemType -like 'x64*')
  {
    $MSYSTEM_PREFIX = 'clang64'
    $MINGW_PACKAGE_PREFIX = 'mingw-w64-clang-x86_64'
  }
else
  {
    Write-Host '(WARNING): 32-bit builds will be dropped in a future release. See: https://gitlab.gnome.org/GNOME/gimp/-/issues/10922' -ForegroundColor Yellow
    $MSYSTEM_PREFIX = 'mingw32'
    $MINGW_PACKAGE_PREFIX = 'mingw-w64-i686'
  }
$env:Path = "$MSYS2_PREFIX/$MSYSTEM_PREFIX/bin;$MSYS2_PREFIX/usr/bin;" + $env:Path

if ($GITLAB_CI)
  {
    pacman --noconfirm -S --needed base-devel (Get-Content build/windows/all-deps-uni.txt).Replace("MINGW_PACKAGE_PREFIX",$MINGW_PACKAGE_PREFIX)
  }


# Prepare env
if (-not $GITLAB_CI -and -not $GIMP_PREFIX)
  {
    #FIXME:'gimpenv' have buggy code about Windows paths
    $GIMP_PREFIX = "$PWD\..\_install".Replace('\', '/')
  }

$env:Path = "$GIMP_PREFIX/bin;" + $env:Path
$env:PKG_CONFIG_PATH = "$GIMP_PREFIX/lib/pkgconfig;$MSYS2_PREFIX/$MSYSTEM_PREFIX/lib/pkgconfig;$MSYS2_PREFIX/$MSYSTEM_PREFIX/share/pkgconfig"
$env:XDG_DATA_DIRS = "$GIMP_PREFIX/share;$MSYS2_PREFIX/$MSYSTEM_PREFIX/share"
$env:GI_TYPELIB_PATH = "$GIMP_PREFIX/lib/girepository-1.0;$MSYS2_PREFIX/$MSYSTEM_PREFIX/lib/girepository-1.0"


# Build GIMP
#$pkgconfig_path = (Get-ChildItem $MSYS2_PREFIX/lib/pkgconfig -Filter *.pc -Recurse | Select-Object -ExpandProperty Name)
#foreach ($pc in $pkgconfig_path)
#  {
#    (Get-Content $MSYS2_PREFIX/lib/pkgconfig/$pc) | Foreach-Object {$_ -replace "prefix=/clang64","prefix=$MSYS2_PREFIX/$MSYSTEM_PREFIX"} |
#     Set-Content $MSYS2_PREFIX/lib/pkgconfig/$pc
#  }

if (-not (Test-Path _build\build.ninja -Type Leaf))
  {
    New-Item _build -ItemType Directory -Force
    meson setup _build -Dprefix="$GIMP_PREFIX" `
                       -Dgi-docgen=disabled -Djavascript=disabled -Dvala=disabled `
                       -Ddirectx-sdk-dir="$MSYS2_PREFIX/$MSYSTEM_PREFIX" -Denable-default-bin=enabled `
                       -Dbuild-id='org.gimp.GIMP_official' $INSTALLER_OPTION $STORE_OPTION
  }
Set-Location _build
ninja
ninja install
Set-Location ..
