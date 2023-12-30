#pragma once

#include <climits>

#include <atomic>
#include <functional>
#include <future>

class I2cAccessor;
class I2cTransaction;

enum class HwResult : int;

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
    static constexpr int c_truncateShift = 12;

public:
    PressureSensor(I2cAccessor& i2cAccessor) : m_i2cAccessor(i2cAccessor) {}

    std::future<HwResult> ReadPressureAsync();
    std::future<HwResult> NotifyWhenPressure(std::function<HwResult(int)>&& isExpectedValue,
        std::function<void(HwResult)>&& completionAction);

    int GetLastRawPressure() const { return m_lastRawValue.load(); }
    int GetLastPressure() const { return TruncateNoise(GetLastRawPressure()); }
    int GetMinRawPressure() const { return m_minRawValue.load(); }
    int GetMinPressure() const { return TruncateNoise(GetMinRawPressure()); }
    uint32_t GetLastMeasurementTimeMs() const { return m_lastMeasurementTimeMs.load(); }
    bool IsMeasurementStale() const;
    int GetPressureFetchIfStale();

    static int TruncateNoise(int rawValue) { return rawValue >> c_truncateShift; }

    static float ConvertRawToPsi(int rawValue)
    {
        return static_cast<float>(rawValue * (c_maxPsi - c_minPsi)) 
            / static_cast<float>(c_outputMax - c_outputMin) + static_cast<float>(c_minPsi);
    }

    static float ConvertToPsi(int value)
    {
        return ConvertRawToPsi(value << c_truncateShift);
    }

private:
    void FillI2cTransaction(I2cTransaction& transaction);

    I2cAccessor& m_i2cAccessor;
    std::atomic_int32_t m_lastRawValue = 0;
    std::atomic_int32_t m_minRawValue = INT_MAX / 2;
    std::atomic_uint32_t m_lastMeasurementTimeMs = 0;
};
