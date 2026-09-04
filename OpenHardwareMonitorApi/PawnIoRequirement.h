#pragma once

namespace OpenHardwareMonitorApi
{
    // Kept separate from device access so missing/old installations can be tested
    // without changing the machine's registry or installed drivers.
    inline void ValidatePawnIoVersion(bool installed, System::Version^ version)
    {
        if (!installed || version == nullptr || version < gcnew System::Version(2, 0, 0, 0))
        {
            bool chinese = System::Globalization::CultureInfo::CurrentUICulture->TwoLetterISOLanguageName == L"zh";
            throw gcnew System::InvalidOperationException(chinese
                ? L"硬件监控需要 PawnIO 2.0 或更新版本。请从 https://pawnio.eu/ 安装官方驱动后重新启动 TrafficMonitor。"
                : L"Hardware monitoring requires PawnIO 2.0 or later. Install the official driver from https://pawnio.eu/ and restart TrafficMonitor.");
        }
    }
}
