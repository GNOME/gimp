# 1. CONFIGURE MANIFEST

#### Get and Set GIMP full (major.minor.micro) version
Set-Location build\windows\store\
Get-Content -Path '..\..\..\meson.build' | Select-Object -Index 2 | Set-Content -Path .\GIMP_version-raw.txt -NoNewline
Get-Content -Path GIMP_version-raw.txt | Foreach-Object {$_ -replace "  version: '",""}  | Foreach-Object {$_ -replace "',",""} | Set-Content -Path .\GIMP_version2-manifest.txt -NoNewline
$version = Get-Content -Path 'GIMP_version2-manifest.txt' -Raw
$manifest = Get-Content -Path 'AppxManifest.xml'
$newManifest = $manifest -replace "@GIMP_VERSION@", "$version" | Set-Content -Path 'AppxManifest.xml'

#### Get GIMP major.minor version
"$version".Substring(0,4) | Set-Content -Path .\GIMP_version3-majorminor.txt -NoNewline
$majorminor = Get-Content -Path 'GIMP_version3-majorminor.txt' -Raw
$dots = ($majorminor.ToCharArray() -eq '.').count
if ($dots -eq 2) {"$version".Substring(0,3) | Set-Content -Path .\GIMP_version3-majorminor.txt -NoNewline}
$majorminor = Get-Content -Path 'GIMP_version3-majorminor.txt' -Raw

#### Match supported fileypes
Get-Content -Path '..\installer\associations.list' | Foreach-Object {"              <uap:FileType>." + $_} | Foreach-Object {$_ +  "</uap:FileType>"} | set-content -Path .\filetypes.txt
Get-Content -Path filetypes.txt | Where-Object {$_ -notmatch 'xcf'} | Set-Content -Path .\filetypes2.txt
$filetypes = Get-Content -Path 'filetypes2.txt' -Raw
$manifest = Get-Content -Path 'AppxManifest.xml'
$newManifest = $manifest -replace "@FILE_TYPES@", "$filetypes" | Set-Content -Path 'AppxManifest.xml'

#### Configure each architecture
New-Item -ItemType Directory -Path "gimp-arm64"
Copy-Item AppxManifest.xml gimp-arm64
Set-Location gimp-arm64
$manifest = Get-Content -Path 'AppxManifest.xml'
$transitionalManifest = $manifest -replace 'neutral', 'arm64' | Set-Content -Path 'AppxManifest.xml'
$transitionalManifest = Get-Content -Path 'AppxManifest.xml'
$NewtransitionalManifest = $transitionalManifest -replace '%TWAIN%', '' | Set-Content -Path 'AppxManifest.xml'
$NewtransitionalManifest = Get-Content -Path 'AppxManifest.xml'
$newManifest = $NewtransitionalManifest -replace '@ARCH@', 'X64' | Set-Content -Path 'AppxManifest.xml'
Set-Location ..\

New-Item -ItemType Directory -Path "gimp-w64"
Copy-Item AppxManifest.xml gimp-w64
Set-Location gimp-w64
$manifest = Get-Content -Path 'AppxManifest.xml'
$transitionalManifest = $manifest -replace 'neutral', 'x64' | Set-Content -Path 'AppxManifest.xml'
$transitionalManifest = Get-Content -Path 'AppxManifest.xml'
$NewtransitionalManifest = $transitionalManifest -replace '%TWAIN%', '<uap6:LoaderSearchPathEntry FolderPath="VFS\ProgramFiles@ARCH@\GIMP @MAJORMINOR@\32\bin"/>' | Set-Content -Path 'AppxManifest.xml'
$NewtransitionalManifest = Get-Content -Path 'AppxManifest.xml'
$newManifest = $NewtransitionalManifest -replace '@ARCH@', 'X64' | Set-Content -Path 'AppxManifest.xml'
Set-Location ..\

New-Item -ItemType Directory -Path "gimp-w32"
Copy-Item AppxManifest.xml gimp-w32
Set-Location gimp-w32
$manifest = Get-Content -Path 'AppxManifest.xml'
$transitionalManifest = $manifest -replace 'neutral', 'x86' | Set-Content -Path 'AppxManifest.xml'
$transitionalManifest = Get-Content -Path 'AppxManifest.xml'
$NewtransitionalManifest = $transitionalManifest -replace '%TWAIN%', '' | Set-Content -Path 'AppxManifest.xml'
$NewtransitionalManifest = Get-Content -Path 'AppxManifest.xml'
$newManifest = $NewtransitionalManifest -replace '@ARCH@', 'X86' | Set-Content -Path 'AppxManifest.xml'
Set-Location ..\

