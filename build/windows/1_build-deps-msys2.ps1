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
powershell -Command { $ProgressPreference = 'SilentlyContinue'; $env:PATH="$env:MSYS_ROOT\usr\bin;$env:PATH"; pacman --noconfirm -Suy }; if ("$LASTEXITCODE" -gt '0') { exit 1 }
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
function self_build ([string]$repo, [array]$branch, [array]$patches, [array]$options)
  {
    $dep = ($repo -split "/")[-1] -replace '.git',''
    Write-Output "$([char]27)[0Ksection_start:$(Get-Date -UFormat %s -Millisecond 0):${dep}_build[collapsed=true]$([char]13)$([char]27)[0KBuilding $dep"
    ## Make sure that the deps repos are fine
    if (-not (Test-Path $dep))
      {
        ### For tagged jobs (i.e. release or test jobs for upcoming releases), use the
        ### last tag. Otherwise use the default branch's HEAD.
        $repo = if ($repo -notlike '*/*') { "https://gitlab.gnome.org/GNOME/$dep.git" } else { $repo }
        if (($repo -like '*babl*' -or $repo -like '*gegl*') -and $CI_COMMIT_TAG)
          {
            $tag_branch = (git ls-remote --exit-code --refs --sort=version:refname $repo refs/tags/$($dep.ToUpper())_[0-9]*_[0-9]*_[0-9]* | Select-Object -Last 1).Split('refs/')[-1]
          }
        else
          {
            $tag_branch = if ($null -eq $branch -or $branch -eq '' -or $branch -like '*.patch*' -or $branch -like '-*') { 'master' } else { $branch }
          }
        Write-Output "Using tag/branch of ${dep}: ${tag_branch}"
        git clone --branch=${tag_branch} --depth $GIT_DEPTH $repo
      }
    Set-Location $dep
    git pull
    if ($branch -like "*.patch*" -or $patches -like "*.patch*")
      {
        ### This allows to add some minor patch on a dependency without having a proper new release.
        foreach ($patch in $(if ($branch -like '*.patch*') { $branch } else { $patches }))
          {
            if ("$patch" -notlike "http*")
              {
                git apply -v $GIMP_DIR\$patch
              }
            else
              {
                $downloaded_patch = "$PWD\$(Split-Path $patch -Leaf)"
                #(We need to ensure that TLS 1.2 is enabled because of some runners)
                [Net.ServicePointManager]::SecurityProtocol = [Net.SecurityProtocolType]::Tls12
                #We need to use .NET directly since Invoke-WebRequest does not work with GitHub
                Add-Type -AssemblyName System.Net.Http; $client = [System.Net.Http.HttpClient]::new();
                $response = $client.GetAsync("$patch").Result; [System.IO.File]::WriteAllBytes("$downloaded_patch", $response.Content.ReadAsByteArrayAsync().Result)
                git apply -v $downloaded_patch
                Remove-item $downloaded_patch
              }
          }
      }

    ## Configure and/or build
    if (-not (Test-Path _build-$env:MSYSTEM_PREFIX\build.ninja -Type Leaf))
      {
        if ((Test-Path meson.build -Type Leaf) -and -not (Test-Path CMakeLists.txt -Type Leaf))
          {
            #babl and GEGL already auto install .pdb but we don't know about other eventual deps
            if ("$env:MSYSTEM_PREFIX" -ne 'MINGW32')
              {
                if ("$dep" -ne 'babl' -and "$dep" -ne 'gegl')
                  {
                    Add-Content meson.build "meson.add_install_script(find_program('$("$GIMP_DIR".Replace('\','/'))/build/windows/2_bundle-gimp-uni_sym.py'))"
                  }
                $clang_opts_meson=@('-Dc_args=-"fansi-escape-codes -gcodeview"', '-Dcpp_args=-"fansi-escape-codes -gcodeview"', '-Dc_link_args="-Wl,--pdb="', '-Dcpp_link_args="-Wl,--pdb="')
              }
            meson setup _build-$env:MSYSTEM_PREFIX -Dprefix="$GIMP_PREFIX" $PKGCONF_RELOCATABLE_OPTION `
                        -Dbuildtype=debugoptimized $clang_opts_meson `
                        $(if ($branch -like '-*') { $branch } elseif ($patches -like '-*') { $patches } else { $options });
          }
        elseif (Test-Path CMakeLists.txt -Type Leaf)
          {
            if ("$env:MSYSTEM_PREFIX" -ne 'MINGW32')
              {
                Add-Content CMakeLists.txt "install(CODE `"execute_process(COMMAND `${Python3_EXECUTABLE`} $("$GIMP_DIR".Replace('\','/'))/build/windows/2_bundle-gimp-uni_sym.py`)`")"
                $clang_opts_cmake=@('-DCMAKE_C_FLAGS="-gcodeview"', '-DCMAKE_CXX_FLAGS="-gcodeview"', '-DCMAKE_EXE_LINKER_FLAGS="-Wl,--pdb="', '-DCMAKE_SHARED_LINKER_FLAGS="-Wl,--pdb="', '-DCMAKE_MODULE_LINKER_FLAGS="-Wl,--pdb="')
              }
            cmake -G Ninja -B _build-$env:MSYSTEM_PREFIX -DCMAKE_INSTALL_PREFIX="$GIMP_PREFIX" `
                  -DCMAKE_BUILD_TYPE=RelWithDebInfo -DCMAKE_COLOR_DIAGNOSTICS=ON $clang_opts_cmake `
                  $(if ($branch -like '-*') { $branch } elseif ($patches -like '-*') { $patches } else { $options });
          }
        if ("$LASTEXITCODE" -gt '0') { exit 1 }
      }
    Set-Location _build-$env:MSYSTEM_PREFIX
    ninja; if ("$LASTEXITCODE" -gt '0') { exit 1 }
    ninja install; if ("$LASTEXITCODE" -gt '0') { exit 1 }
    Set-Location ../..
    Write-Output "$([char]27)[0Ksection_end:$(Get-Date -UFormat %s -Millisecond 0):${dep}_build$([char]13)$([char]27)[0K"
  }

self_build babl
self_build gegl @('-Dworkshop=true')
if ("$env:MSYSTEM_PREFIX" -ne 'MINGW32')
  {
    self_build https://github.com/Exiv2/exiv2 "v0.28.7" @('https://github.com/Exiv2/exiv2/pull/3361.patch') @('-DCMAKE_DLL_NAME_WITH_SOVERSION=ON', '-DEXIV2_BUILD_EXIV2_COMMAND=OFF', '-DEXIV2_ENABLE_VIDEO=OFF')
  }

Set-Location $GIMP_DIR
