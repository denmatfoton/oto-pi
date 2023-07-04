#pragma once

#include <cstdint>

class MagnetSensor
{
    static constexpr int c_sensorAddress = 0x36;

public:
    MagnetSensor(int i2cHandle);

    void Configure();

    void ReadConfig();
    void ReadStatus();
    float ReadAngle();
    
private:
    const int m_i2cHandle;
    const int dummy = 9;
};

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
