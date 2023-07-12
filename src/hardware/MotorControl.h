#pragma once

#include "MagnetSensor.h"
#include "PressureSensor.h"

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

class NozzleControl
{
private:
    static constexpr int c_pwmPinNozzle = 18;
    static constexpr int c_drainPinNozzle = 17;
    static constexpr int c_pwmPinValve = 13;
    static constexpr int c_drainPinValve = 6;

public:
    NozzleControl(I2cAccessor& i2cAccessor) :
        m_motorNozzle(c_pwmPinNozzle, c_drainPinNozzle),
        m_motorValve(c_pwmPinValve, c_drainPinValve),
        m_magnetSensor(i2cAccessor),
        m_pressureSensor(i2cAccessor)
    {}

    std::future<I2cStatus> RotateTo(MotorDirection direction, int targetAngle, int dutyPercent);
    std::future<I2cStatus> FetchPosition() { return m_magnetSensor.ReadAngleAsync(); }
    int GetPosition() { return m_magnetSensor.GetLastRawAngle(); }

    std::future<I2cStatus> SetPressure(int targetPressure, int dutyPercent);
    std::future<I2cStatus> FetchPressure() { return m_pressureSensor.ReadPressureAsync(); }
    int GetPressure() { return m_pressureSensor.GetLastRawPressure(); }
    std::future<I2cStatus> CloseValve();

private:
    MotorControl m_motorNozzle;
    MotorControl m_motorValve;
    MagnetSensor m_magnetSensor;
    PressureSensor m_pressureSensor;
};
