#include <iostream>

#include "I2cAccessor.h"
#include "MagnetSensor.h"
#include "PressureSensor.h"

using namespace std;

int main()
{
    static constexpr const char i2cFileName[] = "/dev/i2c-1";

#if 0
    int i2cHandle = open(i2cFileName, O_RDWR);

    PressureSensor pressureSensor(i2cHandle);

    auto start = chrono::steady_clock::now();
    float pressurePsi = pressureSensor.ReadPressurePsi();
    auto end = chrono::steady_clock::now();
    cout << "Pressure: " << pressurePsi << endl;
    auto durationMs = chrono::duration_cast<chrono::milliseconds>(end - start).count();
    cout << "Duration: " << durationMs << " ms" << endl;

    MagnetSensor magnetSensor(i2cHandle);

    magnetSensor.ReadConfig();
    magnetSensor.ReadStatus();
    magnetSensor.ReadAngle();
    
    close(i2cHandle);
#endif

    I2cAccessor i2cAccessor;
    if (i2cAccessor.Init(i2cFileName) != 0)
    {
        cerr << "i2cAccessor.Init() failed" << endl;
        return -1;
    }

    PressureSensor pressureSensor(i2cAccessor);
    MagnetSensor magnetSensor(i2cAccessor);

    auto pressureFuture = pressureSensor.ReadPressureAsync();
    auto angleFuture = magnetSensor.ReadAngleAsync();

    angleFuture.wait();
    if (pressureFuture.get() != I2cStatus::Success)
    {
        cerr << "pressureSensor.ReadPressureAsync() failed" << endl;
    }
    else
    {
        int angle = magnetSensor.GetLastRawAngle();
        cout << "Angle: " << angle << endl;
    }

    pressureFuture.wait();

    if (pressureFuture.get() != I2cStatus::Success)
    {
        cerr << "pressureSensor.ReadPressureAsync() failed" << endl;
    }
    else
    {
        int rawPressure = pressureSensor.GetLastRawPressure();
        cout << "Raw pressure: " << rawPressure << endl;
        float pressurePsi = PressureSensor::ConvertToPsi(rawPressure);
        cout << "Pressure: " << pressurePsi << " Psi" << endl;
    }

    return 0;
}
