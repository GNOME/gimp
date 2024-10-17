#!/usr/bin/env pwsh

if (-not $GITLAB_CI)
  {
    # Make the script work locally
    if (-not (Test-Path build\windows) -and -not (Test-Path 1_build-deps-msys2.ps1 -Type Leaf) -or $PSScriptRoot -notlike "*build\windows*")
      {
        Write-Host '(ERROR): Script called from wrong dir. Please, read: https://developer.gimp.org/core/setup/build/windows/' -ForegroundColor Red
        exit 1
      }
    elseif (Test-Path 1_build-deps-msys2.ps1 -Type Leaf)
      {
        Set-Location ..\..
      }
    $GIT_DEPTH = '1'
  }


# Install the required (pre-built) packages for babl, GEGL and GIMP
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

#https://github.com/msys2/MSYS2-packages/issues/4340
$PACMAN_CONF = "$MSYS2_PREFIX\etc\pacman.conf"
Copy-Item $PACMAN_CONF "$PACMAN_CONF.bak"
(Get-Content $PACMAN_CONF) | Foreach-Object {$_ -replace "SigLevel    = Required","SigLevel    = DatabaseNever"} |
Set-Content $PACMAN_CONF
pacman --noconfirm -Suy
pacman --noconfirm -S --needed base-devel $MINGW_PACKAGE_PREFIX-toolchain (Get-Content build/windows/all-deps-uni.txt).Replace("MINGW_PACKAGE_PREFIX",$MINGW_PACKAGE_PREFIX)
Remove-Item $PACMAN_CONF
Rename-Item "$PACMAN_CONF.bak" $PACMAN_CONF


# Prepare env
$GIMP_DIR = $PWD
if (-not $GITLAB_CI -and -not $GIMP_PREFIX)
  {
    Set-Location ..
    $GIMP_PREFIX = "$PWD\_install"
  }

$env:Path = $env:Path + ";$GIMP_PREFIX/bin"
$env:PKG_CONFIG_PATH = "$MSYS2_PREFIX/$MSYSTEM_PREFIX/lib/pkgconfig;$MSYS2_PREFIX/$MSYSTEM_PREFIX/share/pkgconfig;$GIMP_PREFIX/lib/pkgconfig"
$env:XDG_DATA_DIRS = "$MSYS2_PREFIX/$MSYSTEM_PREFIX/share;$GIMP_PREFIX/share"
$env:GI_TYPELIB_PATH = "$MSYS2_PREFIX/$MSYSTEM_PREFIX/lib/girepository-1.0;$GIMP_PREFIX/lib/girepository-1.0"


# Build babl and GEGL
function self_build ([string]$dep, [string]$option1, [string]$option2)
  {
    ## Make sure that the deps repos are fine
    if (-not (Test-Path $dep))
      {
        $repo="https://gitlab.gnome.org/GNOME/$dep.git"

        # For tagged jobs (i.e. release or test jobs for upcoming releases), use the
        # last tag. Otherwise use the default branch's HEAD.
        if ($CI_COMMIT_TAG)
          {
            $tag = (git ls-remote --exit-code --refs --sort=version:refname $repo refs/tags/GIMP_[0-9]*_* | Select-Object -Last 1).Split('refs/')[-1]
            $git_options="--branch=$tag"
            Write-Output "Using tagged release of ${dep}: $tag"
          }

        git clone $git_options --depth $GIT_DEPTH $repo
      }
    Set-Location $dep
    git pull

    ## Configure and/or build
    if (-not (Test-Path _build\build.ninja -Type Leaf))
      {
        New-Item _build -ItemType Directory -Force
        meson setup _build -Dprefix="$GIMP_PREFIX" $option1 $option2
      }
    Set-Location _build
    ninja
    ninja install
    ccache --show-stats
    Set-Location ../..
  }

#https://gitlab.gnome.org/GNOME/babl/-/issues/97
self_build babl '-Dwith-docs=false' '-Denable-vapi=false'
self_build gegl '-Dworkshop=true' '-Dvapigen=disabled'

Set-Location $GIMP_DIR
