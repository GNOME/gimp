#!/usr/bin/env pwsh

$ErrorActionPreference = 'Stop'
$PSNativeCommandUseErrorActionPreference = $true

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
#MSYS2 forces us to presume 'InstallLocation'. See: https://github.com/msys2/msys2-installer/issues/85
$MSYS2_PREFIX = 'C:/msys64'
if ($MSYSTEM_PREFIX -eq 'mingw32')
  {
    Write-Host '(WARNING): 32-bit builds will be dropped in a future release. See: https://gitlab.gnome.org/GNOME/gimp/-/issues/10922' -ForegroundColor Yellow
  }
elseif ((Get-WmiObject -Class Win32_ComputerSystem).SystemType -like 'ARM64*')
  {
    $MSYSTEM_PREFIX = 'clangarm64'
    $MINGW_PACKAGE_PREFIX = 'mingw-w64-clang-aarch64'
  }
elseif ((Get-WmiObject -Class Win32_ComputerSystem).SystemType -like 'x64*')
  {
    $MSYSTEM_PREFIX = 'clang64'
    $MINGW_PACKAGE_PREFIX = 'mingw-w64-clang-x86_64'
  }
$env:Path = "$MSYS2_PREFIX/$MSYSTEM_PREFIX/bin;$MSYS2_PREFIX/usr/bin;" + $env:Path

pacman --noconfirm -Suy
pacman --noconfirm -S --needed base-devel $MINGW_PACKAGE_PREFIX-toolchain (Get-Content build/windows/all-deps-uni.txt).Replace('${MINGW_PACKAGE_PREFIX}',$MINGW_PACKAGE_PREFIX).Replace(' \','')


# Prepare env
$GIMP_DIR = $PWD

if (-not $GITLAB_CI)
  {
    Set-Location ..

    if (-not $GIMP_PREFIX)
      {
        $GIMP_PREFIX = "$PWD\_install"
      }

    Invoke-Expression ((Get-Content $GIMP_DIR\.gitlab-ci.yml | Select-String 'env:Path \+' -Context 0,3) -replace '> ','' -replace '- ','')
  }


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
            $tagprefix="$dep".ToUpper()
            $tag = (git ls-remote --exit-code --refs --sort=version:refname $repo refs/tags/${tagprefix}_[0-9]*_[0-9]*_[0-9]* | Select-Object -Last 1).Split('refs/')[-1]
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
        meson setup _build -Dprefix="$GIMP_PREFIX" $option1 $option2
      }
    Set-Location _build
    ninja
    ninja install
    if ("$LASTEXITCODE" -gt '0' -or "$?" -eq 'False')
      {
        ## We need to manually check failures in pre-7.4 PS
        exit 1
      }
    ccache --show-stats
    Set-Location ../..
  }

#FIXME: babl dev docs are broken. See: https://gitlab.gnome.org/GNOME/babl/-/issues/97
self_build babl '-Dwith-docs=false'
self_build gegl '-Dworkshop=true'

Set-Location $GIMP_DIR
