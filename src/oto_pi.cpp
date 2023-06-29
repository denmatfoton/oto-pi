#include <fcntl.h>
#include <unistd.h>

#include <chrono>
#include <iostream>

#include "MagnetSensor.h"
#include "PressureSensor.h"

using namespace std;

int main()
{
    static constexpr const char i2cFileName[] = "/dev/i2c-1";
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

    return 0;
}
