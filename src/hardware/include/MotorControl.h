#pragma once

#include "CommonDefs.h"

#include <thread>

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

    static constexpr int c_pwmFreq = 10000;

public:
    MotorControl(int pwmPin, int drainPin) :
        m_pwmPin(pwmPin), m_drainPin(drainPin) {}
    ~MotorControl() { Stop(); }

    int Init();
    void Run(MotorDirection direction, int dutyPercent);
    template <class _Rep, class _Period>
    void RunDuration(MotorDirection direction, const std::chrono::duration<_Rep, _Period>& duration, int dutyPercent)
    {
        Run(direction, dutyPercent);
        std::this_thread::sleep_for(duration);
        Stop();
    }
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
