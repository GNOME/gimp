#!/usr/bin/env pwsh

# Parameters
param ($revision = "$GIMP_CI_MS_STORE",
       $wack = 'Non-WACK',
       $build_dir,
       $a64_bundle = 'gimp-clangarm64',
       $x64_bundle = 'gimp-clang64')

$ErrorActionPreference = 'Stop'
$PSNativeCommandUseErrorActionPreference = $true

if (-not $GITLAB_CI)
  {
    # Make the script work locally
    if (-not (Test-Path build\windows\store) -and -not (Test-Path 3_dist-gimp-winsdk.ps1 -Type Leaf) -or $PSScriptRoot -notlike "*build\windows\store*")
      {
        Write-Host '(ERROR): Script called from wrong dir. Please, call the script from gimp source.' -ForegroundColor Red
        exit 1
      }
    elseif (Test-Path 3_dist-gimp-winsdk.ps1 -Type Leaf)
      {
        Set-Location ..\..\..
      }

    $PARENT_DIR = '..\'
  }


# 1. AUTODECTET LATEST WINDOWS SDK AND MSSTORE-CLI
Write-Output "$([char]27)[0Ksection_start:$(Get-Date -UFormat %s -Millisecond 0):msix_tlkt$([char]13)$([char]27)[0KChecking WinSDK and other tools installation"
if ((Get-WmiObject -Class Win32_ComputerSystem).SystemType -like 'ARM64*')
  {
    $cpu_arch = 'arm64'
  }
else
  {
    $cpu_arch = 'x64'
  }
