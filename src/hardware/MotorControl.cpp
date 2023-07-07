#include "I2cAccessor.h"
#include "MotorControl.h"
#include "Utils.h"

#include <pigpio.h>

#include <chrono>
#include <iostream>
#include <thread>

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
}

void MotorControl::Stop()
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

std::future<I2cStatus> Nozzle::RotateTo(MotorDirection direction, int targetAngle, int dutyPercent)
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
        promise<I2cStatus> pr;
        auto fut = pr.get_future();
        pr.set_value(I2cStatus::Success);
        return fut;
    }

    m_motor.Run(direction, dutyPercent);

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

    std::function<bool(int)> isExpectedValue;

    cout << "minAngle: " << minAngle << ", maxAngle: " << maxAngle << endl;
    
    if (maxAngle > minAngle)
    {
        isExpectedValue = [maxAngle, minAngle] (int curAngle) {
            return curAngle < maxAngle && curAngle > minAngle;
        };
    }
    else {
        isExpectedValue = [maxAngle, minAngle] (int curAngle) {
            return curAngle < maxAngle || curAngle > minAngle;
        };
    };

    return m_magnetSensor.NotifyWhenAngle(move(isExpectedValue),
        [this] (I2cStatus) { m_motor.Stop(); });
}
