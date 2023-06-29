#pragma once

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

struct ConfigReg
{
    union
	{
		unsigned char raw[2];
		struct
        {
            unsigned int sf : 2;
            unsigned int fth : 3;
            unsigned int wd : 1;
            unsigned int __unused : 2;
            unsigned int pm : 2;
            unsigned int hyst : 2;
            unsigned int outs : 2;
            unsigned int pwmf : 2;
        };
	};
};

struct StatusReg
{
    union
    {
        unsigned char raw;
        struct
        {
            unsigned int __unused : 3;
            unsigned int magnet_high : 1;
            unsigned int magnet_low : 1;
            unsigned int magnet_detected : 1;
        };
    };
};
