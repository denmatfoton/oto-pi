#pragma once

#include <atomic>
#include <functional>
#include <future>

class I2cAccessor;
class I2cTransaction;

class PressureSensor
{
    static constexpr int c_sensorAddress = 0x18;
    static constexpr int c_busyFlag = 0x20;
    static constexpr int c_integrityFlag = 0x04;
    static constexpr int c_mathSatFlag = 0x01;
    static constexpr int c_maxPsi = 25;
    static constexpr int c_minPsi = 0;
    static constexpr int c_outputMax = 0xE66666;
    static constexpr int c_outputMin = 0x19999A;

public:
    PressureSensor(I2cAccessor& i2cAccessor) : m_i2cAccessor(i2cAccessor) {}

#if 0
    float ReadPressurePsi();
    int ReadRawPressure();
#endif

    std::future<int> ReadRawPressureAsync();
    void NotifyWhen(std::function<bool(int)> isExpectedValue,
        std::function<void(int)> callback);

    static float ConvertToPsi(int rawValue)
    {
        return static_cast<float>((rawValue - c_outputMin) * (c_maxPsi - c_minPsi)) 
            / static_cast<float>(c_outputMax - c_outputMin) + static_cast<float>(c_minPsi);
    }

private:
    void FillI2cTransaction(I2cTransaction& transaction);

    I2cAccessor& m_i2cAccessor;
    std::atomic_int32_t m_lastRawValue = 0;
};
