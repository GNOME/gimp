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

    $PARENT_DIR = '\..'
  }


# Install the required (pre-built) packages for babl, GEGL and GIMP
if (-not $MSYS_ROOT)
  {
    $MSYS_ROOT = $(Get-ChildItem HKCU:\Software\Microsoft\Windows\CurrentVersion\Uninstall -Recurse | ForEach-Object { Get-ItemProperty $_.PSPath -ErrorAction SilentlyContinue } | Where-Object { $_.PSObject.Properties.Value -like "*The MSYS2 Developers*" } | ForEach-Object { return "$($_.InstallLocation)" }) -replace '\\','/'
    if ("$MSYS_ROOT" -eq '')
      {
        Write-Host '(ERROR): MSYS2 installation not found. Please, install it with: winget install MSYS2.MSYS2' -ForegroundColor Red
        exit 1
      }
  }
if (-not $MSYSTEM_PREFIX)
  {
    $MSYSTEM_PREFIX = if ((Get-WmiObject Win32_ComputerSystem).SystemType -like 'ARM64*') { 'clangarm64' } else { 'clang64' }
  }
$env:Path = "$MSYS_ROOT/$MSYSTEM_PREFIX/bin;$MSYS_ROOT/usr/bin;" + $env:Path

Write-Output "$([char]27)[0Ksection_start:$(Get-Date -UFormat %s -Millisecond 0):deps_install[collapsed=true]$([char]13)$([char]27)[0KInstalling dependencies provided by MSYS2"
if ("$PSCommandPath" -like "*1_build-deps-msys2.ps1*" -or "$CI_JOB_NAME" -like "*deps*")
  {
    pacman --noconfirm -Suy
  }
pacman --noconfirm -S --needed (Get-Content build/windows/all-deps-uni.txt).Replace('${MINGW_PACKAGE_PREFIX}',$(if ($MINGW_PACKAGE_PREFIX) { "$MINGW_PACKAGE_PREFIX" } elseif ($MSYSTEM_PREFIX -eq 'clangarm64') { 'mingw-w64-clang-aarch64' } else { 'mingw-w64-clang-x86_64' })).Replace(' \','')
Write-Output "$([char]27)[0Ksection_end:$(Get-Date -UFormat %s -Millisecond 0):deps_install$([char]13)$([char]27)[0K"


# Prepare env
$GIMP_DIR = $PWD
Set-Location ${GIMP_DIR}${PARENT_DIR}

if (-not $GIMP_PREFIX)
  {
    $GIMP_PREFIX = "$PWD\_install"
  }

Invoke-Expression ((Get-Content $GIMP_DIR\.gitlab-ci.yml | Select-String 'win_environ\[' -Context 0,7) -replace '> ','' -replace '- ','')


# Build babl and GEGL
function self_build ([string]$dep, [string]$option1, [string]$option2)
  {
    Write-Output "$([char]27)[0Ksection_start:$(Get-Date -UFormat %s -Millisecond 0):${dep}_build[collapsed=true]$([char]13)$([char]27)[0KBuilding $dep"

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
    if (-not (Test-Path _build-$MSYSTEM_PREFIX\build.ninja -Type Leaf))
      {
        meson setup _build-$MSYSTEM_PREFIX -Dprefix="$GIMP_PREFIX" $PKGCONF_RELOCATABLE_OPTION $option1 $option2
      }
    Set-Location _build-$MSYSTEM_PREFIX
    ninja
    ninja install
    if ("$LASTEXITCODE" -gt '0' -or "$?" -eq 'False')
      {
        ## We need to manually check failures in pre-7.4 PS
        exit 1
      }
    Set-Location ../..
    Write-Output "$([char]27)[0Ksection_end:$(Get-Date -UFormat %s -Millisecond 0):${dep}_build$([char]13)$([char]27)[0K"
  }

self_build babl
self_build gegl

Set-Location $GIMP_DIR
