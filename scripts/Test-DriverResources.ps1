param([Parameter(Mandatory=$true)][string]$PackagePath)
$ErrorActionPreference = 'Stop'
# Reflection-only inspection uses .NET Framework and never executes assembly code.
if ($PSVersionTable.PSEdition -eq 'Core') {
    & "$env:WINDIR/System32/WindowsPowerShell/v1.0/powershell.exe" -NoProfile -ExecutionPolicy Bypass -File $PSCommandPath -PackagePath $PackagePath
    if ($LASTEXITCODE -ne 0) { throw 'Driver resource inspection failed.' }
    exit 0
}
$package = (Resolve-Path -LiteralPath $PackagePath).Path
foreach ($name in 'LibreHardwareMonitorLib.dll','RAMSPDToolkit-NDD.dll') {
    $assembly = [Reflection.Assembly]::ReflectionOnlyLoadFrom((Join-Path $package $name))
    $resources = @($assembly.GetManifestResourceNames())
    if ($resources -match '(?i)WinRing|\.sys($|\.)') { throw "Embedded legacy driver in $name" }
    if ($name -eq 'LibreHardwareMonitorLib.dll') {
        if ($assembly.GetType('LibreHardwareMonitor.Hardware.Ring0',$false)) { throw 'Legacy LHM Ring0 implementation is present.' }
        if (!($resources -match 'PawnIo')) { throw 'LHM PawnIO modules are absent.' }
    } else {
        # NDD retains IWinRing0Driver and enum names for API compatibility, but
        # RELEASE_NDD excludes OLS implementation and all embedded driver files.
        # Upstream commit: 3b47b960e0830fef344624ad5e389675d5f0a1ce.
        if ($assembly.GetType('RAMSPDToolkit.Windows.Driver.Implementations.WinRing0.OLS',$false)) { throw 'RAMSPDToolkit includes an actual WinRing0 implementation.' }
        if ($resources.Count -ne 0) { throw 'Unexpected resources in the audited RAMSPDToolkit-NDD variant.' }
    }
    Write-Output "$name : checked resources and legacy driver implementation types."
}
