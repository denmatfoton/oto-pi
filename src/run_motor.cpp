#include "MotorControl.h"

#include <iostream>

using namespace std;

int main(int argc, char *argv[])
{
    if (argc < 5)
    {
        cerr << "Incorrect number of arguments." << endl;
        return -1;
    }

    MotorUnit motorUnit = argv[1][0] == 'n' ? MotorUnit::Nozzle : MotorUnit::Valve;

    MotorDirection direction = MotorDirection::Close;
    switch (argv[2][0])
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
    
    int duty = strtol(argv[3], nullptr, 10);
    if (duty < 0 || duty > 100)
    {
        cerr << "Incorrect duty argument." << endl;
        return -1;
    }

    int durationMs = strtol(argv[4], nullptr, 10);
    if (durationMs < 0)
    {
        cerr << "Incorrect durationMs argument." << endl;
        return -1;
    }

    int status = MotorControl::InitializeGpio();
    if (status != 0)
    {
        cerr << "InitializeGpio failed with: " << status << endl;
        return -1;
    }

    MotorControl motor(motorUnit);
    motor.RunMotor(direction, duty, durationMs);

    return 0;
}
