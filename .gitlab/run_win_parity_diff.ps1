#!/usr/bin/env pwsh

param ($jump)

$ErrorActionPreference = 'Stop'
$PSNativeCommandUseErrorActionPreference = $true


# 1. AUTODECTET LATEST POWERSHELL
if (-not (Get-Command "pwsh" -ErrorAction SilentlyContinue))
  {
    if ((Get-WmiObject -Class Win32_ComputerSystem).SystemType -like 'ARM64*')
      {
        $cpu_arch = 'arm64'
      }
    else
      {
        $cpu_arch = 'x64'
      }

    [Net.ServicePointManager]::SecurityProtocol = [Net.SecurityProtocolType]::Tls12

    $pwsh_tag = (Invoke-WebRequest https://api.github.com/repos/PowerShell/PowerShell/releases | ConvertFrom-Json)[0].tag_name
    $xmlObject = New-Object XML
    $xmlObject.Load("https://raw.githubusercontent.com/PowerShell/PowerShell/refs/tags/$pwsh_tag/PowerShell.Common.props")
    $dotnet_major = ($xmlObject.Project.PropertyGroup.TargetFramework | Out-String) -replace "`r`n",'' -replace 'net',''
    $dotnet_tag = ((Invoke-WebRequest https://api.github.com/repos/dotnet/runtime/releases | ConvertFrom-Json).tag_name | Select-String "$dotnet_major" | Select-Object -First 1).ToString() -replace 'v',''

    if (-not (Test-Path "$Env:ProgramFiles\dotnet\shared\Microsoft.NETCore.App\$dotnet_major*\"))
      {
        Write-Output "(INFO): downloading .NET v$dotnet_tag"
        Invoke-WebRequest https://aka.ms/dotnet/$dotnet_major/dotnet-runtime-win-$cpu_arch.zip -OutFile ${PARENT_DIR}dotnet-runtime.zip
        Expand-Archive ${PARENT_DIR}dotnet-runtime.zip ${PARENT_DIR}dotnet-runtime -Force
        $env:PATH = "$(Resolve-Path $PWD\${PARENT_DIR}dotnet-runtime);" + $env:PATH
        $env:DOTNET_ROOT = "$(Resolve-Path $PWD\${PARENT_DIR}dotnet-runtime)"
      }

    if (-not (Test-Path Registry::'HKEY_CURRENT_USER\Software\Microsoft\Windows\CurrentVersion\App Paths\MSStore.exe'))
      {
        Write-Output "(INFO): downloading PowerShell $pwsh_tag"
        Invoke-WebRequest https://github.com/Powershell/PowerShell/releases/download/$pwsh_tag/PowerShell-$($pwsh_tag -replace 'v','')-win-$cpu_arch.zip -OutFile ${PARENT_DIR}PowerShell.zip
        Expand-Archive ${PARENT_DIR}PowerShell.zip ${PARENT_DIR}PowerShell -Force
        $env:PATH = "$(Resolve-Path $PWD\${PARENT_DIR}PowerShell);" + $env:PATH
      }
  }
if ($PSVersionTable.PSVersion.Major -eq 5)
  {
    pwsh .\compare.ps1 jump
  }

if ($PSVersionTable.PSVersion.Major -ne 5 -or $jump)
  {
    #Get info about GIMP version etc
    ForEach ($line in $(Select-String 'define' "$(Get-ChildItem _build* | Select-Object -First 1)\config.h" -AllMatches))
      {
        Invoke-Expression $($line -replace '^.*#' -replace 'define ','$' -replace ' ','=')
      }

    # Bundle files thar serve as paradigm
    $excludes = 'gimp.cmd|.gitignore|python|twain|\\32|\\uninst'
    $bundle_path = if ((Get-WmiObject Win32_ComputerSystem).SystemType -like 'ARM64*') { 'gimp-clangarm64' } else { 'gimp-clang64' }
    Write-Output "(INFO): listing files in $bundle_path bundle"
    $bundle_files = Get-ChildItem "$bundle_path" -Recurse | Where-Object { $_.FullName -notmatch $excludes } | Sort-Object Name | Select-Object Name

    
    # Check if installer have parity with bundle
    $installer_path = Resolve-Path build\windows\installer\_Output\gimp*setup*.exe -ErrorAction SilentlyContinue
    if ("$installer_path" -ne '')
      {
        Write-Output "(INFO): listing files in installer installation"
        $uninstaller_path = Get-ItemProperty Registry::"HKEY_CURRENT_USER\Software\Microsoft\Windows\CurrentVersion\Uninstall\GIMP-$GIMP_MUTEX_VERSION*" -ErrorAction SilentlyContinue | Select-Object -ExpandProperty UninstallString
        if ("$uninstaller_path" -ne '')
          {
            $installation_path = (Get-ItemProperty Registry::"HKEY_CURRENT_USER\Software\Microsoft\Windows\CurrentVersion\Uninstall\GIMP-$GIMP_MUTEX_VERSION*" -ErrorAction SilentlyContinue | Select-Object -ExpandProperty InstallLocation)
            Start-Process "$uninstaller_path" -ArgumentList "/VERYSILENT", "/SUPPRESSMSGBOXES" -Wait
            Remove-Item "$installation_path" -Recurse -Force -ErrorAction SilentlyContinue
          }
        Start-Process $installer_path -ArgumentList "/VERYSILENT", "/SUPPRESSMSGBOXES", "/CURRENTUSER", "/SP-" -Wait
        $installation_path = (Get-ItemProperty Registry::"HKEY_CURRENT_USER\Software\Microsoft\Windows\CurrentVersion\Uninstall\GIMP-$GIMP_MUTEX_VERSION*" -ErrorAction SilentlyContinue | Select-Object -ExpandProperty InstallLocation).TrimEnd('\')
        $installation_files = Get-ChildItem "$installation_path" -Recurse | Where-Object { $_.FullName -notmatch $excludes } | Sort-Object Name | Select-Object Name

        Write-Output "(INFO): comparing bundle with installer installation"
        Compare-Object -ReferenceObject $bundle_files -DifferenceObject $installation_files -Property Name
        Write-Host "`n"
      }
   

    # Check if msix have parity with bundle
    $msixbundle_path = Resolve-Path "build\windows\store\_Output\*neutral*" -ErrorAction SilentlyContinue
    if ("$msixbundle_path" -ne '')
      {
        Write-Output "(INFO): listing files in MSIX pseudo-installation"
        ##Extract .msixbundle
        $new_msixbundle_path = $msixbundle_path -replace '.msixbundle', '.zip'
        Rename-Item "$msixbundle_path" "$new_msixbundle_path"
        Expand-Archive "$new_msixbundle_path" "$msixbundle_path" -Force
        ##Extract .msix
        $msix_path = Resolve-Path "$msixbundle_path\*$(if ((Get-WmiObject Win32_ComputerSystem).SystemType -like 'ARM64*') { 'arm64' } else { 'x64' })*"
        $new_msix_path = $msix_path -replace '4.msix', '4.zip'
        Rename-Item "$msix_path" "$new_msix_path"
        Expand-Archive "$new_msix_path" "$msix_path" -Force
        Add-Type -AssemblyName System.Web
        Get-ChildItem "$msix_path" -Recurse | Where-Object { $_.Name -match "%" } | ForEach-Object { Rename-Item -LiteralPath $_.FullName -NewName ([System.Web.HttpUtility]::UrlDecode($_.Name)) }
        $msix_files = Get-ChildItem "$msix_path\VFS\ProgramFilesX64\GIMP" -Recurse | Where-Object { $_.FullName -notmatch $excludes } | Sort-Object Name | Select-Object Name

        Write-Output "(INFO): comparing bundle with MSIX pseudo-installation"
        Compare-Object -ReferenceObject $bundle_files -DifferenceObject $msix_files -Property Name -PassThru
      }
  }
