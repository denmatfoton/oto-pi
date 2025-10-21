#include "NozzleControl.h"

#include "I2cAccessor.h"
#include "Utils.h"

#include <Logger.h>
#include <MathUtils.h>

#include <iostream>
#include <string>

using namespace std;

static constexpr int c_defaultDutyPercent = 100;
static constexpr int c_preciseDutyPercent = 20;
static constexpr int c_pressureTolerance = 20;
static constexpr int c_minPressureMeasurementAfterMotorStart = 3;

template<typename T>
static std::future<T> GetCompletedFuture(T value)
{
    return std::async(std::launch::deferred, [value = std::move(value)]() {
        return value;
    });
}

NozzleControl::NozzleControl() :
    m_motorNozzle(c_pwmPinNozzle, c_drainPinNozzle),
    m_motorValve(c_pwmPinValve, c_drainPinValve),
    m_spI2cAccessor(new I2cAccessor()),
    m_magnetSensor(*m_spI2cAccessor),
    m_pressureSensor(*m_spI2cAccessor)
{}

NozzleControl::~NozzleControl()
{
    if (m_closeValveOnExit)
    {
        CloseValve(true /*fCloseTight*/);
    }
}

int NozzleControl::Init()
{
    IfFailRet(m_spI2cAccessor->Init(c_i2cFileName));
    IfFailRet(m_motorNozzle.Init());
    IfFailRet(m_motorValve.Init());
    cout << "NozzleControl::Init succeeded" << endl;
    return 0;
}

int NozzleControl::GetPositionFetch()
{
    auto measurementFuture = m_magnetSensor.ReadAngleAsync();
    measurementFuture.wait();
    return GetPosition();
}

int NozzleControl::GetPressureFetch()
{
    auto measurementFuture = m_pressureSensor.ReadPressureAsync();
    measurementFuture.wait();
    return GetPressure();
}

std::future<HwResult> NozzleControl::RotateToDirectionAsync(MotorDirection direction, int targetAngle, int dutyPercent)
{
    int startAngle = m_magnetSensor.GetRawAngleFetchIfStale();

    int distance = 0;

    // Angle increases when direction == MotorDirection::Right
    // and decreases when direction == MotorDirection::Left
    if (direction == MotorDirection::Right)
    {
        distance = targetAngle - startAngle;
    }
    else
    {
        distance = startAngle - targetAngle;
    }

    // If passing through 0 point, distance will be negative
    if (distance < 0)
    {
        distance += MagnetSensor::c_angleRange;
    }

    static constexpr int epsilon = 3;
    static constexpr int inertialConst = 30;

    if (distance <= epsilon)
    {
        return GetCompletedFuture(HwResult::Success);
    }

    m_motorNozzle.Run(direction, dutyPercent);

    int inertialOffset = inertialConst * dutyPercent / 100;
    inertialOffset = max(min(inertialOffset, distance / 2), epsilon);

    int maxAngle = targetAngle + epsilon;
    int minAngle = targetAngle - epsilon;

    if (direction == MotorDirection::Right)
    {
        minAngle = targetAngle - inertialOffset;
    }
    else
    {
        maxAngle = targetAngle + inertialOffset;
    }

    minAngle = (minAngle + MagnetSensor::c_angleRange) % MagnetSensor::c_angleRange;
    maxAngle %= MagnetSensor::c_angleRange;

    std::function<HwResult(int)> isExpectedValue;

    cout << "minAngle: " << minAngle << ", maxAngle: " << maxAngle << endl;
    
    if (maxAngle > minAngle)
    {
        isExpectedValue = [maxAngle, minAngle] (int curAngle) {
            return curAngle < maxAngle && curAngle > minAngle ? HwResult::Success : HwResult::Repeat;
        };
    }
    else {
        isExpectedValue = [maxAngle, minAngle] (int curAngle) {
            return curAngle < maxAngle || curAngle > minAngle ? HwResult::Success : HwResult::Repeat;
        };
    };

    return m_magnetSensor.NotifyWhenAngle(move(isExpectedValue),
        [this] (HwResult) { m_motorNozzle.Stop(); });
}

