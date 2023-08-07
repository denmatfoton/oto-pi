#include "NozzleControl.h"

#include "I2cAccessor.h"
#include "Utils.h"

#include <iostream>

using namespace std;

template<typename T>
static std::future<T> GetCompletedFuture(T value)
{
    promise<T> pr;
    auto fut = pr.get_future();
    pr.set_value(value);
    return fut;
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
    CloseValve();
}

int NozzleControl::Init()
{
    IfFailRet(m_spI2cAccessor->Init(c_i2cFileName));
    IfFailRet(m_motorNozzle.Init());
    IfFailRet(m_motorValve.Init());
    cout << "NozzleControl::Init succeeded" << endl;
    return 0;
}

std::future<I2cStatus> NozzleControl::RotateTo(MotorDirection direction, int targetAngle, int dutyPercent)
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
        return GetCompletedFuture(I2cStatus::Success);
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

std::future<I2cStatus> NozzleControl::RotateDiff(int diffAngle, int dutyPercent)
{
    int curPosition = GetPositionFetchIfStale();

    MotorDirection direction = diffAngle > 0 ? MotorDirection::Right : MotorDirection::Left;
    int targetAngle = (curPosition + diffAngle + MagnetSensor::c_angleRange) % MagnetSensor::c_angleRange;

    return RotateTo(direction, targetAngle, dutyPercent);
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

    static constexpr int inertialConst = 10'000;
    int inertialOffset = inertialConst * dutyPercent / 100;
    inertialOffset = max(min(inertialOffset, distance / 2), epsilon);

    std::function<I2cStatus(int)> isExpectedValue;

    if (direction == MotorDirection::Open)
    {
        int thresholdPressure = targetPressure - inertialOffset;
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
        int thresholdPressure = targetPressure + inertialOffset;
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

std::future<I2cStatus> NozzleControl::SetPressureDiff(int diffPressure, int dutyPercent)
{
    int curPressure = GetPressureFetchIfStale();
    return SetPressure(curPressure + diffPressure, dutyPercent);
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
