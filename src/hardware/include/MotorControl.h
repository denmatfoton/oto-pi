#pragma once

#include "CommonDefs.h"

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
