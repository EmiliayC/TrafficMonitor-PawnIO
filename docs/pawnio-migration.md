# LibreHardwareMonitor / PawnIO migration

Release scope: x64 and x86. ARM64/ARM64EC commands below are optional reference only and have not been validated.

This change migrates the standard edition from the vendored LibreHardwareMonitor
0.9.4 DLL to the official `LibreHardwareMonitorLib` NuGet package **0.9.6**, targeting
.NET Framework **4.7.2**. The native `IOpenHardwareMonitor` ABI is unchanged.
The existing upstream Lite release workflow remains independent of this dependency.

## Runtime requirements and deployment

- Install the official signed [PawnIO](https://pawnio.eu/) driver, version **2.0 or
  later**, and run the standard edition as administrator. This is a system driver,
  not a DLL to copy into the application directory. TrafficMonitor neither installs
  a driver silently nor falls back to WinRing0. Restart TrafficMonitor after driver
  installation or upgrade (the library caches installation/version discovery).
- .NET Framework 4.7.2 or later and the corresponding Visual C++ runtime are needed.
- Distribute `TrafficMonitor.exe`, `OpenHardwareMonitorApi.dll`, **all runtime DLLs**
  copied by the build, and **TrafficMonitor.exe.config** together. Preserve the usual
  application skins/language assets. Do not replace only LibreHardwareMonitorLib.dll.
- Use a clean release directory. Do not mix older standard-edition files or a plugin's
  old copy of LibreHardwareMonitor with this runtime. This patch does not migrate
  the separate TrafficMonitorPlugins repository.
- Missing, outdated or inaccessible PawnIO prevents hardware-monitor initialization
  and gives an installation/access message. Network and other native monitoring are
  independent. During sampling, access failure clears the current hardware readings;
  repeated identical failures produce only one dialog until recovery or a new error.
  Unavailable sensors remain `-1` (the application's existing unavailable sentinel),
  rather than fabricated zero values. A valid zero remains valid.

## Reproducible dependency restoration

`OpenHardwareMonitorApi/RuntimeDependencies/RuntimeDependencies.csproj` pins the
package to `[0.9.6]`. `packages.lock.json` records exact transitive versions and NuGet
content hashes. `HardwareRuntime.targets` runs locked restore/build automatically,
then copies the runtime assets and generated CLR configuration. NuGet resolves the
framework-provided assemblies; not every PackageReference results in a separate DLL.
Never ship `ref/net472/LibreHardwareMonitorLib.dll`: it is a reference assembly, not
an implementation.

| Solution platform | Bridge / LHM runtime |
| --- | --- |
| x64 | x64 / `win-x64` |
| x86 (VC project: Win32) | x86 / `win-x86` |
| ARM64EC | existing x64 bridge mapping / `win-x64`; copied into ARM64EC output |

To intentionally update dependencies, edit the exact version, run `dotnet restore`
with `--force-evaluate` on RuntimeDependencies.csproj, review the lockfile changes,
then repeat all checks below. Normal builds must keep `RestoreLockedMode=true`.

## Build

Use a VS 2022 developer PowerShell with the v143 toolset, C++/CLI support, the .NET
Framework 4.7.2 targeting pack, a Windows SDK and MFC for each target architecture.
The helper project also requires a .NET SDK; SDK 9.0.300 was used for local validation.
ARM64EC additionally needs both VC.Tools.ARM64 and VC.Tools.ARM64EC, plus ARM64 MFC components.

```powershell
msbuild TrafficMonitor.sln /m /nr:false /p:Configuration=Release /p:Platform=x64
./scripts/Verify-HardwarePackage.ps1 -PackagePath Bin/x64/Release -Platform x64

msbuild TrafficMonitor.sln /m /nr:false /p:Configuration=Release /p:Platform=x86
./scripts/Verify-HardwarePackage.ps1 -PackagePath Bin/Release -Platform x86

msbuild TrafficMonitor.sln /m /nr:false /p:Configuration=Release /p:Platform=ARM64EC
./scripts/Verify-HardwarePackage.ps1 -PackagePath Bin/ARM64EC/Release -Platform ARM64EC
```

Build different platforms sequentially: the small dependency project shares its
NuGet intermediate directory. CI uses a separate machine for each platform.

## Verification

```powershell
msbuild tests/HardwareMonitorTests.vcxproj /nr:false /p:Configuration=Release /p:Platform=x64
./tests/bin/x64/HardwareMonitorTests.exe
msbuild tests/NativeHostSmoke.vcxproj /nr:false /p:Configuration=Release /p:Platform=x64
# On a machine without accessible PawnIO (including an ordinary non-elevated process):
./tests/bin/x64/native/NativeHostSmoke.exe --expect-unavailable
# On a test machine with PawnIO, from an elevated prompt (three real samples):
./tests/bin/x64/native/NativeHostSmoke.exe
```

Repeat with VC platform `Win32` and the `tests/bin/Win32` paths for x86. The sensor
regression tests need no driver and make no hardware accesses. They cover missing
and old PawnIO versions, nullable and non-finite readings, removal of old samples,
empty clock collections, real zero readings, CPU Total selection and subhardware.
The native smoke executable links to the actual built bridge import library and
runs beside DLLs/config copied from the application package. It checks the native
host/CLR boundary as well as sampling and shutdown.

`Verify-HardwarePackage.ps1` checks executable/bridge/LHM architecture, version,
exact dependency hashes, CLR configuration, duplicate LHM copies and driver payloads.
It emits `hardware-runtime-manifest.json`. `Test-DriverResources.ps1` uses .NET
Framework reflection-only inspection, without executing the inspected assemblies.

### RAMSPDToolkit-NDD audit

The pinned NDD variant retains an `IWinRing0Driver` interface and a `WinRing0` enum
member. Those strings are not a driver. At upstream commit
`3b47b960e0830fef344624ad5e389675d5f0a1ce`, `RELEASE_NDD` excludes the OLS driver
implementation and embedded driver payloads. Verification requires the exact
restored NDD binary hash, no embedded resources and no OLS implementation; actual
WinRing0 filenames/implementation markers remain forbidden. LHM must have PawnIO
module resources and no old Ring0 implementation.

Sources:
- [LHM 0.9.6 package project](https://github.com/LibreHardwareMonitor/LibreHardwareMonitor/blob/v0.9.6/LibreHardwareMonitorLib/LibreHardwareMonitorLib.csproj)
- [LHM 0.9.6 PawnIO implementation](https://github.com/LibreHardwareMonitor/LibreHardwareMonitor/blob/v0.9.6/LibreHardwareMonitorLib/PawnIo/PawnIo.cs)
- [LHM driver's minimum-version check](https://github.com/LibreHardwareMonitor/LibreHardwareMonitor/blob/v0.9.6/LibreHardwareMonitor/UI/MainForm.cs)
- [RAMSPDToolkit NDD build conditions](https://github.com/Blacktempel/RAMSPDToolkit/blob/3b47b960e0830fef344624ad5e389675d5f0a1ce/RAMSPDToolkit/RAMSPDToolkit.csproj)
- [RAMSPDToolkit driver factory](https://github.com/Blacktempel/RAMSPDToolkit/blob/3b47b960e0830fef344624ad5e389675d5f0a1ce/RAMSPDToolkit/Windows/Driver/DriverManager.cs)

See [validation results](pawnio-validation.md) for the actual local test scope.

ARM64EC image identification: [Microsoft ARM64EC documentation](https://learn.microsoft.com/en-us/windows/arm/arm64ec#identifying-arm64ec-binaries-and-apps).