#### Set major.minor version
$manifest = Get-Content -Path 'gimp-arm64\AppxManifest.xml'
$finalManifest = $manifest -replace "@MAJORMINOR@", "$majorminor" | Set-Content -Path 'gimp-arm64\AppxManifest.xml'
$manifest = Get-Content -Path 'gimp-w64\AppxManifest.xml'
$finalManifest = $manifest -replace "@MAJORMINOR@", "$majorminor" | Set-Content -Path 'gimp-w64\AppxManifest.xml'
$manifest = Get-Content -Path 'gimp-w32\AppxManifest.xml'
$finalManifest = $manifest -replace "@MAJORMINOR@", "$majorminor" | Set-Content -Path 'gimp-w32\AppxManifest.xml'


# 2. CREATE ASSETS

Set-Location Assets

& 'C:\msys64\usr\bin\pacman.exe' --noconfirm -S mingw-w64-ucrt-x86_64-librsvg
Set-Alias -Name 'rsvg-convert' -Value 'C:\msys64\ucrt64\bin\rsvg-convert.exe'

Copy-Item ..\..\..\..\icons\Legacy\scalable\gimp-wilber.svg .\wilber.svg
Copy-Item ..\..\..\..\desktop\scalable\gimp.svg .\wilber_hihes.svg

#### Generate Universal Icons (introduced in Win 8)
rsvg-convert wilber.svg -o StoreLogo.png -w 50 -h 50
rsvg-convert wilber.svg -o StoreLogo.scale-100.png -w 50 -h 50
rsvg-convert wilber.svg -o StoreLogo.scale-125.png -w 65 -h 65 # Aproximated
rsvg-convert wilber.svg -o StoreLogo.scale-150.png -w 75 -h 75
rsvg-convert wilber.svg -o StoreLogo.scale-200.png -w 100 -h 100
rsvg-convert wilber.svg -o StoreLogo.scale-400.png -w 200 -h 200
rsvg-convert wilber.svg -o MedTile.png -w 150 -h 150
rsvg-convert wilber.svg -o MedTile.scale-100.png -w 150 -h 150
rsvg-convert wilber.svg -o MedTile.scale-125.png -w 190 -h 190 # Aproximated
rsvg-convert wilber.svg -o MedTile.scale-150.png -w 225 -h 225
rsvg-convert wilber_hihes.svg -o MedTile.scale-200.png -w 300 -h 300
rsvg-convert wilber_hihes.svg -o MedTile.scale-400.png -w 600 -h 600

#### Generate New Icons (partly introduced in late Win 10)
rsvg-convert wilber.svg -o AppList.png -w 44 -h 44
rsvg-convert wilber.svg -o AppList.scale-100.png -w 44 -h 44
rsvg-convert wilber.svg -o AppList.scale-125.png -w 55 -h 55
rsvg-convert wilber.svg -o AppList.scale-150.png -w 66 -h 66
rsvg-convert wilber.svg -o AppList.scale-200.png -w 88 -h 88
rsvg-convert wilber.svg -o AppList.scale-400.png -w 176 -h 176
rsvg-convert wilber.svg -o AppList.altform-unplated.png -w 44 -h 44
rsvg-convert wilber.svg -o AppList.altform-lightunplated.png -w 44 -h 44
rsvg-convert wilber.svg -o AppList.targetsize-44_altform-unplated.png -w 44 -h 44
rsvg-convert wilber.svg -o AppList.targetsize-44_altform-lightunplated.png -w 44 -h 44
function Generate-New-Icons {
    rsvg-convert wilber.svg -o AppList.targetsize-${size}.png -w ${size} -h ${size}
    Copy-Item AppList.targetsize-${size}.png AppList.targetsize-${size}_altform-unplated.png
    Copy-Item AppList.targetsize-${size}.png AppList.targetsize-${size}_altform-lightunplated.png
}
$size = 16
Generate-New-Icons
$size = 20
Generate-New-Icons
$size = 24
Generate-New-Icons
$size = 30
Generate-New-Icons
$size = 32
Generate-New-Icons
$size = 36
Generate-New-Icons
$size = 40
Generate-New-Icons
$size = 48
Generate-New-Icons
$size = 60
Generate-New-Icons
$size = 64
Generate-New-Icons
$size = 72
Generate-New-Icons
$size = 80
Generate-New-Icons
$size = 96
Generate-New-Icons
rsvg-convert wilber_hihes.svg -o AppList.targetsize-256.png -w 256 -h 256
    Copy-Item AppList.targetsize-256.png AppList.targetsize-256_altform-unplated.png
    Copy-Item AppList.targetsize-256.png AppList.targetsize-256_altform-lightunplated.png