std::future<HwResult> NozzleControl::RotateDiffAsync(int diffAngle, int dutyPercent)
{
    int curPosition = GetPositionFetchIfStale();

    MotorDirection direction = diffAngle > 0 ? MotorDirection::Right : MotorDirection::Left;
    int targetAngle = (curPosition + diffAngle + MagnetSensor::c_angleRange) % MagnetSensor::c_angleRange;

    return RotateToDirectionAsync(direction, targetAngle, dutyPercent);
}

std::future<HwResult> NozzleControl::RotateToAsync(int targetAngle, int dutyPercent)
{
    int curAngle = m_magnetSensor.GetRawAngleFetchIfStale();

    int diffAngle = targetAngle - curAngle;
    if (diffAngle > MagnetSensor::c_angleRange / 2)
    {
        diffAngle -= MagnetSensor::c_angleRange;
    }
    if (diffAngle < -MagnetSensor::c_angleRange / 2)
    {
        diffAngle += MagnetSensor::c_angleRange;
    }

    MotorDirection direction = diffAngle > 0 ? MotorDirection::Right : MotorDirection::Left;
    
    return RotateToDirectionAsync(direction, targetAngle, dutyPercent);
}

std::future<HwResult> NozzleControl::SetPressureAsync(int targetPressure, int dutyPercent)
{
    int startPressure = m_pressureSensor.GetPressureFetchIfStale();

    int diff = targetPressure - startPressure;
    int distance = abs(diff);

    static constexpr int epsilon = 10;
    if (distance < epsilon)
    {
        return GetCompletedFuture(HwResult::Success);
    }

    if (targetPressure < m_pressureSensor.GetMinPressure())
    {
        cerr << "targetPressure too low" << endl;
        return GetCompletedFuture(HwResult::UnexpectedValue);
    }

    MotorDirection direction = diff > 0 ? MotorDirection::Open : MotorDirection::Close;

    m_motorValve.Run(direction, dutyPercent);

    static constexpr int inertialConst = 50;
    static constexpr int trendAnalyzerSize = 16;
    int inertialOffset = inertialConst * dutyPercent / 100;
    inertialOffset = max(min(inertialOffset, distance / 2), epsilon);

    std::function<HwResult(int)> isExpectedValue;

    if (direction == MotorDirection::Open)
    {
        int thresholdPressure = targetPressure - inertialOffset;
        isExpectedValue = [thresholdPressure, analyzer = SequenceTrendAnalyzer<trendAnalyzerSize>()] (int curPressure) mutable {
            analyzer.Push(curPressure);
            if (analyzer.IsFull() && analyzer.CurTrend(trendAnalyzerSize / 2) <= 0) {
                return HwResult::MaxValueReached;
            }
            return curPressure > thresholdPressure ? HwResult::Success : HwResult::Repeat;
        };
    }
    else
    {
        int thresholdPressure = targetPressure + inertialOffset;
        isExpectedValue = [thresholdPressure, analyzer = SequenceTrendAnalyzer<trendAnalyzerSize>()] (int curPressure) mutable {
            analyzer.Push(curPressure);
            if (analyzer.IsFull() && analyzer.CurTrend(trendAnalyzerSize / 2) >= 0) {
                return HwResult::UnexpectedValue;
            }
            return curPressure < thresholdPressure ? HwResult::Success : HwResult::Repeat;
        };
    }

    return m_pressureSensor.NotifyWhenPressure(move(isExpectedValue),
        [this] (HwResult) { m_motorValve.Stop(); });
}

std::future<HwResult> NozzleControl::SetPressureDiffAsync(int diffPressure, int dutyPercent)
{
    int curPressure = GetPressureFetchIfStale();
    return SetPressureAsync(curPressure + diffPressure, dutyPercent);
}

bool NozzleControl::IsWaterPressurePresent()
{
    return IsItWaterPressure(m_pressureSensor.GetPressureFetchIfStale());
}

HwResult NozzleControl::OpenValve()
{
    auto futureOpen = OpenValveAsync();
    futureOpen.wait();
    return futureOpen.get();
}

std::future<HwResult> NozzleControl::OpenValveAsync()
{
    LogInfo("");

    if (IsWaterPressurePresent())
    {
        LogWarning("Water pressure already present");
        return GetCompletedFuture(HwResult::Success);
    }

    return OpenValveInternalAsync(MotorDirection::Open);
}

