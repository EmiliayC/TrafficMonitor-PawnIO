# PawnIO migration validation — 2026-09-04

## Completed

- Repository's vendored library identified as LibreHardwareMonitor 0.9.4.0.
- Locked NuGet restore and .NET Framework 4.7.2 runtime build succeeded for win-x64
  and win-x86. Exact package/transitive versions and hashes are in packages.lock.json.
- Standard TrafficMonitor.sln Release builds succeeded for x64 and x86. The existing
  application emits unrelated conversion/deprecated-API/linker warnings; no build
  errors remain for these two platforms.
- SensorReaderTests passed as x64 and Win32: absent/outdated PawnIO version inputs,
  valid minimum/newer version inputs, null/NaN/infinite sensor values, empty clocks,
  removed readings, valid zeros, total CPU selection and subhardware traversal.
- Both architectures reject initialization under a non-elevated process on this
  machine and return an actionable PawnIO access diagnostic.
- Both native host executables loaded the **actual built OpenHardwareMonitorApi.dll**
  and copied runtime/config, sampled enabled CPU/GPU/storage/motherboard groups three
  times as administrator, and exited successfully. No test-only assembly resolver
  or alternate bridge implementation is used by NativeHostSmoke.
- Publication validation succeeded for x64 and x86: PE architecture, LHM 0.9.6.0,
  all 13 copied runtime DLL hashes, CLR config, duplicate-library checks, no legacy
  WinRing0 implementation/payloads, and no bundled kernel drivers. Framework-provided
  assemblies are resolved by .NET Framework rather than shipped unnecessarily.
- Negative checks correctly rejected a missing System.Memory.dll, an x86 LHM in an
  x64 package, a WinRing0x64.sys fixture and a plain x64 EXE presented as ARM64EC.
- Audit of exact RAMSPDToolkit-NDD 1.4.2 source and binary confirmed that its retained
  WinRing0 interface/enum names have no corresponding OLS implementation or embedded
  driver resources. This is explicitly checked, not treated as blanket trust of DLLs.

## Hardware observations

Local system: Windows 11 x64; installed signed PawnIO **2.0.1.0** was reused and
was not changed by this work. Sample temperatures (Celsius):

| Native host | Sample | CPU | GPU | Disk | Motherboard |
| --- | --- | --- | --- | --- | --- |
| x64 | 1 | 58.0000 | 54 | 55.94 | 52.3333 |
| x64 | 2 | 53.5625 | 55 | 55.74 | 52.3333 |
| x64 | 3 | 52.8750 | 55 | 55.74 | 52.3333 |
| x86 | 1 | 57.4375 | 55 | 55.94 | 52.3333 |
| x86 | 2 | 49.8750 | 54 | 55.74 | 52.3333 |
| x86 | 3 | 48.4375 | 54 | 55.74 | 52.3333 |

These demonstrate successful real readings on this machine; they do not establish
sensor accuracy or compatibility with all hardware. No driver was stopped,
uninstalled or downgraded to create missing/old-driver tests. Those states were
covered through version-validation inputs; the actual permission-denied path was
also exercised. A fresh driver-free machine check is configured in CI but was not
run locally.

## Release scope: x64 and x86

Release acceptance covers x64 and x86. Both packages are build- and hardware-tested.
ARM64/ARM64EC compilation and runtime validation are outside this release's scope.
The retained ARM64EC source configuration and runtime-copy target are unverified;
no ARM64EC archive is shipped and the migration CI only builds x64 and x86.

## Reproducibility and environment

Validation used VS 2022 v143 (MSVC 14.44.35207), .NET SDK 9.0.300 and the .NET
Framework 4.7.2 targeting pack. Missing Windows SDK 10.0.26100 and x64/x86 MFC
components were installed to enable the local builds. No reboot was performed.
The results above are local validation results. GitHub Actions results are reported separately in the repository Actions tab.

See [migration/build instructions](pawnio-migration.md) and
[dependency notices](hardware-runtime-notices.md).
