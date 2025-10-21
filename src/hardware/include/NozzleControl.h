#pragma once

#include "MagnetSensor.h"
#include "MotorControl.h"
#include "PressureSensor.h"

#include <atomic>
#include <memory>

class Logger;
enum LogLevel : int;

class NozzleControl
{
private:
    static constexpr int c_pwmPinNozzle = 18;
    static constexpr int c_drainPinNozzle = 17;
    static constexpr int c_pwmPinValve = 13;
    static constexpr int c_drainPinValve = 6;

    // Custom definitions
    static constexpr int c_waterPressureThreshold = 10;
    static constexpr int c_valveOpeningTimeoutMs = 3'000;

public:
    NozzleControl();
    ~NozzleControl();

    int Init();

    std::future<HwResult> RotateToAsync(int targetAngle, int dutyPercent);
    std::future<HwResult> RotateToDirectionAsync(MotorDirection direction, int targetAngle, int dutyPercent);
    std::future<HwResult> RotateDiffAsync(int diffAngle, int dutyPercent);
    std::future<HwResult> FetchPosition() { return m_magnetSensor.ReadAngleAsync(); }
    int GetPosition() { return m_magnetSensor.GetLastRawAngle(); }
    int GetPositionFetchIfStale() { return m_magnetSensor.GetRawAngleFetchIfStale(); }
    int GetPositionFetch();

    std::future<HwResult> SetPressureAsync(int targetPressure, int dutyPercent);
    std::future<HwResult> SetPressureDiffAsync(int diffPressure, int dutyPercent);
    std::future<HwResult> FetchPressure() { return m_pressureSensor.ReadPressureAsync(); }
    int GetPressure() { return m_pressureSensor.GetLastPressure(); }
    int GetPressureFetchIfStale() { return m_pressureSensor.GetPressureFetchIfStale(); }
    int GetPressureFetch();

    void SetCloseValveOnExit(bool value) { m_closeValveOnExit = value; }
    bool IsWaterPressurePresent();
    HwResult OpenValve();
    std::future<HwResult> OpenValveAsync();
    HwResult CloseValve(bool fCloseTight);
    std::future<HwResult> CloseValveAsync();

    template <class _Rep, class _Period>
    void TurnValve(MotorDirection direction, const std::chrono::duration<_Rep, _Period>& duration, int dutyPercent)
    {
        m_motorValve.RunDuration(direction, duration, dutyPercent);
    }
    const PressureSensor& GetPressureSensor() const { return m_pressureSensor; }
    const MagnetSensor& GetMagnetSensor() const { return m_magnetSensor; }

    void SetLogger(Logger* pLogger)
    {
        m_pLogger = pLogger;
    }

    void SetTargetPressure(int targetPressure)
    {
        m_targetPressure = targetPressure;
    }

    std::future<HwResult> StartContinuousPressureMeasurement()
    {
        return m_pressureSensor.StartContinuousMeasurement(
            [this] (int curPressure) {
                return ProcessPressureMeasurement(curPressure);
            });
    }
protected:
    HwResult ProcessPressureMeasurement(int curPressure);
    void StopMotorValveIfRunning(int curPressure, int changeRate);

    bool IsItWaterPressure(int curPressure) {
        return curPressure > m_pressureSensor.GetMinPressure() + c_waterPressureThreshold;
    }
    std::future<HwResult> OpenValveInternalAsync(MotorDirection direction);
    std::future<HwResult> CloseValveInternalAsync(MotorDirection direction);

    int m_closedValveDurationMs = 200;  // Duration between valve closure and valve opening when motor keeps running
    bool m_closeValveOnExit = true;

    
    MotorControl m_motorNozzle;
    MotorControl m_motorValve;
    
    std::unique_ptr<I2cAccessor> m_spI2cAccessor;
    MagnetSensor m_magnetSensor;
    PressureSensor m_pressureSensor;
    
    std::atomic<int> m_targetPressure = 0;
    OvershootInterpolator<20, 1000, int> m_overshootInterpolator;
    int m_iPressureMeasurementAfterMotorStart = 0;
    int m_pressureAtMotorStop = -1;
    int m_changeRateAtMotorStop = 0;    Logger* m_pLogger = nullptr;
};

class Interpolator;

class NozzleControlCalibrated : public NozzleControl
{
public:
    NozzleControlCalibrated();
    ~NozzleControlCalibrated();

    HwResult CalibrateNozzle(int minDurationUs, int maxDurationUs, double multiplier);
    HwResult CalibrateValve(int stepDurationMs);

    void RotateDiffDurationBased(int diffAngle);
    std::future<HwResult> RotateDiffDurationBasedAsync(int diffAngle);

    void SetPressureDurationBased(int targetPressure);

private:
    HwResult FindCloseTightPosition();

    std::vector<std::unique_ptr<Interpolator>> m_spRotationInterpolator; // Calculates duration to run the nozzle motor to rotate specific angle distance
    std::unique_ptr<Interpolator> m_spPressureInterpolator;
};
