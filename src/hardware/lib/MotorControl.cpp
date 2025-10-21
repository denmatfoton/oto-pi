#include "MotorControl.h"

#include <pigpio.h>

#include <iostream>
#include <thread>

using namespace std;

int MotorControl::Init()
{
    int status = InitializeGpio();
    if (status < 0)
    {
        cerr << "InitializeGpio failed" << endl;
        return status;
    }

    gpioSetMode(m_pwmPin, PI_OUTPUT);
    gpioWrite(m_pwmPin, 0);
    gpioSetMode(m_drainPin, PI_OUTPUT);
    gpioWrite(m_drainPin, 0);

    return 0;
}

int MotorControl::InitializeGpio()
{
    static bool gpioInitialized = false;
    if (!gpioInitialized)
    {
        if (gpioCfgClock(5, 1, 0)) return -1;
        if (gpioInitialise() < 0) return -2;
        gpioInitialized = true;
    }

    return 0;
}

void MotorControl::Run(MotorDirection direction, int dutyPercent)
{
    if (IsRunning())
    {
        if (m_lastDirection == direction)
        {
            return;
        }
        Stop();
    }

    int drainValue = static_cast<int>(direction);
    dutyPercent += (100 - dutyPercent * 2) * drainValue; // Invert duty in case drainValue == 1.
    int duty = dutyPercent * 10000;

    int status = gpioHardwarePWM(m_pwmPin, c_pwmFreq, duty);
    if (status != 0)
    {
        cerr << __func__ << ", gpioHardwarePWM failed with: " << status << endl;
        return;
    }

    gpioWrite(m_drainPin, drainValue);

    m_runStartTime = chrono::steady_clock::now();
    m_runDirection = drainValue * 2 - 1;
    m_lastDirection = direction;
}

void MotorControl::Stop()
{
    int status = gpioHardwarePWM(m_pwmPin, 0, 0);
    if (status != 0)
    {
        cerr << __func__ << ", gpioHardwarePWM failed with: " << status << endl;
        return;
    }

    gpioWrite(m_pwmPin, 0);
    gpioWrite(m_drainPin, 0);

    if (m_runDirection != 0)
    {
        auto end = chrono::steady_clock::now();
        int duration = static_cast<int>(chrono::duration_cast<chrono::milliseconds>(end - m_runStartTime).count());
        m_directionalTimeAccumulatorMs += duration * m_runDirection;
        m_runDirection = 0;
    }
}

void MotorControl::RestoreInitialPosition()
{
    cout << __func__ << ", m_directionalTimeAccumulatorMs: " << m_directionalTimeAccumulatorMs << endl;
    MotorDirection direction = m_directionalTimeAccumulatorMs > 0 ? MotorDirection::Close : MotorDirection::Open;
    Run(direction, 100);
    this_thread::sleep_for(chrono::milliseconds(abs(m_directionalTimeAccumulatorMs)));
    Stop();
}