#### Generate Legacy Icons (discontinued in Win 11)
rsvg-convert wilber_hihes.svg -o LargeTile.png -w 310 -h 310
rsvg-convert wilber.svg -o SmallTile.png -w 71 -h 71

#### Generate XCF icon
rsvg-convert ..\..\fileicon_medium.svg -o fileicon.png -w 44 -h 44
rsvg-convert ..\..\fileicon_cute.svg -o fileicon.targetsize-16.png -w 16 -h 16
rsvg-convert ..\..\fileicon_cute.svg -o fileicon.targetsize-20.png -w 20 -h 20
rsvg-convert ..\..\fileicon_medium.svg -o fileicon.targetsize-24.png -w 24 -h 24
rsvg-convert ..\..\fileicon_medium.svg -o fileicon.targetsize-30.png -w 30 -h 30
rsvg-convert ..\..\fileicon_medium.svg -o fileicon.targetsize-32.png -w 32 -h 32
rsvg-convert ..\..\fileicon_medium.svg -o fileicon.targetsize-36.png -w 36 -h 36
rsvg-convert ..\..\fileicon_medium.svg -o fileicon.targetsize-40.png -w 40 -h 40
rsvg-convert ..\..\fileicon_medium.svg -o fileicon.targetsize-48.png -w 48 -h 48
rsvg-convert ..\..\fileicon_glorious.svg -o fileicon.targetsize-60.png -w 60 -h 60
rsvg-convert ..\..\fileicon_glorious.svg -o fileicon.targetsize-64.png -w 64 -h 64
rsvg-convert ..\..\fileicon_glorious.svg -o fileicon.targetsize-72.png -w 72 -h 72
rsvg-convert ..\..\fileicon_glorious.svg -o fileicon.targetsize-80.png -w 80 -h 80
rsvg-convert ..\..\fileicon_glorious.svg -o fileicon.targetsize-96.png -w 96 -h 96
rsvg-convert ..\..\fileicon_glorious.svg -o fileicon.targetsize-128.png -w 128 -h 128
rsvg-convert ..\..\fileicon_glorious.svg -o fileicon.targetsize-256.png -w 256 -h 256

Remove-Item *.svg

#### Copy icons
Set-Location ..\
Copy-Item -Path "Assets\" -Destination "gimp-arm64\Assets\" -Recurse
Copy-Item -Path "Assets\" -Destination "gimp-w64\Assets\" -Recurse
Copy-Item -Path "Assets\" -Destination "gimp-w32\Assets\" -Recurse

#### Generate resources.pri
Set-Alias -Name 'makepri' -Value "C:\Program Files (x86)\Windows Kits\10\bin\10.0.22621.0\x64\makepri.exe"

Set-Location gimp-arm64
makepri createconfig /cf priconfig.xml /dq lang-en-US /pv 10.0.0
Set-Location ..\
makepri new /pr gimp-arm64 /cf gimp-arm64\priconfig.xml /of gimp-arm64
Remove-Item "gimp-arm64\priconfig.xml"

Set-Location gimp-w64
makepri createconfig /cf priconfig.xml /dq lang-en-US /pv 10.0.0
Set-Location ..\
makepri new /pr gimp-w64 /cf gimp-w64\priconfig.xml /of gimp-w64
Remove-Item "gimp-w64\priconfig.xml"

Set-Location gimp-w32
makepri createconfig /cf priconfig.xml /dq lang-en-US /pv 10.0.0
Set-Location ..\
makepri new /pr gimp-w32 /cf gimp-w32\priconfig.xml /of gimp-w32
Remove-Item "gimp-w32\priconfig.xml"


