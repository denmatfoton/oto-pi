#include "I2cAccessor.h"
#include "MotorControl.h"

#include <iostream>

using namespace std;

int main(int argc, char *argv[])
{
    static constexpr const char i2cFileName[] = "/dev/i2c-1";

    I2cAccessor i2cAccessor;
    if (i2cAccessor.Init(i2cFileName) != 0)
    {
        cerr << "i2cAccessor.Init() failed" << endl;
        return -1;
    }

    NozzleControl nozzle(i2cAccessor);
    auto positionFuture = nozzle.FetchPosition();
    auto pressureFuture = nozzle.FetchPressure();

    cout << "Current position: ";
    positionFuture.wait();
    if (positionFuture.get() != I2cStatus::Success)
    {
        cerr << "Fetch position failed" << endl;
        return -1;
    }

    cout << nozzle.GetPosition() << endl;
    
    cout << "Current raw pressure: ";
    pressureFuture.wait();
    if (pressureFuture.get() != I2cStatus::Success)
    {
        cerr << "Fetch pressure failed" << endl;
        return -1;
    }

    int rawPressure = nozzle.GetPressure();
    cout << rawPressure << endl;
    cout << "Pressure PSI: " << PressureSensor::ConvertToPsi(rawPressure) << endl;

    if (argc < 4)
    {
        cerr << "Incorrect number of arguments." << endl;
        return -1;
    }

    if (MotorControl::InitializeGpio() < 0)
    {
        cerr << "InitializeGpio failed" << endl;
        return -1;
    }

    MotorDirection direction = MotorDirection::Left;
    switch (argv[1][0])
    {
        case 'c':
            direction = MotorDirection::Close;
            break;
        case 'o':
            direction = MotorDirection::Open;
            break;
        case 'l':
            direction = MotorDirection::Left;
            break;
        case 'r':
            direction = MotorDirection::Right;
            break;
        default:
            cerr << "Incorrect direction argument." << endl;
            return -1;
    }

    int targetAngle = strtol(argv[2], nullptr, 10);
    if (targetAngle < 0 || targetAngle >= MagnetSensor::c_angleRange)
    {
        cerr << "Incorrect targetAngle argument." << endl;
        return -1;
    }
    
    int duty = strtol(argv[3], nullptr, 10);
    if (duty < 0 || duty > 100)
    {
        cerr << "Incorrect duty argument." << endl;
        return -1;
    }

    auto rotateFuture = nozzle.RotateTo(direction, targetAngle, duty);

    rotateFuture.wait();
    if (rotateFuture.get() != I2cStatus::Success)
    {
        cerr << "RotateTo failed" << endl;
        return -1;
    }

    cout << "Rotation finished at position: " << nozzle.GetPosition() << endl;

    // Wait for motor to stop completely
    this_thread::sleep_for(1s);

    positionFuture = nozzle.FetchPosition();

    cout << "Refresh position: ";
    positionFuture.wait();
    if (positionFuture.get() != I2cStatus::Success)
    {
        cerr << "Fetch position failed" << endl;
        return -1;
    }

    cout << nozzle.GetPosition() << endl;

    return 0;
}
