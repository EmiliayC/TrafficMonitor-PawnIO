#include "../OpenHardwareMonitorApi/OpenHardwareMonitorImp.h"
#include "../OpenHardwareMonitorApi/PawnIoRequirement.h"
#include <cmath>
#include <stdexcept>

using namespace OpenHardwareMonitorApi;

static void Require(bool condition, const char* message)
{
    if (!condition) throw std::runtime_error(message);
}

static FakeSensor^ Sensor(String^ name, SensorType type, Nullable<float> value)
{
    auto sensor = gcnew FakeSensor();
    sensor->Name = name;
    sensor->SensorType = type;
    sensor->Value = value;
    return sensor;
}

namespace OpenHardwareMonitorApi
{
    class SensorReaderTests
    {
    public:
        static void Run()
        {
            bool missingRejected = false, outdatedRejected = false;
            try { ValidatePawnIoVersion(false, nullptr); }
            catch (InvalidOperationException^ error) { missingRejected = error->Message->Contains("https://pawnio.eu/"); }
            try { ValidatePawnIoVersion(true, gcnew Version(1, 9, 0, 0)); }
            catch (InvalidOperationException^) { outdatedRejected = true; }
            Require(missingRejected && outdatedRejected, "Missing/outdated PawnIO must be rejected with installation guidance");
            ValidatePawnIoVersion(true, gcnew Version(2, 0, 0, 0));
            ValidatePawnIoVersion(true, gcnew Version(2, 1, 0, 0));
            COpenHardwareMonitor monitor;
            auto cpu = gcnew FakeHardware();
            cpu->HardwareType = HardwareType::Cpu;
            float value = 123;
            Require(!monitor.GetCPUFreq(cpu, value) && value == -1, "Empty CPU clocks must be unavailable");
            auto clock = Sensor("CPU Core #1", SensorType::Clock, 4200.0f);
            auto temperature = Sensor("Core", SensorType::Temperature, 60.0f);
            cpu->Sensors = gcnew array<ISensor^>{clock, temperature};
            Require(monitor.GetCPUFreq(cpu, value) && std::abs(value - 4.2f) < 0.001f, "Clock units");
            Require(monitor.GetCpuTemperature(cpu, value) && value == 60, "Valid temperature");
            clock->Value = Nullable<float>();
            temperature->Value = Nullable<float>();
            Require(!monitor.GetCPUFreq(cpu, value) && value == -1, "Null clock must not reuse old sample");
            Require(!monitor.GetCpuTemperature(cpu, value) && value == -1 && monitor.AllCpuTemperature().empty(), "Null temperature must not become zero or remain cached");
            clock->Value = Single::NaN;
            temperature->Value = Single::PositiveInfinity;
            Require(!monitor.GetCPUFreq(cpu, value) && !monitor.GetCpuTemperature(cpu, value), "Non-finite readings must be skipped");
            temperature->Value = 0.0f;
            Require(monitor.GetCpuTemperature(cpu, value) && value == 0, "Zero is a valid reading");
            cpu->Sensors = gcnew array<ISensor^>{Sensor("CPU Core #1", SensorType::Load, 90.0f), Sensor("CPU Total", SensorType::Load, 25.0f)};
            Require(monitor.GetCpuUsage(cpu, value) && value == 25, "Use total CPU load instead of first core");
            auto gpu = gcnew FakeHardware();
            gpu->HardwareType = HardwareType::GpuNvidia;
            Require(!monitor.GetGpuUsage(gpu, value) && value == -1, "Missing GPU must not report idle");
            gpu->Sensors = gcnew array<ISensor^>{Sensor("GPU Core", SensorType::Load, Nullable<float>()), Sensor("Other", SensorType::Load, 0.0f)};
            Require(monitor.GetGpuUsage(gpu, value) && value == 0, "Valid idle GPU fallback");
            auto disk = gcnew FakeHardware();
            disk->Sensors = gcnew array<ISensor^>{Sensor("Total Activity", SensorType::Load, Nullable<float>())};
            Require(!monitor.GetHddUsage(disk, value) && value == -1, "Null disk activity");
            auto board = gcnew FakeHardware();
            board->HardwareType = HardwareType::Motherboard;
            board->SubHardware = gcnew array<IHardware^>{cpu};
            cpu->Sensors = gcnew array<ISensor^>{temperature};
            Require(monitor.GetHardwareTemperature(board, value) && value == 0, "Subhardware temperature traversal");
            monitor.ResetAllValues();
            Require(monitor.AllCpuTemperature().empty() && monitor.m_all_cpu_clock.empty() && monitor.CpuTemperature() == -1, "Disabled hardware must not retain prior readings");
        }
    };
}

int main(array<String^>^ args)
{
    try
    {
        if (args->Length == 0)
        {
            SensorReaderTests::Run();
            Console::WriteLine("Sensor regression tests passed.");
            return 0;
        }
        // This exercises the real native ABI, CLR binding and driver diagnostics.
        auto monitor = CreateInstance();
        if (args[0] == "--expect-unavailable")
        {
            Require(!monitor && !GetErrorMessage().empty(), "Expected a useful initialization error without an accessible driver");
            Console::WriteLine(gcnew String(GetErrorMessage().c_str()));
            return 0;
        }
        Require(monitor != nullptr, "Hardware initialization failed");
        monitor->SetCpuEnable(true);
        monitor->SetGpuEnable(true);
        monitor->SetHddEnable(true);
        monitor->SetMainboardEnable(true);
        for (int i = 0; i < 3; ++i)
        {
            monitor->GetHardwareInfo();
            Require(GetErrorMessage().empty(), "Hardware update failed");
            Console::WriteLine("CPU={0} GPU={1} Disk={2} Board={3}", monitor->CpuTemperature(), monitor->GpuTemperature(), monitor->HDDTemperature(), monitor->MainboardTemperature());
            System::Threading::Thread::Sleep(1000);
        }
        return 0;
    }
    catch (System::Exception^ error) { Console::Error->WriteLine(error); }
    catch (const std::exception& error) { Console::Error->WriteLine(gcnew String(error.what())); }
    return 1;
}