# 3. COPY GIMP FILES

$arm64 = "gimp-arm64\VFS\ProgramFilesX64\GIMP ${majorminor}"
$win64 = "gimp-w64\VFS\ProgramFilesX64\GIMP ${majorminor}"
$win32 = "gimp-w32\VFS\ProgramFilesX86\GIMP ${majorminor}"

#### Copy files into VFS folder (to support external 3P plug-ins)
Copy-Item -Path "..\..\..\gimp-arm64" -Destination "$arm64" -Recurse
Copy-Item -Path "..\..\..\gimp-w64" -Destination "$win64" -Recurse
Copy-Item -Path "..\..\..\gimp-w32" -Destination "$win32" -Recurse

#### Configure interpreters (to support internal Python, Script-Fu and Lua plug-ins)
Copy-Item $arm64\bin\python3.exe $arm64\bin\python.exe
Copy-Item Src\environ\* $arm64\lib\gimp\${majorminor}\environ\
Copy-Item Src\interpreters\* $arm64\lib\gimp\${majorminor}\interpreters\
$lua = Get-Content -Path "$arm64\lib\gimp\${majorminor}\interpreters\lua.interp"
$finalLua = $lua -replace 'jit.exe', '5.1.exe' | Set-Content -Path "$arm64\lib\gimp\${majorminor}\interpreters\lua.interp"

Copy-Item $win64\bin\python3.exe $win64\bin\python.exe
Copy-Item Src\environ\* $win64\lib\gimp\${majorminor}\environ\
Copy-Item Src\interpreters\* $win64\lib\gimp\${majorminor}\interpreters\

Copy-Item $win32\bin\python3.exe $win32\bin\python.exe
Copy-Item Src\environ\* $win32\lib\gimp\${majorminor}\environ\
Copy-Item Src\interpreters\* $win32\lib\gimp\${majorminor}\interpreters\