std::future<HwResult> NozzleControl::OpenValveInternalAsync(MotorDirection direction)
{
    m_motorValve.Run(direction, c_defaultDutyPercent);

    std::function<HwResult(int)> isExpectedValue = [this] (int curPressure) {
        auto curTime = chrono::steady_clock::now();
        int duration = static_cast<int>(chrono::duration_cast<chrono::milliseconds>(curTime - m_motorValve.RunStartedAt()).count());
        if (duration > c_valveOpeningTimeoutMs) {
            return HwResult::Timeout;
        }
        return IsItWaterPressure(curPressure) ? HwResult::Success : HwResult::Repeat;
    };

    std::function<void(HwResult)> completionAction = [this] (HwResult status) {
        m_motorValve.Stop();
        if (status != HwResult::Success) {
            auto fut = CloseValveAsync();
            fut.wait();
        }
    };
    
    return m_pressureSensor.NotifyWhenPressure(move(isExpectedValue), move(completionAction));
}

HwResult NozzleControl::CloseValve(bool fCloseTight)
{
    auto futureClose = CloseValveAsync();
    futureClose.wait();
    IfFailRetResult(futureClose.get());

    if (fCloseTight && m_closedValveDurationMs != 0)
    {
        // Rotate valve to the middle closed position
        m_motorValve.RunDuration(MotorDirection::Close, chrono::milliseconds(m_closedValveDurationMs / 2), c_defaultDutyPercent);
    }

    return HwResult::Success;
}

std::future<HwResult> NozzleControl::CloseValveAsync()
{
    LogInfo("");

    if (!IsWaterPressurePresent())
    {
        LogWarning("Water pressure not present");
        m_motorValve.RestoreInitialPosition();
        return GetCompletedFuture(HwResult::NoWaterPressure);
    }

    return CloseValveInternalAsync(MotorDirection::Close);
}

std::future<HwResult> NozzleControl::CloseValveInternalAsync(MotorDirection direction)
{
    static constexpr int trendAnalyzerSize = 16;
    m_motorValve.Run(direction, c_defaultDutyPercent);

    std::function<HwResult(int)> isExpectedValue = [this, analyzer = SequenceTrendAnalyzer<trendAnalyzerSize>()] (int curPressure) mutable {
        analyzer.Push(curPressure);
        if (analyzer.IsFull() && analyzer.CurTrend(trendAnalyzerSize / 2) >= 0 &&
            !IsItWaterPressure(curPressure)) {
            return HwResult::Success;
        }
        return HwResult::Repeat;
    };

    return m_pressureSensor.NotifyWhenPressure(move(isExpectedValue),
        [this] (HwResult) { m_motorValve.Stop(); });
}

void NozzleControl::StopMotorValveIfRunning(int curPressure, int changeRate)
{
    if (m_motorValve.IsRunning())
    {
        m_motorValve.Stop();
        m_pressureAtMotorStop = curPressure;
        m_changeRateAtMotorStop = changeRate;
    }
}

