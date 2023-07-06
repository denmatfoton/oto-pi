#pragma once

#include "MagnetSensor.h"

enum class MotorDirection : int
{
    Close = 0,
    Open = 1,
    Left = 1,
    Right = 0,
};

class MotorControl
{
private:
    static constexpr int c_pwmPinValve = 13;
    static constexpr int c_drainPinValve = 6;

    static constexpr int c_pwmFreq = 1000;

public:
    MotorControl(int pwmPin, int drainPin);
    ~MotorControl() { Stop(); }

    void Run(MotorDirection direction, int dutyPercent);
    void Stop();

    static int InitializeGpio();

private:
    int m_pwmPin = -1;
    int m_drainPin = -1;
};

class Nozzle
{
private:
    static constexpr int c_pwmPinNozzle = 18;
    static constexpr int c_drainPinNozzle = 17;

public:
    Nozzle(I2cAccessor& i2cAccessor) :
        m_motor(c_pwmPinNozzle, c_drainPinNozzle),
        m_magnetSensor(i2cAccessor)
    {}

    std::future<I2cStatus> RotateTo(MotorDirection direction, int targetAngle, int dutyPercent);
    std::future<I2cStatus> FetchPosition() { return m_magnetSensor.ReadAngleAsync(); }
    int GetPosition() { return m_magnetSensor.GetLastRawAngle(); }

private:
    MotorControl m_motor;
    MagnetSensor m_magnetSensor;
};
