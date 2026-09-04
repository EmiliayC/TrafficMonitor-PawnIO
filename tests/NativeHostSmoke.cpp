#include "OpenHardwareMonitor/OpenHardwareMonitorApi.h"
#include <Windows.h>
#include <iostream>

int wmain(int argc, wchar_t* argv[])
{
    const bool expectUnavailable = argc > 1 && std::wstring(argv[1]) == L"--expect-unavailable";
    auto monitor = OpenHardwareMonitorApi::CreateInstance();
    if (!monitor)
    {
        auto error = OpenHardwareMonitorApi::GetErrorMessage();
        std::wcout << error << std::endl;
        return expectUnavailable && !error.empty() ? 0 : 1;
    }
    if (expectUnavailable) return 2;
    monitor->SetCpuEnable(true);
    monitor->SetGpuEnable(true);
    monitor->SetHddEnable(true);
    monitor->SetMainboardEnable(true);
    for (int i = 0; i < 3; ++i)
    {
        monitor->GetHardwareInfo();
        auto error = OpenHardwareMonitorApi::GetErrorMessage();
        if (!error.empty()) { std::wcerr << error << std::endl; return 3; }
        std::wcout << L"CPU=" << monitor->CpuTemperature() << L" GPU=" << monitor->GpuTemperature()
            << L" Disk=" << monitor->HDDTemperature() << L" Board=" << monitor->MainboardTemperature() << std::endl;
        Sleep(1000);
    }
    std::wcout << L"Native host / mixed-mode bridge / hardware sampling passed." << std::endl;
    return 0;
}