HwResult NozzleControl::ProcessPressureMeasurement(int curPressure)
{
    ++m_iPressureMeasurementAfterMotorStart;
    int changeRate = m_pressureSensor.GetLastChangeRate();

    if (m_iPressureMeasurementAfterMotorStart > c_minPressureMeasurementAfterMotorStart &&
        changeRate * m_motorValve.GetLastDirectionSign() <= 0)
    {
        if (m_motorValve.IsRunning())
        {
            // Valve reached maximum opening position and is closing in opposite direction
            m_motorValve.Stop();
            cerr << "Valve reached maximum opening position and is closing in opposite direction" << endl;
            LogError("Valve reached maximum opening position and is closing in opposite direction, "
                "pressure: %d, changeRate: %d", curPressure, changeRate);
            return HwResult::MaxValueReached;
        }
        else if (m_pressureAtMotorStop > 0)
        {
            // Change rate is zero or opposite to the last direction, means valve stopped completely
            m_overshootInterpolator.SetOvershoot(m_changeRateAtMotorStop, abs(curPressure - m_pressureAtMotorStop));
            m_pressureAtMotorStop = -1;
            m_changeRateAtMotorStop = 0;
        }
    }

    int targetPressure = m_targetPressure.load();
    int diff = targetPressure - curPressure;

    if (abs(diff) <= c_pressureTolerance)
    {
        StopMotorValveIfRunning(curPressure, changeRate);
        return HwResult::Repeat;
    }

    MotorDirection requiredDirection = diff > 0 ? MotorDirection::Open : MotorDirection::Close;
    if (!m_motorValve.IsRunning() || m_motorValve.GetLastDirection() != requiredDirection)
    {
        // Motor is not running or running in opposite direction
        // Start motor in required direction
        LogInfo("Starting motor in required direction: %d", static_cast<int>(requiredDirection));
        m_motorValve.Run(requiredDirection, c_defaultDutyPercent);
        m_iPressureMeasurementAfterMotorStart = 0;
        return HwResult::Repeat;
    }

    // Motor is running in required direction

    if (m_iPressureMeasurementAfterMotorStart < c_minPressureMeasurementAfterMotorStart)
    {
        /// not enough measurements to check if stop condition is met
        return HwResult::Repeat;
    }
    
    // Check if stop condition is met
    int predictedOvershoot = m_overshootInterpolator.PredictOvershoot(changeRate);
    
    // overshoot is always positive, add or subtract it depending on the direction
    int adjustedTargetPressure = targetPressure - predictedOvershoot * m_motorValve.GetLastDirectionSign();

    bool shouldStop = (requiredDirection == MotorDirection::Open && curPressure >= adjustedTargetPressure) ||
                  (requiredDirection == MotorDirection::Close && curPressure <= adjustedTargetPressure);
    if (shouldStop)
    {
        // Stop the motor
        LogInfo("Stopping motor, curPressure: %d, rate: %d, target: %d, adjustedTarget: %d", 
                curPressure, changeRate, targetPressure, adjustedTargetPressure);
        m_motorValve.Stop();
        return HwResult::Repeat;
    }

    return HwResult::Repeat;
}

NozzleControlCalibrated::NozzleControlCalibrated() :
    m_spPressureInterpolator(new Interpolator)
{
    m_spRotationInterpolator.emplace_back(new Interpolator); // Left direction
    m_spRotationInterpolator.emplace_back(new Interpolator); // Right direction
}

NozzleControlCalibrated::~NozzleControlCalibrated()
{
}

HwResult NozzleControlCalibrated::CalibrateNozzle(int minDurationUs, int maxDurationUs, double multiplier)
{
    for (int iDirection = 0; iDirection < 2; ++iDirection)
    {
        string logMessage = "Distance/duration values: [";
        vector<pair<int,int>> angleDuration;
        MotorDirection direction = static_cast<MotorDirection>(iDirection);
        const auto calmDownDuration = chrono::milliseconds(300);
        m_motorNozzle.RunDuration(direction, chrono::milliseconds(50), c_defaultDutyPercent);
        this_thread::sleep_for(calmDownDuration);

        int startAngle = GetPositionFetch();
        for (int durationUs = minDurationUs; durationUs < maxDurationUs;)
        {
            m_motorNozzle.RunDuration(direction, chrono::microseconds(durationUs), c_defaultDutyPercent);
            this_thread::sleep_for(calmDownDuration);
            int endAngle = GetPositionFetch();
            int distance = endAngle - startAngle;
            startAngle = endAngle;

            if (direction == MotorDirection::Left)
            {
                distance = -distance;
            }
            
            if (distance < 0)
            {
                distance += MagnetSensor::c_angleRange;
            }

            angleDuration.emplace_back(distance, durationUs);
            logMessage += "[" + to_string(distance) + ", " + to_string(durationUs) + "], ";
            
            durationUs = static_cast<int>(static_cast<double>(durationUs) * multiplier);
        }

        logMessage.pop_back();
        logMessage.back() = ']';

        LogInfo(logMessage.c_str());

        m_spRotationInterpolator[iDirection]->SetValues(move(angleDuration));
    }

    return HwResult::Success;
}

