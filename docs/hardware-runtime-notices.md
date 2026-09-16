# Hardware runtime dependency notices

The standard edition uses the following locked NuGet packages. Source and license links are recorded from the restored package metadata. Framework-provided assemblies may not be copied into the application package.

| Package | Version | License | Source |
| --- | --- | --- | --- |
| LibreHardwareMonitorLib | 0.9.6 | MPL-2.0 | https://github.com/LibreHardwareMonitor/LibreHardwareMonitor |
| BlackSharp.Core | 1.0.7 | MPL-2.0 | https://github.com/Blacktempel/BlackSharp |
| DiskInfoToolkit | 1.1.2 | MPL-2.0 | https://github.com/Blacktempel/DiskInfoToolkit |
| HidSharp | 2.6.4 | [license](licenses/HidSharp-2.6.4.txt) | https://software.seekye.com/hidsharp |
| RAMSPDToolkit-NDD | 1.4.2 (restore/build only; not distributed) | MPL-2.0 | https://github.com/Blacktempel/RAMSPDToolkit |
| System.Buffers | 4.6.1 | MIT | https://github.com/dotnet/maintenance-packages |
| System.CodeDom | 10.0.2 | MIT | https://dot.net/ |
| System.Management | 10.0.2 | MIT | https://dot.net/ |
| System.Memory | 4.6.3 | MIT | https://github.com/dotnet/maintenance-packages |
| System.Numerics.Vectors | 4.6.1 | MIT | https://github.com/dotnet/maintenance-packages |
| System.Runtime.CompilerServices.Unsafe | 6.1.2 | MIT | https://github.com/dotnet/maintenance-packages |
| System.Security.AccessControl | 6.0.0 | MIT | https://dot.net/ |
| System.Security.Principal.Windows | 5.0.0 | MIT | https://github.com/dotnet/runtime |
| System.Threading.AccessControl | 10.0.3 | MIT | https://dot.net/ |

LibreHardwareMonitor is distributed under MPL-2.0. RAMSPDToolkit-NDD is restored from the official NuGet feed only as a transitive build input and is excluded from release packages because TrafficMonitor does not enable memory/SPD monitoring. PawnIO is installed separately from https://pawnio.eu/ and is not bundled.

Package pages and license files are available at https://www.nuget.org/packages/ using the package IDs and versions above.

License texts: [MPL-2.0](licenses/MPL-2.0.txt), [Microsoft .NET MIT](licenses/dotnet-MIT.txt), [HidSharp](licenses/HidSharp-2.6.4.txt).

## Source availability for MPL-2.0 components

The following unmodified libraries are used by the locked build; RAMSPDToolkit-NDD is not included in the published package. Their corresponding
source code is available at the exact upstream revisions below under MPL-2.0.
The Anti-996 terms applying to TrafficMonitor do not replace or restrict recipients'
rights to these MPL-covered source files.

- LibreHardwareMonitorLib 0.9.6: [source](https://github.com/LibreHardwareMonitor/LibreHardwareMonitor/tree/3d331e3370efb858411f19511373eff65a218701), [source archive](https://github.com/LibreHardwareMonitor/LibreHardwareMonitor/archive/3d331e3370efb858411f19511373eff65a218701.zip).
- BlackSharp.Core 1.0.7: [source](https://github.com/Blacktempel/BlackSharp/tree/c70b735c6cec123ee8a046ac4a0bc6c606f52cf0), [source archive](https://github.com/Blacktempel/BlackSharp/archive/c70b735c6cec123ee8a046ac4a0bc6c606f52cf0.zip).
- DiskInfoToolkit 1.1.2: [source](https://github.com/Blacktempel/DiskInfoToolkit/tree/25319eae5781e75bcf141e844ceab2afe94d40ea), [source archive](https://github.com/Blacktempel/DiskInfoToolkit/archive/25319eae5781e75bcf141e844ceab2afe94d40ea.zip).
- RAMSPDToolkit-NDD 1.4.2: [source](https://github.com/Blacktempel/RAMSPDToolkit/tree/3b47b960e0830fef344624ad5e389675d5f0a1ce), [source archive](https://github.com/Blacktempel/RAMSPDToolkit/archive/3b47b960e0830fef344624ad5e389675d5f0a1ce.zip).

TrafficMonitor also includes TinyXML-2 by Lee Thomason under its zlib-style license. The original notice is preserved in source and supplied in [TinyXML-2 notice](licenses/TinyXML-2.txt).

