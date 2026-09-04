using System;
using System.Collections.Generic;
using LibreHardwareMonitor.Hardware;

public sealed class FakeSensor : ISensor
{
    public IControl Control { get { return null; } }
    public IHardware Hardware { get { return null; } }
    public Identifier Identifier { get { return new Identifier("test", "sensor"); } }
    public int Index { get { return 0; } }
    public bool IsDefaultHidden { get { return false; } }
    public float? Max { get { return Value; } }
    public float? Min { get { return Value; } }
    public string Name { get; set; }
    public IReadOnlyList<IParameter> Parameters { get { return new IParameter[0]; } }
    public SensorType SensorType { get; set; }
    public float? Value { get; set; }
    public IEnumerable<SensorValue> Values { get { return new SensorValue[0]; } }
    public TimeSpan ValuesTimeWindow { get; set; }
    public void ResetMin() { }
    public void ResetMax() { }
    public void ClearValues() { }
    public void Accept(IVisitor visitor) { visitor.VisitSensor(this); }
    public void Traverse(IVisitor visitor) { }
}

public sealed class FakeHardware : IHardware
{
    public FakeHardware() { Sensors = new ISensor[0]; SubHardware = new IHardware[0]; }
    public HardwareType HardwareType { get; set; }
    public Identifier Identifier { get { return new Identifier("test"); } }
    public string Name { get; set; }
    public IHardware Parent { get { return null; } }
    public ISensor[] Sensors { get; set; }
    public IHardware[] SubHardware { get; set; }
    public IDictionary<string, string> Properties { get { return new Dictionary<string, string>(); } }
    public string GetReport() { return "test hardware"; }
    public void Update() { }
    public void Accept(IVisitor visitor) { visitor.VisitHardware(this); }
    public void Traverse(IVisitor visitor) { foreach (var sensor in Sensors) sensor.Accept(visitor); }
    public event SensorEventHandler SensorAdded { add { } remove { } }
    public event SensorEventHandler SensorRemoved { add { } remove { } }
}
