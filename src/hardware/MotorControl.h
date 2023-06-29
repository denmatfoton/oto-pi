#pragma once

enum class MotorUnit : int
{
    Nozzle,
    Valve,
};

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
    static constexpr int c_pwmPinValve = 13;
    static constexpr int c_drainPinValve = 6;

    static constexpr int c_pwmPinNozzle = 18;
    static constexpr int c_drainPinNozzle = 17;

    static constexpr int c_pwmFreq = 1000;

public:
    MotorControl(MotorUnit motorUnit);

    void RunMotor(MotorDirection direction, int dutyPercent, int durationMs);
    void StopMotor();

    static int InitializeGpio();

private:
    int m_pwmPin = -1;
    int m_drainPin = -1;
};
