param([Parameter(Mandatory=$true)][string]$PackagePath)
$ErrorActionPreference = 'Stop'
# Reflection-only inspection uses .NET Framework and never executes assembly code.
if ($PSVersionTable.PSEdition -eq 'Core') {
    & "$env:WINDIR/System32/WindowsPowerShell/v1.0/powershell.exe" -NoProfile -ExecutionPolicy Bypass -File $PSCommandPath -PackagePath $PackagePath
    if ($LASTEXITCODE -ne 0) { throw 'Driver resource inspection failed.' }
    exit 0
}
$package = (Resolve-Path -LiteralPath $PackagePath).Path
$ramSpdPath = Join-Path $package 'RAMSPDToolkit-NDD.dll'
if (Test-Path -LiteralPath $ramSpdPath) { throw 'RAMSPDToolkit-NDD.dll is not allowed in the release package.' }
foreach ($name in 'LibreHardwareMonitorLib.dll') {
    $assembly = [Reflection.Assembly]::ReflectionOnlyLoadFrom((Join-Path $package $name))
    $resources = @($assembly.GetManifestResourceNames())
    if ($resources -match '(?i)WinRing|\.sys($|\.)') { throw "Embedded legacy driver in $name" }
    if ($assembly.GetType('LibreHardwareMonitor.Hardware.Ring0',$false)) { throw 'Legacy LHM Ring0 implementation is present.' }
    if (!($resources -match 'PawnIo')) { throw 'LHM PawnIO modules are absent.' }
    Write-Output "$name : checked resources and legacy driver implementation types."
}
