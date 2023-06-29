#include "MotorControl.h"

#include <pigpio.h>

#include <chrono>
#include <iostream>
#include <thread>

using namespace std;

MotorControl::MotorControl(MotorUnit motorUnit)
{
    switch (motorUnit)
    {
    case MotorUnit::Nozzle:
        m_pwmPin = c_pwmPinNozzle;
        m_drainPin = c_drainPinNozzle;
        break;
    
    case MotorUnit::Valve:
        m_pwmPin = c_pwmPinValve;
        m_drainPin = c_drainPinValve;
        break;
    }

    gpioSetMode(m_pwmPin, PI_OUTPUT);
    gpioWrite(m_pwmPin, 0);
    gpioSetMode(m_drainPin, PI_OUTPUT);
    gpioWrite(m_drainPin, 0);
}

int MotorControl::InitializeGpio()
{
    if (gpioCfgClock(5, 1, 0)) return -1;
    if (gpioInitialise() < 0) return -2;

    return 0;
}

void MotorControl::RunMotor(MotorDirection direction, int duty, int durationMs)
{
    int drainValue = static_cast<int>(direction);
    duty += (100 - duty * 2) * drainValue; // Invert duty in case drainValue == 1.
    duty *= 10000;

    int status = gpioHardwarePWM(m_pwmPin, c_pwmFreq, duty);
    if (status != 0)
    {
        cerr << "gpioHardwarePWM failed with: " << status << endl;
        return;
    }

    gpioWrite(m_drainPin, drainValue);

    this_thread::sleep_for(std::chrono::milliseconds(durationMs));

    StopMotor();
}

void MotorControl::StopMotor()
{
    int status = gpioHardwarePWM(m_pwmPin, 0, 0);
    if (status != 0)
    {
        cerr << "gpioHardwarePWM failed with: " << status << endl;
        return;
    }

    gpioWrite(m_pwmPin, 0);
    gpioWrite(m_drainPin, 0);
}