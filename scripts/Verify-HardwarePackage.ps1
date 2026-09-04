[CmdletBinding()]
param(
    [Parameter(Mandatory=$true)][string]$PackagePath,
    [Parameter(Mandatory=$true)][ValidateSet('x64','x86','ARM64EC')][string]$Platform,
    [ValidateSet('Release','Debug')][string]$Configuration = 'Release'
)
$ErrorActionPreference = 'Stop'
$root = Split-Path $PSScriptRoot -Parent
$package = (Resolve-Path -LiteralPath $PackagePath).Path
$rid = if ($Platform -eq 'x86') { 'win-x86' } else { 'win-x64' }
$runtime = Join-Path $root "OpenHardwareMonitorApi/RuntimeDependencies/bin/$Configuration/net472/$rid"
if (!(Test-Path -LiteralPath $runtime)) { throw 'Build the hardware runtime before validating a package.' }

function Get-PeMachine([string]$Path) {
    $bytes = [IO.File]::ReadAllBytes($Path)
    if ($bytes.Length -lt 64 -or [BitConverter]::ToUInt16($bytes,0) -ne 0x5a4d) { throw "Invalid PE: $Path" }
    $offset = [BitConverter]::ToInt32($bytes,0x3c)
    if ($offset -lt 0 -or $offset+6 -gt $bytes.Length -or [BitConverter]::ToUInt32($bytes,$offset) -ne 0x4550) { throw "Invalid PE: $Path" }
    return [BitConverter]::ToUInt16($bytes,$offset+4)
}

$expectedMachine = if ($Platform -eq 'x86') { 0x14c } else { 0x8664 }
$bridgeMachine = if ($Platform -eq 'x86') { 0x14c } else { 0x8664 }
foreach ($entry in @(@('TrafficMonitor.exe',$expectedMachine), @('OpenHardwareMonitorApi.dll',$bridgeMachine), @('LibreHardwareMonitorLib.dll',$bridgeMachine))) {
    $path = Join-Path $package $entry[0]
    if ((Get-PeMachine $path) -ne $entry[1]) { throw "Incorrect architecture: $path" }
}
if ($Platform -eq 'ARM64EC') {
    # Final ARM64EC images use AMD64 (0x8664), not the OBJ-only 0xa641.
    # Require the additional ARM64X marker reported by the Microsoft linker.
    $dumpbin = Get-Command dumpbin.exe -ErrorAction SilentlyContinue
    if ($dumpbin) { $dumpbinPath = $dumpbin.Source } else {
        $vswhere = Join-Path ${env:ProgramFiles(x86)} 'Microsoft Visual Studio/Installer/vswhere.exe'
        $vs = & $vswhere -latest -products '*' -property installationPath
        $tools = Get-ChildItem -LiteralPath (Join-Path $vs 'VC/Tools/MSVC') -Directory | Sort-Object Name -Descending | Select-Object -First 1
        $dumpbinPath = Join-Path $tools.FullName 'bin/Hostx64/x64/dumpbin.exe'
    }
    $headers = & $dumpbinPath /headers (Join-Path $package 'TrafficMonitor.exe')
    if ($LASTEXITCODE -ne 0 -or ($headers -join "`n") -notmatch '8664[^\r\n]*ARM64X') { throw 'The application lacks ARM64EC/ARM64X image metadata.' }
}
$version = [Diagnostics.FileVersionInfo]::GetVersionInfo((Join-Path $package 'LibreHardwareMonitorLib.dll')).FileVersion
if ($version -ne '0.9.6.0') { throw "Unexpected LibreHardwareMonitor version: $version" }

$files = @(Get-ChildItem -LiteralPath $runtime -Filter '*.dll' | Where-Object Name -ne 'RuntimeDependencies.dll')
$manifest = foreach ($source in $files) {
    $destination = Join-Path $package $source.Name
    $hash = (Get-FileHash -LiteralPath $source.FullName -Algorithm SHA256).Hash
    if (!(Test-Path -LiteralPath $destination) -or (Get-FileHash -LiteralPath $destination -Algorithm SHA256).Hash -ne $hash) {
        throw "Missing or mismatched runtime dependency: $($source.Name)"
    }
    [ordered]@{ file=$source.Name; sha256=$hash; version=[Diagnostics.FileVersionInfo]::GetVersionInfo($destination).FileVersion }
}
$config = Join-Path $package 'TrafficMonitor.exe.config'
if ((Get-FileHash -LiteralPath $config).Hash -ne (Get-FileHash -LiteralPath (Join-Path $runtime 'RuntimeDependencies.dll.config')).Hash) {
    throw 'TrafficMonitor.exe.config does not match the generated CLR configuration.'
}
foreach ($file in Get-ChildItem -LiteralPath $package -Recurse -File) {
    if ($file.Name -match '(?i)WinRing0|OpenHardwareMonitorLib|\.sys$') { throw "Legacy or bundled driver found: $($file.FullName)" }
    if ($file.Extension -eq '.dll' -or $file.Extension -eq '.exe') {
        $bytes = [IO.File]::ReadAllBytes($file.FullName)
        # The hash-verified NDD variant has harmless interface/enum names.
        # Still reject actual WinRing0 filenames and implementation namespaces.
        $pattern = if ($file.Name -eq 'RAMSPDToolkit-NDD.dll') { '(?i)WinRing0(x64)?\.(sys|dll|gz)|Implementations\.WinRing0' } else { '(?i)WinRing0' }
        if ([Text.Encoding]::ASCII.GetString($bytes) -match $pattern -or [Text.Encoding]::Unicode.GetString($bytes) -match $pattern) {
            throw "WinRing0 implementation or payload marker found in: $($file.FullName)"
        }
    }
    if ($file.Name -eq 'LibreHardwareMonitorLib.dll' -and $file.FullName -ne (Join-Path $package 'LibreHardwareMonitorLib.dll')) {
        throw "Additional LibreHardwareMonitor copy could shadow the upgraded runtime: $($file.FullName)"
    }
}
& (Join-Path $PSScriptRoot 'Test-DriverResources.ps1') -PackagePath $package
[ordered]@{ platform=$Platform; runtime=$rid; libreHardwareMonitor=$version; dependencies=$manifest } |
    ConvertTo-Json -Depth 5 | Set-Content -LiteralPath (Join-Path $package 'hardware-runtime-manifest.json') -Encoding UTF8
Write-Output "Verified $Platform package: LibreHardwareMonitor $version, $($files.Count) runtime DLLs, CLR config, no WinRing0 implementations/payloads or bundled kernel drivers."
