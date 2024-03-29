#include "NozzleControl.h"

#include <iostream>

using namespace std;

int RotateValve(NozzleControl& nozzle, MotorDirection direction, int value, int duty)
{
    chrono::milliseconds duration(value);

    nozzle.SetCloseValveOnExit(false);
    nozzle.TurnValve(direction, duration, duty);

    auto pressureFuture = nozzle.FetchPressure();
    
    cout << "Current raw pressure: ";
    pressureFuture.wait();
    if (pressureFuture.get() != HwResult::Success)
    {
        cerr << "Fetch pressure failed" << endl;
        return -1;
    }

    int pressure = nozzle.GetPressure();
    cout << pressure << endl;
    cout << "Pressure PSI: " << PressureSensor::ConvertToPsi(pressure) << endl;
    cout << "Water pressure present: " << nozzle.IsWaterPressurePresent() << endl;
    cout << "Min pressure: " << nozzle.GetPressureSensor().GetMinPressure() << endl;

    return 0;
}

int RotateNozzle(NozzleControl& nozzle, MotorDirection direction, int value, int duty)
{
    int targetAngle = value;
    if (targetAngle < 0 || targetAngle >= MagnetSensor::c_angleRange)
    {
        cerr << "Incorrect targetAngle argument." << endl;
        return -1;
    }

    auto rotateFuture = nozzle.RotateToDirectionAsync(direction, targetAngle, duty);

    rotateFuture.wait();
    if (rotateFuture.get() != HwResult::Success)
    {
        cerr << "RotateToDirectionAsync failed" << endl;
        return -1;
    }

    cout << "Rotation finished at position: " << nozzle.GetPosition() << endl;

    // Wait for motor to stop completely
    this_thread::sleep_for(1s);

    auto positionFuture = nozzle.FetchPosition();

    cout << "Refresh position: ";
    positionFuture.wait();
    if (positionFuture.get() != HwResult::Success)
    {
        cerr << "Fetch position failed" << endl;
        return -1;
    }

    cout << nozzle.GetPosition() << endl;

    return 0;
}

int main(int argc, char *argv[])
{
    NozzleControl nozzle;
    IfFailRet(nozzle.Init());
    
    cout << "Min pressure: " << nozzle.GetPressureSensor().GetMinPressure() << endl;

    auto positionFuture = nozzle.FetchPosition();
    auto pressureFuture = nozzle.FetchPressure();

    cout << "Current position: ";
    positionFuture.wait();
    if (positionFuture.get() != HwResult::Success)
    {
        cerr << "Fetch position failed" << endl;
        return -1;
    }

    cout << nozzle.GetPosition() << endl;
    
    cout << "Current raw pressure: ";
    pressureFuture.wait();
    if (pressureFuture.get() != HwResult::Success)
    {
        cerr << "Fetch pressure failed" << endl;
        return -1;
    }

    int pressure = nozzle.GetPressure();
    cout << pressure << endl;
    cout << "Pressure PSI: " << PressureSensor::ConvertToPsi(pressure) << endl;
    cout << "Min pressure: " << nozzle.GetPressureSensor().GetMinPressure() << endl;

    if (argc < 4)
    {
        cerr << "Incorrect number of arguments." << endl;
        return -1;
    }

    int value = static_cast<int>(strtol(argv[2], nullptr, 10));
    
    int duty = static_cast<int>(strtol(argv[3], nullptr, 10));
    if (duty < 0 || duty > 100)
    {
        cerr << "Incorrect duty argument." << endl;
        return -1;
    }

    MotorDirection direction = MotorDirection::Left;
    switch (argv[1][0])
    {
        case 'c':
            direction = MotorDirection::Close;
            RotateValve(nozzle, direction, value, duty);
            break;
        case 'o':
            direction = MotorDirection::Open;
            RotateValve(nozzle, direction, value, duty);
            break;
        case 'l':
            direction = MotorDirection::Left;
            RotateNozzle(nozzle, direction, value, duty);
            break;
        case 'r':
            direction = MotorDirection::Right;
            RotateNozzle(nozzle, direction, value, duty);
            break;
        default:
            cerr << "Incorrect direction argument." << endl;
            return -1;
    }

    return 0;
}