# Windows SDK
$win_sdk_version = Get-ItemProperty Registry::'HKEY_LOCAL_MACHINE\SOFTWARE\WOW6432Node\Microsoft\Microsoft SDKs\Windows\v10.0' | Select-Object -ExpandProperty ProductVersion
$win_sdk_path = Get-ItemProperty Registry::'HKEY_LOCAL_MACHINE\SOFTWARE\WOW6432Node\Microsoft\Microsoft SDKs\Windows\v10.0' | Select-Object -ExpandProperty InstallationFolder
$env:Path = "${win_sdk_path}bin\${win_sdk_version}.0\$cpu_arch;${win_sdk_path}App Certification Kit;" + $env:Path
# msstore-cli (ONLY FOR RELEASES)
if ("$CI_COMMIT_TAG" -eq (git describe --all | Foreach-Object {$_ -replace 'tags/',''}))
  {
    [Net.ServicePointManager]::SecurityProtocol = [Net.SecurityProtocolType]::Tls12

    $msstore_tag = (Invoke-WebRequest https://api.github.com/repos/microsoft/msstore-cli/releases | ConvertFrom-Json)[0].tag_name
    $xmlObject = New-Object XML
    $xmlObject.Load("https://raw.githubusercontent.com/microsoft/msstore-cli/refs/heads/rel/$msstore_tag/MSStore.API/MSStore.API.csproj")
    $dotnet_major = ($xmlObject.Project.PropertyGroup.TargetFramework | Out-String) -replace "`r`n",'' -replace 'net',''
    $dotnet_tag = ((Invoke-WebRequest https://api.github.com/repos/dotnet/runtime/releases | ConvertFrom-Json).tag_name | Select-String "$dotnet_major" | Select-Object -First 1).ToString() -replace 'v',''

    if (-not (Test-Path "$Env:ProgramFiles\dotnet\shared\Microsoft.NETCore.App\$dotnet_major*\"))
      {
        Write-Output "(INFO): downloading .NET v$dotnet_tag"
        Invoke-WebRequest https://aka.ms/dotnet/$dotnet_major/dotnet-runtime-win-$cpu_arch.zip -OutFile ${PARENT_DIR}dotnet-runtime.zip
        Expand-Archive ${PARENT_DIR}dotnet-runtime.zip ${PARENT_DIR}dotnet-runtime -Force
        $env:Path = "$(Resolve-Path $PWD\${PARENT_DIR}dotnet-runtime);" + $env:Path
        $env:DOTNET_ROOT = "$(Resolve-Path $PWD\${PARENT_DIR}dotnet-runtime)"
      }

    if (-not (Test-Path Registry::'HKEY_CURRENT_USER\Software\Microsoft\Windows\CurrentVersion\App Paths\MSStore.exe'))
      {
        Write-Output "(INFO): downloading MSStoreCLI $msstore_tag"
        Invoke-WebRequest https://github.com/microsoft/msstore-cli/releases/download/$msstore_tag/MSStoreCLI-win-$cpu_arch.zip -OutFile ${PARENT_DIR}MSStoreCLI.zip
        Expand-Archive ${PARENT_DIR}MSStoreCLI.zip ${PARENT_DIR}MSStoreCLI -Force
        $env:Path = "$(Resolve-Path $PWD\${PARENT_DIR}MSStoreCLI);" + $env:Path
      }
    $msstore_text = " | Installed MSStoreCLI: $(msstore --version)"
  }
Write-Output "(INFO): Installed WinSDK: ${win_sdk_version}${msstore_text}"
Write-Output "$([char]27)[0Ksection_end:$(Get-Date -UFormat %s -Millisecond 0):msix_tlkt$([char]13)$([char]27)[0K"


# 2. GLOBAL VARIABLES
Write-Output "$([char]27)[0Ksection_start:$(Get-Date -UFormat %s -Millisecond 0):msix_info$([char]13)$([char]27)[0KGetting MSIX global info"
if (-not $build_dir)
  {
    $build_dir = Get-ChildItem _build* | Select-Object -First 1
  }
$config_path = "$build_dir\config.h"
if (-not (Test-Path "$config_path"))
  {
    Write-Host "(ERROR): config.h file not found. You can run 'build/windows/2_build-gimp-msys2.ps1' or configure GIMP to generate it." -ForegroundColor red
    exit 1
  }
ForEach ($line in $(Select-String 'define' $config_path -AllMatches))
  {
    Invoke-Expression $($line -replace '^.*#' -replace 'define ','$' -replace ' ','=')
  }

## Get Identity Name (the dir shown in Explorer)
if (-not $GIMP_RELEASE -or $GIMP_IS_RC_GIT)
  {
    $IDENTITY_NAME="GIMP.GIMPInsider"
  }
elseif ($GIMP_RELEASE -and $GIMP_UNSTABLE -or $GIMP_RC_VERSION)
  {
    $IDENTITY_NAME="GIMP.GIMPPreview"
  }
else
  {
    $IDENTITY_NAME="GIMP.43237F745459"
  }

## Get custom GIMP version (major.minor.micro+revision.0), a compliant way to publish to Partner Center:
## https://learn.microsoft.com/en-us/windows/apps/publish/publish-your-app/msix/app-package-requirements)
### Get GIMP app version (major.minor)
$major = ($GIMP_APP_VERSION.Split('.'))[0]
$minor = ($GIMP_APP_VERSION.Split('.'))[-1]
### Get GIMP micro version
$micro = ($GIMP_VERSION.Split('.'))[-1] -replace '(.+?)-.+','$1'
if ($micro -ne '0')
  {
    $micro_digit = $micro
  }
### Get GIMP revision
if ($revision -match '[1-9]' -and $CI_PIPELINE_SOURCE -ne 'schedule')
  {
    $revision_text = ", revision: $revision"
  }
elseif ($GIMP_RC_VERSION)
  {
    $revision = $GIMP_RC_VERSION
    $revision_text = ", RC: $revision"
  }
else
  {
    $wack = "$revision"
    $revision = "0"
  }
$CUSTOM_GIMP_VERSION = "$GIMP_APP_VERSION.${micro_digit}${revision}.0"

Write-Output "(INFO): Identity: $IDENTITY_NAME | Version: $CUSTOM_GIMP_VERSION (major: $major, minor: $minor, micro: ${micro}${revision_text})"

## Autodetects what arch bundles will be packaged
if (-not (Test-Path "$a64_bundle") -and -not (Test-Path "$x64_bundle"))
  {
    Write-Host "(ERROR): No bundle found. You can tweak 'build/windows/2_build-gimp-msys2.ps1' or configure GIMP with '-Dms-store=true' to make one." -ForegroundColor red
    exit 1
  }
elseif ((Test-Path "$a64_bundle") -and -not (Test-Path "$x64_bundle"))
  {
    Write-Output "(INFO): Arch: arm64"
  }
elseif (-not (Test-Path "$a64_bundle") -and (Test-Path "$x64_bundle"))
  {
    Write-Output "(INFO): Arch: x64"
  }
elseif ((Test-Path "$a64_bundle") -and (Test-Path "$x64_bundle"))
  {
    Write-Output "(INFO): Arch: arm64 and x64"
  }
Write-Output "$([char]27)[0Ksection_end:$(Get-Date -UFormat %s -Millisecond 0):msix_info$([char]13)$([char]27)[0K"


$supported_archs = "$a64_bundle","$x64_bundle"
foreach ($bundle in $supported_archs)
  {
    if (Test-Path "$bundle")
      {
        if ((Test-Path $a64_bundle) -and (Test-Path $x64_bundle))
          {
            $temp_text='temporary '
          }
        if (("$bundle" -like '*a64*') -or ("$bundle" -like '*aarch64*') -or ("$bundle" -like '*arm64*'))
          {
            $msix_arch = 'arm64'
          }
        else
          {
            $msix_arch = 'x64'
          }
        Write-Output "$([char]27)[0Ksection_start:$(Get-Date -UFormat %s -Millisecond 0):${msix_arch}_making[collapsed=true]$([char]13)$([char]27)[0KMaking ${temp_text}$msix_arch MSIX"

        ## Prevent Git going crazy
        $ig_content = "`n$msix_arch`n*.appxsym`n*.zip"
        if (Test-Path .gitignore -Type Leaf)
          {
            if (-not (Test-Path .gitignore.bak -Type Leaf))
              {
                Copy-Item .gitignore .gitignore.bak
              }
            Add-Content .gitignore "$ig_content"
          }
        else
          {
            New-Item .gitignore | Out-Null
            Set-Content .gitignore "$ig_content"
          }

        ## Create temporary dir
        if (Test-Path $msix_arch)
          {
            Remove-Item $msix_arch/ -Recurse
          }
        New-Item $msix_arch -ItemType Directory | Out-Null


        # 3. PREPARE MSIX "SOURCE"

        ## 3.1. CONFIGURE MANIFEST
        Write-Output "(INFO): configuring AppxManifest.xml for $msix_arch"
        Copy-Item build\windows\store\AppxManifest.xml $msix_arch
        ### Set msix_arch
        (Get-Content $msix_arch\AppxManifest.xml) | Foreach-Object {$_ -replace "neutral","$msix_arch"} |
        Set-Content $msix_arch\AppxManifest.xml
        ### Set Identity Name
        (Get-Content $msix_arch\AppxManifest.xml) | Foreach-Object {$_ -replace "@IDENTITY_NAME@","$IDENTITY_NAME"} |
        Set-Content $msix_arch\AppxManifest.xml
        ### Set Display Name (the name shown in MS Store)
        if (-not $GIMP_RELEASE -or $GIMP_IS_RC_GIT)
          {
            $display_name='GIMP (Insider)'
          }
        elseif (($GIMP_RELEASE -and $GIMP_UNSTABLE) -or $GIMP_RC_VERSION)
          {
            $display_name='GIMP (Preview)'
          }
        else
          {
            $display_name='GIMP'
          }
        (Get-Content $msix_arch\AppxManifest.xml) | Foreach-Object {$_ -replace "@DISPLAY_NAME@","$display_name"} |
        Set-Content $msix_arch\AppxManifest.xml
        ### Set custom GIMP version (major.minor.micro+revision.0)
        (Get-Content $msix_arch\AppxManifest.xml) | Foreach-Object {$_ -replace "@CUSTOM_GIMP_VERSION@","$CUSTOM_GIMP_VERSION"} |
        Set-Content $msix_arch\AppxManifest.xml
        ### Set GIMP mutex version (major.minor or major)
        if ($GIMP_UNSTABLE)
          {
            $gimp_mutex_version="$GIMP_APP_VERSION"
          }
        else
          {
            $gimp_mutex_version="$major"
          }
        (Get-Content $msix_arch\AppxManifest.xml) | Foreach-Object {$_ -replace "@GIMP_MUTEX_VERSION@","$gimp_mutex_version"} |
        Set-Content $msix_arch\AppxManifest.xml
        ### Match supported filetypes
        $file_types = Get-Content 'build\windows\installer\data_associations.list'       | Foreach-Object {"              <uap:FileType>." + $_} |
                      Foreach-Object {$_ +  "</uap:FileType>"}                           | Where-Object {$_ -notmatch 'xcf'}
        (Get-Content $msix_arch\AppxManifest.xml) | Foreach-Object {$_ -replace "@FILE_TYPES@","$file_types"}  |
        Set-Content $msix_arch\AppxManifest.xml

        ## 3.2. CREATE ICON ASSETS
        $icons_path = "$build_dir\build\windows\store\Assets"
        if (-not (Test-Path "$icons_path"))
          {
            Write-Host "(ERROR): MS Store icons not found. You can tweak 'build/windows/2_build-gimp-msys2.ps1' or configure GIMP with '-Dms-store=true' to build them." -ForegroundColor red
            exit 1
          }
        Write-Output "(INFO): generating resources*.pri from $icons_path"
        ### Copy pre-generated icons to each msix_arch
        New-Item $msix_arch\Assets -ItemType Directory | Out-Null
        Copy-Item "$icons_path\*.png" $msix_arch\Assets\ -Recurse
        ### Generate resources*.pri
        Set-Location $msix_arch
        makepri createconfig /cf priconfig.xml /dq lang-en-US /pv 10.0.0 | Out-File ..\winsdk.log
        Set-Location ..\
        makepri new /pr $msix_arch /cf $msix_arch\priconfig.xml /of $msix_arch | Out-File winsdk.log -Append
        Remove-Item $msix_arch\priconfig.xml


        # 4. COPY GIMP FILES
        Write-Output "(INFO): preparing GIMP files in $msix_arch VFS"
        $vfs = "$msix_arch\VFS\ProgramFilesX64\GIMP"

        ## Copy files into VFS folder (to support external 3P plug-ins)
        Copy-Item "$bundle" "$vfs" -Recurse -Force

        ## Set revision on about dialog (this does the same as '-Drevision' build option)
        if (-not $GIMP_RC_VERSION)
          {
            (Get-Content "$vfs\share\gimp\*\gimp-release") | Foreach-Object {$_ -replace "revision=0","revision=$revision"} |
            Set-Content "$vfs\share\gimp\*\gimp-release"
          }

        ## Disable Update check (ONLY FOR RELEASES)
        if ($GIMP_RELEASE -and -not $GIMP_IS_RC_GIT)
          {
            Add-Content "$vfs\share\gimp\*\gimp-release" 'check-update=false'
          }

        ## Remove uneeded files (to match the Inno Windows Installer artifact)
        Get-ChildItem "$vfs" -Recurse -Include (".gitignore", "gimp.cmd") | Remove-Item -Recurse


        # 5.A. MAKE .MSIX AND CORRESPONDING .APPXSYM

        ## Make .appxsym for each msix_arch (ONLY FOR RELEASES)
        $APPXSYM = "${IDENTITY_NAME}_${CUSTOM_GIMP_VERSION}_$msix_arch.appxsym"
        if ($CI_COMMIT_TAG -match 'GIMP_[0-9]*_[0-9]*_[0-9]*' -or $GIMP_CI_MS_STORE -like 'MSIXUPLOAD*')
          {
            Write-Output "(INFO): making $APPXSYM"
            Get-ChildItem $msix_arch -Filter *.pdb -Recurse | Compress-Archive -DestinationPath "$APPXSYM.zip"
            Get-ChildItem *.zip | Rename-Item -NewName $APPXSYM
            Get-ChildItem $msix_arch -Include *.pdb -Recurse -Force | Remove-Item -Recurse -Force
          }

        ## Make .msix from each msix_arch
        $MSIX_ARTIFACT = $APPXSYM -replace '.appxsym','.msix'
        Write-Output "(INFO): packaging $MSIX_ARTIFACT"
        makeappx pack /d $msix_arch /p $MSIX_ARTIFACT /o | Out-File winsdk.log -Append
        Remove-Item $msix_arch/ -Recurse
        Write-Output "$([char]27)[0Ksection_end:$(Get-Date -UFormat %s -Millisecond 0):${msix_arch}_making$([char]13)$([char]27)[0K"
      } #END of 'if (Test-Path...'
  } #END of 'foreach ($msix_arch...'


# 5.B. MAKE .MSIXBUNDLE OR SUBSEQUENT .MSIXUPLOAD
if (((Test-Path $a64_bundle) -and (Test-Path $x64_bundle)) -and (Get-ChildItem *.msix -Recurse).Count -gt 1)
  {
    $MSIXBUNDLE = "${IDENTITY_NAME}_${CUSTOM_GIMP_VERSION}_neutral.msixbundle"
    $MSIX_ARTIFACT = "$MSIXBUNDLE"
    if ($GIMP_RELEASE -and -not $GIMP_IS_RC_GIT)
      {
        $MSIXUPLOAD = "${IDENTITY_NAME}_${CUSTOM_GIMP_VERSION}_x64_arm64_bundle.msixupload"
        $MSIX_ARTIFACT = "$MSIXUPLOAD"
      }
    Write-Output "$([char]27)[0Ksection_start:$(Get-Date -UFormat %s -Millisecond 0):msix_making[collapsed=true]$([char]13)$([char]27)[0KPackaging $MSIX_ARTIFACT"

    ## Make .msixbundle with all archs
    ## (This is needed not only for easier multi-arch testing but
    ##  also to make sure against Partner Center getting confused)
    New-Item _TempOutput -ItemType Directory | Out-Null
    Move-Item *.msix _TempOutput/
    makeappx bundle /bv "${CUSTOM_GIMP_VERSION}" /d _TempOutput /p $MSIXBUNDLE /o
    Remove-Item _TempOutput/ -Recurse

    ## Make .msixupload (ONLY FOR RELEASES)
    if ($GIMP_RELEASE -and -not $GIMP_IS_RC_GIT)
      {
        Write-Output "(INFO): creating $MSIXUPLOAD for submission"
        Compress-Archive -Path "*.appxsym","*.msixbundle" -DestinationPath "$MSIXUPLOAD.zip"
        Get-ChildItem ${IDENTITY_NAME}*.zip | Rename-Item -NewName $MSIXUPLOAD
        Remove-Item *.appxsym -Force
        Remove-Item *.msixbundle -Force
      }
    Write-Output "$([char]27)[0Ksection_end:$(Get-Date -UFormat %s -Millisecond 0):msix_making$([char]13)$([char]27)[0K"
  }

Remove-Item .gitignore
if (Test-Path .gitignore.bak -Type Leaf)
  {
    Rename-Item .gitignore.bak .gitignore
  }


# 6.A. CERTIFY .MSIX OR .MSIXBUNDLE WITH WACK (OPTIONAL)
# (Partner Center does the same thing before publishing)
if (-not $GITLAB_CI -and $wack -eq 'WACK')
  {
    ## Prepare file naming
    ## (appcert CLI does NOT allow relative paths)
    $fullpath = $PWD
    ## (appcert CLI does NOT allow more than one dot on xml name)
    $xml_artifact = "$MSIX_ARTIFACT" -replace '.msix', '-report.xml' -replace 'bundle', ''

    ## Generate detailed report
    ## (appcert only works with admin rights so let's use sudo)
    $nt_build = [System.Environment]::OSVersion.Version | Select-Object -ExpandProperty Build
    if ($nt_build -lt '26052')
      {
        Write-Host "(ERROR): Certification from CLI requires 'sudo' (available only for build 10.0.26052.0 and above)" -ForegroundColor Red
        exit 1
      }
    $sudo_mode = Get-ItemProperty Registry::'HKEY_LOCAL_MACHINE\SOFTWARE\Microsoft\Windows\CurrentVersion\Sudo' | Select-Object -ExpandProperty Enabled
    if ($sudo_mode -eq '0')
      {
        Write-Host "(ERROR): 'sudo' is disabled. Please enable it in Settings." -ForegroundColor Red
        exit 1
      }
    elseif ($sudo_mode -ne '3')
      {
        Write-Host "(ERROR): 'sudo' is not in normal/inline mode. Please change it in Settings." -ForegroundColor Red
        exit 1
      }
    Write-Output "(INFO): certifying $MSIX_ARTIFACT with WACK"
    sudo appcert test -appxpackagepath $fullpath\$MSIX_ARTIFACT -reportoutputpath $fullpath\$xml_artifact

    ## Output overall result
    if (Test-Path $xml_artifact -Type Leaf)
      {
        $xmlObject = New-Object XML
        $xmlObject.Load("$xml_artifact")
        $result = $xmlObject.REPORT.OVERALL_RESULT
        if ($result -eq 'FAIL')
          {
            Write-Host "(ERROR): $MSIX_ARTIFACT not passed. See: $xml_artifact" -ForegroundColor Red
            exit 1
          }
        elseif ($result -eq 'WARNING')
          {
            Write-Host "(WARNING): $MSIX_ARTIFACT passed partially. See: $xml_artifact" -ForegroundColor Yellow
          }
        #elseif ($result -eq 'PASS')
          #{
            # Output nothing
          #}
      }
  }


# 6.B. SIGN .MSIX OR .MSIXBUNDLE PACKAGE (NOT THE BINARIES)
# (Partner Center does the same thing, for free, before publishing)
if (-not $GIMP_RELEASE -or $GIMP_IS_RC_GIT)
  {
    Write-Output "$([char]27)[0Ksection_start:$(Get-Date -UFormat %s -Millisecond 0):msix_trust${msix_arch}[collapsed=true]$([char]13)$([char]27)[0KSelf-signing $MSIX_ARTIFACT (for testing purposes)"
    signtool sign /debug /fd sha256 /a /f $(Resolve-Path build\windows\store\pseudo-gimp*.pfx) /p eek $MSIX_ARTIFACT
    if ("$LASTEXITCODE" -gt '0' -or "$?" -eq 'False')
      {
        ## We need to manually check failures in pre-7.4 PS
        $pseudo_gimp = "pseudo-gimp_$(Get-Date -UFormat %s -Millisecond 0)"
        New-SelfSignedCertificate -Type Custom -Subject "$(([xml](Get-Content build\windows\store\AppxManifest.xml)).Package.Identity.Publisher)" -KeyUsage DigitalSignature -FriendlyName "$pseudo_gimp" -CertStoreLocation "Cert:\CurrentUser\My" -TextExtension @("2.5.29.37={text}1.3.6.1.5.5.7.3.3", "2.5.29.19={text}") | Out-Null
        Export-PfxCertificate -Cert "Cert:\CurrentUser\My\$(Get-ChildItem Cert:\CurrentUser\My | Where-Object FriendlyName -EQ "$pseudo_gimp" | Select-Object -ExpandProperty Thumbprint)" -FilePath "build/windows/store/${pseudo_gimp}.pfx" -Password (ConvertTo-SecureString -String eek -Force -AsPlainText) | Out-Null
        Remove-Item "Cert:\CurrentUser\My\$(Get-ChildItem Cert:\CurrentUser\My | Where-Object FriendlyName -EQ "$pseudo_gimp" | Select-Object -ExpandProperty Thumbprint)"
        if ($GITLAB_CI)
          {
            $OUTPUT_DIR = 'build\windows\store\_Output\'
            $OUTPUT_DIR_PARTIAL = '_Output\'
            New-Item $OUTPUT_DIR -ItemType Directory -Force | Out-Null
            Move-Item build\windows\store\$pseudo_gimp.pfx $OUTPUT_DIR
          }
        Write-Host "(ERROR): Self-signing certificate expired. Please commit the generated '${OUTPUT_DIR}${pseudo_gimp}.pfx' on 'build\windows\store\'." -ForegroundColor red
        $sha256 = (Get-FileHash build\windows\store\${OUTPUT_DIR_PARTIAL}${pseudo_gimp}.pfx -Algorithm SHA256 | Select-Object -ExpandProperty Hash).ToLower()
        Write-Output "(INFO): ${pseudo_gimp}.pfx SHA-256: $sha256"
        exit 1
      }
    Copy-Item build\windows\store\pseudo-gimp*.pfx pseudo-gimp.pfx -Recurse

    ## Generate checksums
    $sha256 = (Get-FileHash $MSIX_ARTIFACT -Algorithm SHA256 | Select-Object -ExpandProperty Hash).ToLower()
    Write-Output "(INFO): $MSIX_ARTIFACT SHA-256: $sha256"
    $sha512 = (Get-FileHash $MSIX_ARTIFACT -Algorithm SHA512 | Select-Object -ExpandProperty Hash).ToLower()
    Write-Output "(INFO): $MSIX_ARTIFACT SHA-512: $sha512"
    Write-Output "$([char]27)[0Ksection_end:$(Get-Date -UFormat %s -Millisecond 0):msix_trust${msix_arch}$([char]13)$([char]27)[0K"
  }


if ($GITLAB_CI)
  {
    # GitLab doesn't support wildcards when using "expose_as" so let's move to a dir
    $OUTPUT_DIR = "$PWD\build\windows\store\_Output"
    New-Item $OUTPUT_DIR -ItemType Directory -Force | Out-Null
    Move-Item $MSIX_ARTIFACT $OUTPUT_DIR -Force
    if (-not $GIMP_RELEASE -or $GIMP_IS_RC_GIT)
      {
        Copy-Item pseudo-gimp.pfx $OUTPUT_DIR
      }
  }


# 7. SUBMIT .MSIXUPLOAD TO MS STORE (ONLY FOR RELEASES)
if ("$CI_COMMIT_TAG" -eq (git describe --all | Foreach-Object {$_ -replace 'tags/',''}))
  {
    Write-Output "$([char]27)[0Ksection_start:$(Get-Date -UFormat %s -Millisecond 0):msix_submission[collapsed=true]$([char]13)$([char]27)[0KSubmitting $MSIX_ARTIFACT to Microsoft Store"
    ## Connect with Microsoft Entra credentials (stored on GitLab)
    ## (The last one can be revoked at any time in MS Entra Admin center)
    msstore reconfigure --tenantId $TENANT_ID --sellerId $SELLER_ID --clientId $CLIENT_ID --clientSecret $CLIENT_SECRET

    ## Set product_id (which is not confidential) needed by HTTP calls of some commands
    if ($GIMP_UNSTABLE -or $GIMP_RC_VERSION)
      {
        $PRODUCT_ID="9NZVDVP54JMR"
      }
    else
      {
        $PRODUCT_ID="9PNSJCLXDZ0V"
      }

    ## Create submission and upload .msixupload file to it
    msstore publish $OUTPUT_DIR\$MSIX_ARTIFACT -id $PRODUCT_ID -nc

    ## FIXME: Update submission info. See: https://github.com/microsoft/msstore-cli/issues/70
    ###Get changelog from Linux appdata
    #$xmlObject = New-Object XML
    #$xmlObject.Load("desktop\org.gimp.GIMP.appdata.xml.in.in")
    #if ($xmlObject.component.releases.release[0].version -ne ("$GIMP_VERSION".ToLower() -replace '-','~'))
    #  {
    #    Write-Host "(ERROR): appdata does not match main meson file. Submission can't be done." -ForegroundColor red
    #    exit 1
    #  }
    #$store_changelog = ''
    #foreach ($p in $xmlObject.component.releases.release[0].description.p){$store_changelog += "$p"}
    #foreach ($li in $xmlObject.component.releases.release[0].description.ul.li){$store_changelog += "- $li`n"}
    #$store_changelog = $store_changelog -replace "  ",'' -replace '(?m)^\s*?\n' -replace '"',"'"
    ###Prepare submission info
    #$jsonObject = msstore submission getListingAssets $PRODUCT_ID | Select-Object -Skip 5 | ConvertFrom-Json
    #$jsonObject.'ReleaseNotes' = "$store_changelog"
    #$full_jsonObject = msstore submission get $PRODUCT_ID | Select-Object -Skip 3 | ConvertFrom-Json
    #$full_jsonObject."Listings"."en-us"."BaseListing" = ($jsonObject | ConvertTo-Json -Compress)
    ###Fix submission info against JSON scaping hell
    #$final_json = $full_jsonObject | ConvertTo-Json -Compress
    #Fix excessive backslashes
    #$final_json = $final_json -replace "\\\\",'\' -replace "\\\\",'\'
    #Scape spaces and new lines
    #$final_json = $final_json -replace ' ','\b' -replace '`n','\n'
    #PowerShell converts 'BaseListing' wrongly
    #$final_json = $final_json -replace '"{','{' -replace '}"','}'
    #$final_json = $final_json -replace '\\"Title\\":\\"GIMP\\b\(Preview\)\\"','\"Title\":\"GIMP\b\r\n(Preview)\"'
    #PowerShell converts 'PlatformOverrides' wrongly
    #$final_json = $final_json -replace '""','{}'
    #PowerShell destroys 'Genres' array
    #$final_json = $final_json -replace '\"Genres\":\"MultimediaDesign_IllustrationAndGraphicDesign\"','"Genres":["MultimediaDesign_IllustrationAndGraphicDesign"]'
    #PowerShell converts 'Warning' wrongly
    #$final_json = $final_json -replace '\"@{','{"' -replace 'Code=','Code":"' -replace ';\\b','","'
    #$final_json = $final_json -replace 'Details=','Details":"' -replace '\.}]','."}]'
    #PowerShell destroys 'Languages' array
    #$final_json = $final_json -replace '\"Languages\":{}','"Languages":[]'
    #$final_json = $final_json -replace '\"Languages\":\"en-US\"','"Languages":["en-US"]'
    #PowerShell destroys 'Capabilities' array
    #$final_json = $final_json -replace '\"Capabilities\":{}','"Capabilities":[]'
    #$final_json = $final_json -replace 'Capabilities\":\"','Capabilities":["' -replace '\\brunFullTrust\",','","runFullTrust"],'
    ##Scape double quotes
    #$final_json = $final_json -replace '"','\"'
    #Fix excessive backslashes again
    #$final_json = $final_json -replace "\\\\",'\' -replace "\\\\",'\'
    #msstore submission update $PRODUCT_ID $final_json

    ## Start certification then publishing
    msstore submission publish $PRODUCT_ID
    Write-Output "$([char]27)[0Ksection_end:$(Get-Date -UFormat %s -Millisecond 0):msix_submission$([char]13)$([char]27)[0K"
  }
