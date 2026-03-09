# Check architecture of executables on PATH
# Detect host architecture
$hostArch = if ((Get-WmiObject Win32_ComputerSystem).SystemType -like 'ARM64*') { 'ARM64' } else { 'X64' }
Write-Host "Host architecture: $hostArch"

# Get all executables in PATH
$executables = foreach ($p in $($env:PATH -split ';' | Where-Object { $_ -and (Test-Path $_) })) { Get-ChildItem -Path $p -Filter *.exe -ErrorAction SilentlyContinue }

foreach ($exe in $executables) {
  #if ("$exe" -like '*Windows/*' -and "$exe" -notlike '*Microsoft/*') {
  try {
    $fs = [System.IO.File]::OpenRead($exe.FullName)
    $br = New-Object System.IO.BinaryReader($fs)

    # Locate PE header
    $fs.Seek(0x3C, 'Begin') | Out-Null
    $fs.Seek($br.ReadInt32() + 4, 'Begin') | Out-Null

    # Map machine type to architecture
    $arch = switch ($br.ReadUInt16()) {
      0x014c { "x86" }
      0x8664 { "x64" }
      0xAA64 { "ARM64" }
      default { "Unknown" }
    }

    if ($arch -ne $hostArch) {
      Write-Warning "$($exe.FullName) is $arch, not native ($hostArch)"
    }

    $br.Close()
    $fs.Close()
  } catch { #do nothing
  }
}
