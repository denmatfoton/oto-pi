#pragma once

#include <climits>

#include <atomic>
#include <functional>
#include <future>

class I2cAccessor;
class I2cTransaction;

enum class I2cStatus : int;

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
    static constexpr int c_approximateAmbientPressure = 7'740'000;

    PressureSensor(I2cAccessor& i2cAccessor) : m_i2cAccessor(i2cAccessor) {}

    std::future<I2cStatus> ReadPressureAsync();
    std::future<I2cStatus> NotifyWhenPressure(std::function<I2cStatus(int)>&& isExpectedValue,
        std::function<void(I2cStatus)>&& completionAction);

    int GetLastRawPressure() { return m_lastRawValue.load(); }
    int GetMinRawPressure() { return m_minRawValue.load(); }
    uint32_t GetLastMeasurmentTimeMs() { return m_lastMeasurmentTimeMs.load(); }
    bool IsMeasurmentStale();
    int GetRawPressureFetchIfStale();

    static float ConvertToPsi(int rawValue)
    {
        return static_cast<float>(rawValue * (c_maxPsi - c_minPsi)) 
            / static_cast<float>(c_outputMax - c_outputMin) + static_cast<float>(c_minPsi);
    }

private:
    void FillI2cTransaction(I2cTransaction& transaction);

    I2cAccessor& m_i2cAccessor;
    std::atomic_int32_t m_lastRawValue = 0;
    std::atomic_int32_t m_minRawValue = INT_MAX;
    std::atomic_uint32_t m_lastMeasurmentTimeMs = 0;
};