HwResult NozzleControlCalibrated::CalibrateValve(int stepDurationMs)
{
    IfFailRetResult(FindCloseTightPosition());

    IfFailRetResult(OpenValve());
    IfFailRetResult(CloseValve(false /*fCloseTight*/));

    vector<pair<int,int>> pressureDuration;
    int curPressure = GetPressureFetch();
    pressureDuration.emplace_back(curPressure, 0);
    const auto calmDownDuration = chrono::milliseconds(stepDurationMs * 2);
    int prevPressure = curPressure;

    for (int durationMs = stepDurationMs; ; durationMs += stepDurationMs)
    {
        TurnValve(MotorDirection::Open, chrono::milliseconds(stepDurationMs), c_defaultDutyPercent);
        this_thread::sleep_for(calmDownDuration);
        curPressure = GetPressureFetch();
        if (prevPressure >= curPressure)
        {
            break;
        }
        pressureDuration.emplace_back(curPressure, durationMs);
        prevPressure = curPressure;
    }
    
    m_spPressureInterpolator->SetValues(move(pressureDuration));

    IfFailRetResult(CloseValve(true /*fCloseTight*/));

    return HwResult::Success;
}

HwResult NozzleControlCalibrated::FindCloseTightPosition()
{
    IfFailRetResult(OpenValve());
    IfFailRetResult(CloseValve(false /*fCloseTight*/));

    auto startTime = chrono::steady_clock::now();
    auto futureOpen = OpenValveInternalAsync(MotorDirection::Close);
    futureOpen.wait();
    IfFailRetResult(futureOpen.get());
    auto endTime = chrono::steady_clock::now();

    m_closedValveDurationMs = static_cast<int>(chrono::duration_cast<chrono::milliseconds>(endTime - startTime).count());

    auto futureClose = CloseValveInternalAsync(MotorDirection::Open);
    futureClose.wait();
    IfFailRetResult(futureClose.get());

    // Rotate valve to the middle closed position
    m_motorValve.RunDuration(MotorDirection::Open, chrono::milliseconds(m_closedValveDurationMs / 2), c_defaultDutyPercent);

    return HwResult::Success;
}

void NozzleControlCalibrated::RotateDiffDurationBased(int diffAngle)
{
    MotorDirection direction = diffAngle > 0 ? MotorDirection::Right : MotorDirection::Left;
    int durationUs = m_spRotationInterpolator[static_cast<int>(direction)]->Predict(abs(diffAngle));

    {
        string logMessage = "curPosition: " + to_string(GetPosition());
        logMessage += ", diffAngle: " + to_string(diffAngle) + ", durationUs: " + to_string(durationUs);
        LogInfo(logMessage.c_str());
    }
    
    if (durationUs > 0)
    {
        m_motorNozzle.RunDuration(direction, chrono::microseconds(durationUs), c_defaultDutyPercent);
    }
}

#if 0
future<HwResult> NozzleControlCalibrated::RotateDiffDurationBasedAsync(int diffAngle)
{
    MotorDirection direction = diffAngle > 0 ? MotorDirection::Right : MotorDirection::Left;
    
    promise<HwResult> completionPromise;
    auto completionFuture = completionPromise.get_future();
    m_rotationTimer.SetCallback([this, comPromise=move(completionPromise)] () mutable {
        m_motorNozzle.Stop();
        comPromise.set_value(HwResult::Success);
    });

    int durationUs = m_rotationInterpolator[static_cast<int>(direction)].Predict(diffAngle);

    m_motorNozzle.Run(direction, c_defaultDutyPercent);
    m_rotationTimer.Start(durationUs / 1000);
    
    return completionFuture;
}
#endif

void NozzleControlCalibrated::SetPressureDurationBased(int targetPressure)
{
    int curPressure = GetPressureFetchIfStale();
    int curDurationMs = m_spPressureInterpolator->Predict(curPressure);
    int targetDurationMs = m_spPressureInterpolator->Predict(targetPressure);
    
    MotorDirection direction = targetPressure > curPressure ? MotorDirection::Open : MotorDirection::Close;
    int durationMs = abs(targetDurationMs - curDurationMs);
    
    if (durationMs > 0)
    {
        m_motorValve.RunDuration(direction, chrono::milliseconds(durationMs), c_defaultDutyPercent);
    }
}
