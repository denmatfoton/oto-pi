#pragma once

#include "CommonDefs.h"

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
    TimePoint RunStartedAt() { return m_runStart; }
    void RestoreInitialPosition();

    static int InitializeGpio();

private:
    const int m_pwmPin = -1;
    const int m_drainPin = -1;

    TimePoint m_runStart;
    int m_runDirection = 0;
    int m_directionalTimeAccumulatorMs = 0;
};

class NozzleControl
{
private:
    static constexpr int c_pwmPinNozzle = 18;
    static constexpr int c_drainPinNozzle = 17;
    static constexpr int c_pwmPinValve = 13;
    static constexpr int c_drainPinValve = 6;

    // Custom definitions
    static constexpr int c_waterPressureThreshold = 3'000;
    static constexpr int c_valveOpeningTimeoutMs = 1000;

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

    bool IsWaterPressurePresent();
    std::future<I2cStatus> OpenValve();
    std::future<I2cStatus> CloseValve();

    void TurnValve(MotorDirection direction, std::chrono::milliseconds duration, int dutyPercent);

private:
    void HandleValveState(I2cStatus status, MotorDirection direction);
    bool IsItWaterPressure(int curPressure) {
        return curPressure > m_pressureSensor.GetMinRawPressure() + c_waterPressureThreshold;
    }

    MotorControl m_motorNozzle;
    MotorControl m_motorValve;
    MagnetSensor m_magnetSensor;
    PressureSensor m_pressureSensor;
};
