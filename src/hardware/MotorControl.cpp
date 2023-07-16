#include "I2cAccessor.h"
#include "MotorControl.h"
#include "Utils.h"

#include <pigpio.h>

#include <iostream>

using namespace std;

MotorControl::MotorControl(int pwmPin, int drainPin) :
    m_pwmPin(pwmPin), m_drainPin(drainPin)
{
    if (InitializeGpio() < 0)
    {
        cerr << "InitializeGpio failed" << endl;
    }

    gpioSetMode(m_pwmPin, PI_OUTPUT);
    gpioWrite(m_pwmPin, 0);
    gpioSetMode(m_drainPin, PI_OUTPUT);
    gpioWrite(m_drainPin, 0);
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
    int drainValue = static_cast<int>(direction);
    dutyPercent += (100 - dutyPercent * 2) * drainValue; // Invert duty in case drainValue == 1.
    int duty = dutyPercent * 10000;

    int status = gpioHardwarePWM(m_pwmPin, c_pwmFreq, duty);
    if (status != 0)
    {
        cerr << "gpioHardwarePWM failed with: " << status << endl;
        return;
    }

    gpioWrite(m_drainPin, drainValue);

    m_runStart = chrono::steady_clock::now();
    m_runDirection = drainValue * 2 - 1;
}

void MotorControl::Stop()
{
    int status = gpioHardwarePWM(m_pwmPin, 0, 0);
    if (status != 0)
    {
        cerr << "gpioHardwarePWM failed with: " << status << endl;
        return;
    }

    if (m_runDirection != 0)
    {
        auto end = chrono::steady_clock::now();
        int duration = static_cast<int>(chrono::duration_cast<chrono::milliseconds>(end - m_runStart).count());
        m_directionalTimeAccumulatorMs += duration * m_runDirection;
        m_runDirection = 0;
    }

    gpioWrite(m_pwmPin, 0);
    gpioWrite(m_drainPin, 0);
}

void MotorControl::RestoreInitialPosition()
{
    cout << "RestoreInitialPosition, m_directionalTimeAccumulatorMs: " << m_directionalTimeAccumulatorMs << endl;
    MotorDirection direction = m_directionalTimeAccumulatorMs > 0 ? MotorDirection::Close : MotorDirection::Open;
    Run(direction, 100);
    this_thread::sleep_for(chrono::milliseconds(abs(m_directionalTimeAccumulatorMs)));
    Stop();
}



template<typename T>
std::future<T> GetCompletedFuture(T value)
{
    promise<T> pr;
    auto fut = pr.get_future();
    pr.set_value(value);
    return fut;
}

std::future<I2cStatus> NozzleControl::RotateTo(MotorDirection direction, int targetAngle, int dutyPercent)
{
    int startAngle = m_magnetSensor.GetRawAngleFetchIfStale();
    cout << "startAngle: " << startAngle << endl;
    cout << "targetAngle: " << targetAngle << endl;

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

    cout << "distance: " << distance << endl;

    static constexpr int epsilon = 3;
    static constexpr int inertionConst = 30;

    if (distance <= epsilon)
    {
        return GetCompletedFuture(I2cStatus::Success);
    }

    m_motorNozzle.Run(direction, dutyPercent);

    int inertionOffset = inertionConst * dutyPercent / 100;
    inertionOffset = max(min(inertionOffset, distance / 2), epsilon);

    int maxAngle = targetAngle + epsilon;
    int minAngle = targetAngle - epsilon;

    if (direction == MotorDirection::Right)
    {
        minAngle = targetAngle - inertionOffset;
    }
    else
    {
        maxAngle = targetAngle + inertionOffset;
    }

    minAngle = (minAngle + MagnetSensor::c_angleRange) % MagnetSensor::c_angleRange;
    maxAngle %= MagnetSensor::c_angleRange;

    std::function<I2cStatus(int)> isExpectedValue;

    cout << "minAngle: " << minAngle << ", maxAngle: " << maxAngle << endl;
    
    if (maxAngle > minAngle)
    {
        isExpectedValue = [maxAngle, minAngle] (int curAngle) {
            return curAngle < maxAngle && curAngle > minAngle ? I2cStatus::Success : I2cStatus::Repeat;
        };
    }
    else {
        isExpectedValue = [maxAngle, minAngle] (int curAngle) {
            return curAngle < maxAngle || curAngle > minAngle ? I2cStatus::Success : I2cStatus::Repeat;
        };
    };

    return m_magnetSensor.NotifyWhenAngle(move(isExpectedValue),
        [this] (I2cStatus) { m_motorNozzle.Stop(); });
}

