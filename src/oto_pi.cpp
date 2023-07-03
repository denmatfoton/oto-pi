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

    int rawPressure = -1;
    mutex mutex;
    condition_variable cv;
    
    unique_lock lk(mutex);

    pressureSensor.ReadRawPressureAsync([&mutex, &cv, &rawPressure] (int rawValue) {
        {
            unique_lock lk2(mutex);
            rawPressure = rawValue;
        }
        cv.notify_one();
    });

    cv.wait(lk);

    if (rawPressure < 0)
    {
        cerr << "pressureSensor.ReadRawPressureAsync() failed" << endl;
        return -1;
    }

    float pressurePsi = PressureSensor::ConvertToPsi(rawPressure);
    cout << "Pressure: " << pressurePsi << endl;

    return 0;
}
