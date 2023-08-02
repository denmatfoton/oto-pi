#pragma once

#include "MagnetSensor.h"
#include "MotorControl.h"
#include "PressureSensor.h"

#include <memory>

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
    NozzleControl();
    ~NozzleControl();

    int Init();

    std::future<I2cStatus> RotateTo(MotorDirection direction, int targetAngle, int dutyPercent);
    std::future<I2cStatus> RotateDiff(int diffAngle, int dutyPercent);
    std::future<I2cStatus> FetchPosition() { return m_magnetSensor.ReadAngleAsync(); }
    int GetPosition() { return m_magnetSensor.GetLastRawAngle(); }
    int GetPositionFetchIfStale() { return m_magnetSensor.GetRawAngleFetchIfStale(); }

    std::future<I2cStatus> SetPressure(int targetPressure, int dutyPercent);
    std::future<I2cStatus> SetPressureDiff(int diffPressure, int dutyPercent);
    std::future<I2cStatus> FetchPressure() { return m_pressureSensor.ReadPressureAsync(); }
    int GetPressure() { return m_pressureSensor.GetLastRawPressure(); }
    int GetPressureFetchIfStale() { return m_pressureSensor.GetRawPressureFetchIfStale(); }

    bool IsWaterPressurePresent();
    std::future<I2cStatus> OpenValve();
    std::future<I2cStatus> CloseValve();

    void TurnValve(MotorDirection direction, std::chrono::milliseconds duration, int dutyPercent);

private:
    void HandleValveState(I2cStatus status, MotorDirection direction);
    bool IsItWaterPressure(int curPressure) {
        return curPressure > m_pressureSensor.GetMinRawPressure() + c_waterPressureThreshold;
    }

    int m_lastTargetAngle = -1;
    int m_lastTargetPressure = -1;
    MotorControl m_motorNozzle;
    MotorControl m_motorValve;
    std::unique_ptr<I2cAccessor> m_spI2cAccessor;
    MagnetSensor m_magnetSensor;
    PressureSensor m_pressureSensor;
};