#### Remove uneeded files (to match the Windows Installer artifact)
$omissions = ("include\", "lib\babl-0.1\*.a", "lib\gdk-pixbuf-2.0\2.10.0\loaders\*.a", "lib\gegl-0.4\*.a", "lib\gegl-0.4\*.json", "lib\gimp\${majorminor}\modules\*.a", "lib\gimp\${majorminor}\plug-ins\test-sphere-v3\", "lib\gimp\${majorminor}\plug-ins\ts-helloworld\", "lib\gio\modules\giomodule.cache", "lib\gtk-3.0\", "share\applications\", "share\glib-2.0\codegen\", "share\glib-2.0\dtds\", "share\glib-2.0\gdb\", "share\glib-2.0\gettext\", "share\gir-1.0\", "share\man\", "share\themes\", "share\vala\", "gimp.cmd")
#TO DO: "share\ghostscript\..\doc",#
Set-Location $arm64
Remove-Item $omissions -Recurse
Set-Location ..\..\..\..\$win64
Remove-Item $omissions -Recurse
Set-Location ..\..\..\..\$win32
Remove-Item $omissions -Recurse
Set-Location ..\..\..\..\

#### Copy 32-bit TWAIN into ~~ARM64~~ and x64
#New-Item -ItemType Directory -Path "$arm64\32\bin\"
New-Item -ItemType Directory -Path "$win64\32\bin\"
#Copy-Item -Path "$win32\bin\gspawn*" -Destination "$arm64\32\bin\"
Copy-Item -Path "$win32\bin\gspawn*" -Destination "$win64\32\bin\"
#Copy-Item -Path "$win32\bin\*.dll" -Destination "$arm64\32\bin\"
Copy-Item -Path "$win32\bin\*.dll" -Destination "$win64\32\bin\"

#Copy-Item -Path "$win32\etc\fonts\" -Destination "$arm64\32\etc\fonts\" -Recurse
Copy-Item -Path "$win32\etc\fonts\" -Destination "$win64\32\etc\fonts\" -Recurse
#Copy-Item -Path "$win32\etc\gtk-3.0\" -Destination "$arm64\32\etc\gtk-3.0\" -Recurse
Copy-Item -Path "$win32\etc\gtk-3.0\" -Destination "$win64\32\etc\gtk-3.0\" -Recurse

#New-Item -ItemType Directory -Path "$arm64\32\lib\babl-0.1\"
New-Item -ItemType Directory -Path "$win64\32\lib\babl-0.1\"
#Copy-Item -Path "$win32\lib\babl-0.1\*.dll" -Destination "$arm64\32\lib\babl-0.1\" -Recurse
Copy-Item -Path "$win32\lib\babl-0.1\*.dll" -Destination "$win64\32\lib\babl-0.1\" -Recurse
#New-Item -ItemType Directory -Path "$arm64\32\lib\gdk-pixbuf-2.0\2.10.0\loaders\"
New-Item -ItemType Directory -Path "$win64\32\lib\gdk-pixbuf-2.0\2.10.0\loaders\"
#Copy-Item -Path "$win32\lib\gdk-pixbuf-2.0\2.10.0\loaders.cache" -Destination "$arm64\32\lib\gdk-pixbuf-2.0\2.10.0\" -Recurse
Copy-Item -Path "$win32\lib\gdk-pixbuf-2.0\2.10.0\loaders.cache" -Destination "$win64\32\lib\gdk-pixbuf-2.0\2.10.0\" -Recurse
#Copy-Item -Path "$win32\lib\gdk-pixbuf-2.0\2.10.0\loaders\*.dll" -Destination "$arm64\32\lib\gdk-pixbuf-2.0\2.10.0\loaders\" -Recurse
Copy-Item -Path "$win32\lib\gdk-pixbuf-2.0\2.10.0\loaders\*.dll" -Destination "$win64\32\lib\gdk-pixbuf-2.0\2.10.0\loaders\" -Recurse
#New-Item -ItemType Directory -Path "$arm64\32\lib\gegl-0.4\"
New-Item -ItemType Directory -Path "$win64\32\lib\gegl-0.4\"
#Copy-Item -Path "$win32\lib\gegl-0.4\*.dll" -Destination "$arm64\32\lib\gegl-0.4\" -Recurse
Copy-Item -Path "$win32\lib\gegl-0.4\*.dll" -Destination "$win64\32\lib\gegl-0.4\" -Recurse

#Copy-Item -Path "..\..\..\gimp-w32\share\themes\" -Destination "$arm64\32\share\themes\" -Recurse
Copy-Item -Path "..\..\..\gimp-w32\share\themes\" -Destination "$win64\32\share\themes\" -Recurse

#Copy-Item -Path "$win32\lib\gimp\$majorminor\plug-ins\twain\twain.exe" -Destination "$arm64\lib\gimp\$majorminor\plug-ins\twain\twain.exe"
Copy-Item -Path "$win32\lib\gimp\$majorminor\plug-ins\twain\twain.exe" -Destination "$win64\lib\gimp\$majorminor\plug-ins\twain\twain.exe"

#### Disable Update check (since the package is auto updated)
Add-Content $arm64\share\gimp\$majorminor\gimp-release "check-update=false"
Add-Content $win64\share\gimp\$majorminor\gimp-release "check-update=false"
Add-Content $win32\share\gimp\$majorminor\gimp-release "check-update=false"

#### Remove uncompliant files (to fix signing issues)
$corrections = ("*.debug", "*.tar")
Get-ChildItem gimp-arm64 -Recurse -Include $corrections | Remove-Item -Recurse
Get-ChildItem gimp-w64 -Recurse -Include $corrections | Remove-Item -Recurse
Get-ChildItem gimp-w32 -Recurse -Include $corrections | Remove-Item -Recurse


# 4. BUILD .MSIXBUNDLE

### Build .msix for each arch
Set-Alias -Name 'makeappx' -Value 'C:\Program Files (x86)\Windows Kits\10\App Certification Kit\MakeAppx.exe'

makeappx pack /d "gimp-arm64" /p "_TempOutput\gimp-${version}_arm64.msix"
makeappx pack /d "gimp-w64" /p "_TempOutput\gimp-${version}_w64.msix"
makeappx pack /d "gimp-w32" /p "_TempOutput\gimp-${version}_w32.msix"

### Build .msixbundle
makeappx bundle /bv "${version}.0" /d "_TempOutput" /p "_Output\gimp-${version}-local.msixbundle "

### Copy (not CA) certificate to futher signing
Copy-Item -Path "pseudo-gimp.pfx" -Destination "_Output\" -Recurse

exit