#pragma once

class PressureSensor
{
    static constexpr int c_sensorAddress = 0x18;
    static constexpr int c_busyFlag = 0x20;
    static constexpr int c_itegrityFlag = 0x04;
    static constexpr int c_mathSatFlag = 0x01;
    static constexpr int c_maxPsi = 25;
    static constexpr int c_minPsi = 0;
    static constexpr int c_outputMax = 0xE66666;
    static constexpr int c_outputMin = 0x19999A;

public:
    PressureSensor(int i2cHandle) : m_i2cHandle(i2cHandle) {}

    float ReadPressurePsi();
    int ReadRawPressue();

private:
    const int m_i2cHandle;
};
