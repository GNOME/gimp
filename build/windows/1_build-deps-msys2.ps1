#!/usr/bin/env pwsh

# Ensure the script work properly
$ErrorActionPreference = 'Stop'
$PSNativeCommandUseErrorActionPreference = $false #to ensure error catching as in pre-7.4 PS
if (-not (Test-Path build\windows) -and -not (Test-Path 1_build-deps-msys2.ps1 -Type Leaf) -or $PSScriptRoot -notlike "*build\windows*")
  {
    Write-Host '(ERROR): Script called from wrong dir. Please, read: https://developer.gimp.org/core/setup/build/windows/' -ForegroundColor Red
    exit 1
  }
elseif (Test-Path 1_build-deps-msys2.ps1 -Type Leaf)
  {
    Set-Location ..\..
  }
if (-not $GITLAB_CI)
  {
    $GIT_DEPTH = '1'

    $PARENT_DIR = '\..'
  }


# Install the required (pre-built) packages for babl, GEGL and GIMP
if (-not $env:MSYS_ROOT)
  {
    $env:MSYS_ROOT = $(Get-ChildItem HKCU:\Software\Microsoft\Windows\CurrentVersion\Uninstall -Recurse | ForEach-Object { Get-ItemProperty $_.PSPath -ErrorAction SilentlyContinue } | Where-Object { $_.PSObject.Properties.Value -like "*The MSYS2 Developers*" } | ForEach-Object { return "$($_.InstallLocation)" }) -replace '\\','/'
    if ("$env:MSYS_ROOT" -eq '')
      {
        Write-Host '(ERROR): MSYS2 installation not found. Please, install it with: winget install MSYS2.MSYS2' -ForegroundColor Red
        exit 1
      }
  }
if (-not $env:MSYSTEM_PREFIX)
  {
    $env:MSYSTEM_PREFIX = if ((Get-WmiObject Win32_ComputerSystem).SystemType -like 'ARM64*') { 'clangarm64' } else { 'clang64' }
  }

Write-Output "$([char]27)[0Ksection_start:$(Get-Date -UFormat %s -Millisecond 0):deps_install[collapsed=true]$([char]13)$([char]27)[0KInstalling dependencies provided by MSYS2"
if ("$PSCommandPath" -like "*1_build-deps-msys2.ps1*" -or "$CI_JOB_NAME" -like "*deps*")
  {
    powershell -Command { $ProgressPreference = 'SilentlyContinue'; $env:PATH="$env:MSYS_ROOT\usr\bin;$env:PATH"; pacman --noconfirm -Suy }; if ("$LASTEXITCODE" -gt '0') { exit 1 }
  }
powershell -Command { $ProgressPreference = 'SilentlyContinue'; $env:PATH="$env:MSYS_ROOT\usr\bin;$env:PATH"; pacman --noconfirm -S --needed $(if ($env:MSYSTEM_PREFIX -ne 'mingw32') { "$(if ($env:MSYSTEM_PREFIX -eq 'clangarm64') { 'mingw-w64-clang-aarch64' } else { 'mingw-w64-clang-x86_64' })-perl" }) (Get-Content build/windows/all-deps-uni.txt | Where-Object { $_.Trim() -ne '' -and -not $_.Trim().StartsWith('#') }).Replace('${MINGW_PACKAGE_PREFIX}',$(if ($env:MINGW_PACKAGE_PREFIX) { "$env:MINGW_PACKAGE_PREFIX" } elseif ($env:MSYSTEM_PREFIX -eq 'clangarm64') { 'mingw-w64-clang-aarch64' } else { 'mingw-w64-clang-x86_64' })).Replace(' \','') }; if ("$LASTEXITCODE" -gt '0') { exit 1 }
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
function self_build ([string]$dep, [string]$unstable_branch, [string]$stable_patch, [string]$option1, [string]$option2)
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
        elseif ($unstable_branch)
          {
            $git_options="--branch=$unstable_branch"
          }
        git clone $git_options --depth $GIT_DEPTH $repo

        # This allows to add some minor patch on a dependency without having a proper new release.
        if ($CI_COMMIT_TAG -and $stable_patch)
          {
            Set-Location $dep
            git apply $GIMP_DIR\$stable_patch
            Set-Location ..
          }
      }
    Set-Location $dep
    git pull

    ## Configure and/or build
    if (-not (Test-Path _build-$env:MSYSTEM_PREFIX\build.ninja -Type Leaf))
      {
        meson setup _build-$env:MSYSTEM_PREFIX -Dprefix="$GIMP_PREFIX" $PKGCONF_RELOCATABLE_OPTION $option1 $option2; if ("$LASTEXITCODE" -gt '0') { exit 1 }
      }
    Set-Location _build-$env:MSYSTEM_PREFIX
    ninja; if ("$LASTEXITCODE" -gt '0') { exit 1 }
    ninja install; if ("$LASTEXITCODE" -gt '0') { exit 1 }
    Set-Location ../..
    Write-Output "$([char]27)[0Ksection_end:$(Get-Date -UFormat %s -Millisecond 0):${dep}_build$([char]13)$([char]27)[0K"
  }

self_build babl
self_build gegl master build/windows/patches/0001-meson-only-generate-CodeView-.pdb-symbols-on-Windows.patch

Set-Location $GIMP_DIR