std::future<I2cStatus> NozzleControl::SetPressure(int targetPressure, int dutyPercent)
{
    int startPressure = m_pressureSensor.GetRawPressureFetchIfStale();

    int diff = targetPressure - startPressure;
    int distance = abs(diff);

    static constexpr int epsilon = 1000;
    if (distance < epsilon)
    {
        return GetCompletedFuture(I2cStatus::Success);
    }

    if (targetPressure < m_pressureSensor.GetMinRawPressure())
    {
        cerr << "targetPressure too low" << endl;
        return GetCompletedFuture(I2cStatus::UnexpectedValue);
    }

    MotorDirection direction = diff > 0 ? MotorDirection::Open : MotorDirection::Close;

    m_motorValve.Run(direction, dutyPercent);

    static constexpr int inertionConst = 10'000;
    int inertionOffset = inertionConst * dutyPercent / 100;
    inertionOffset = max(min(inertionOffset, distance / 2), epsilon);

    std::function<I2cStatus(int)> isExpectedValue;

    if (direction == MotorDirection::Open)
    {
        int thresholdPressure = targetPressure - inertionOffset;
        isExpectedValue = [thresholdPressure, analyzer = SequenceTrendAnalyzer<16>()] (int curPressure) mutable {
            analyzer.Push(curPressure);
            if (analyzer.IsFull() && analyzer.CurTrend() < 200) {
                return I2cStatus::MaxValueReached;
            }
            return curPressure > thresholdPressure ? I2cStatus::Success : I2cStatus::Repeat;
        };
    }
    else
    {
        int thresholdPressure = targetPressure + inertionOffset;
        isExpectedValue = [thresholdPressure, analyzer = SequenceTrendAnalyzer<16>()] (int curPressure) mutable {
            analyzer.Push(curPressure);
            if (analyzer.IsFull() && analyzer.CurTrend() > -200) {
                return I2cStatus::UnexpectedValue;
            }
            return curPressure < thresholdPressure ? I2cStatus::Success : I2cStatus::Repeat;
        };
    }

    return m_pressureSensor.NotifyWhenPressure(move(isExpectedValue),
        [this] (I2cStatus) { m_motorValve.Stop(); });
}

bool NozzleControl::IsWaterPressurePresent()
{
    return IsItWaterPressure(m_pressureSensor.GetRawPressureFetchIfStale());
}

std::future<I2cStatus> NozzleControl::OpenValve()
{
    if (IsWaterPressurePresent())
    {
        return GetCompletedFuture(I2cStatus::Success);
    }

    static constexpr int dutyPercent = 100;
    m_motorValve.Run(MotorDirection::Open, dutyPercent);

    std::function<I2cStatus(int)> isExpectedValue = [this] (int curPressure) {
        auto curTime = chrono::steady_clock::now();
        int duration = static_cast<int>(chrono::duration_cast<chrono::milliseconds>(curTime - m_motorValve.RunStartedAt()).count());
        if (duration > c_valveOpeningTimeoutMs) {
            return I2cStatus::Timeout;
        }
        return IsItWaterPressure(curPressure) ? I2cStatus::Success : I2cStatus::Repeat;
    };

    std::function<void(I2cStatus)> completionAction = [this] (I2cStatus status) {
        m_motorValve.Stop();
        if (status != I2cStatus::Success) {
            auto fut = CloseValve();
            fut.wait();
        }
    };
    
    return m_pressureSensor.NotifyWhenPressure(move(isExpectedValue), move(completionAction));
}

std::future<I2cStatus> NozzleControl::CloseValve()
{
    if (!IsWaterPressurePresent())
    {
        m_motorValve.RestoreInitialPosition();
        return GetCompletedFuture(I2cStatus::Success);
    }

    static constexpr int dutyPercent = 100;
    m_motorValve.Run(MotorDirection::Close, dutyPercent);

    std::function<I2cStatus(int)> isExpectedValue = [this, analyzer = SequenceTrendAnalyzer<16>()] (int curPressure) mutable {
        analyzer.Push(curPressure);
        if (analyzer.IsFull() && analyzer.CurTrend() < 10 &&
            !IsItWaterPressure(curPressure)) {
            return I2cStatus::Success;
        }
        return I2cStatus::Repeat;
    };

    return m_pressureSensor.NotifyWhenPressure(move(isExpectedValue),
        [this] (I2cStatus) { m_motorValve.Stop(); });
}

void NozzleControl::TurnValve(MotorDirection direction, std::chrono::milliseconds duration, int dutyPercent)
{
    m_motorValve.Run(direction, dutyPercent);
    this_thread::sleep_for(duration);
    m_motorValve.Stop();
}
