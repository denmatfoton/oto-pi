#pragma once

#include <PolarCoordinates.h>

#include <cstdint>

#include <atomic>
#include <chrono>
#include <functional>
#include <future>

class I2cAccessor;
class I2cTransaction;

enum class HwResult : int;

union ConfigReg
{
    int raw;
    struct Fields
    {
        uint32_t sf : 2;
        uint32_t fth : 3;
        uint32_t wd : 1;
        uint32_t __unused : 2;
        uint32_t pm : 2;
        uint32_t hyst : 2;
        uint32_t outs : 2;
        uint32_t pwmf : 2;
    } fields;
};

union StatusReg
{
    unsigned char raw;
    struct Fields
    {
        uint32_t __unused : 3;
        uint32_t magnet_high : 1;
        uint32_t magnet_low : 1;
        uint32_t magnet_detected : 1;
    } fields;
};

class MagnetSensor
{
    static constexpr int c_sensorAddress = 0x36;

public:
    static constexpr int c_angleRange = HwCoord::c_angleRange;

    MagnetSensor(I2cAccessor& i2cAccessor) : m_i2cAccessor(i2cAccessor) {}

    void ReadConfig();
    void ReadStatus();

    std::future<HwResult> ReadAngleAsync();
    std::future<HwResult> NotifyWhenAngle(std::function<HwResult(int)>&& isExpectedValue,
        std::function<void(HwResult)>&& completionAction);
    
    int GetLastRawAngle() const { return m_lastRawAngle.load(); }
    uint32_t GetLastMeasurementTimeMs() const { return m_lastMeasurementTimeMs.load(); }
    bool IsMeasurementStale() const;
    int GetRawAngleFetchIfStale();

private:
    void FillI2cTransactionReadAngle(I2cTransaction& transaction);

    I2cAccessor& m_i2cAccessor;
    std::atomic_int32_t m_lastRawAngle = 0;
    std::atomic_uint32_t m_lastMeasurementTimeMs = 0;
};
