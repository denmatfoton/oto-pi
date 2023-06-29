#include "MagnetSensor.h"

extern "C" {
#include <i2c/smbus.h>
}
#include <linux/i2c-dev.h>
#include <math.h>
#include <sys/ioctl.h>
#include <unistd.h>

#include <iostream>

using namespace std;

MagnetSensor::MagnetSensor(int i2cHandle) : m_i2cHandle(i2cHandle)
{
}

int SwapBytesWord(int w)
{
    return ((w >> 8) & 0xFF) | ((w & 0xFF) << 8);
}

void MagnetSensor::ReadConfig()
{
    if (ioctl(m_i2cHandle, I2C_SLAVE, c_sensorAddress) < 0)
    {
        cerr << "ReadConfig: ioctl failed" << endl;
        return;
    }
    
    int zpos = i2c_smbus_read_word_data(m_i2cHandle, 1);
    cout << "ZPOS: " << zpos << endl;
    
    int mpos = i2c_smbus_read_word_data(m_i2cHandle, 3);
    cout << "MPOS: " << mpos << endl;
    
    int mang = i2c_smbus_read_word_data(m_i2cHandle, 5);
    cout << "MANG: " << mang << endl;

    int wConf = i2c_smbus_read_word_data(m_i2cHandle, 0x7);
    if (wConf < 0)
    {
        cerr << "ReadConfig: read word failed. status: " << wConf << endl;
        return;
    }

    ConfigReg confReg { static_cast<__u8>(wConf & 0xFF), static_cast<__u8>(wConf >> 8) };
    
    cout << "WD: " << confReg.wd << endl;
    cout << "FTH: " << confReg.fth << endl;
    cout << "SF: " << confReg.sf << endl;

    cout << "PWMF: " << confReg.pwmf << endl;
    cout << "OUTS: " << confReg.outs << endl;
    cout << "HYST: " << confReg.hyst << endl;
    cout << "PM: " << confReg.pm << endl << endl;
}

void MagnetSensor::ReadStatus()
{
    if (ioctl(m_i2cHandle, I2C_SLAVE, c_sensorAddress) < 0)
    {
        cerr << "ReadStatus: ioctl failed" << endl;
        return;
    }

    int b = i2c_smbus_read_byte_data(m_i2cHandle, 0x0B);
    if (b < 0)
    {
        cerr << "ReadConfig: read status failed. status: " << b << endl;
        return;
    }

    StatusReg statusReg { static_cast<__u8>(b) };

    cout << "Magnet high: " << statusReg.magnet_high << endl;
    cout << "Magnet low: " << statusReg.magnet_low << endl;
    cout << "Magnet detected: " << statusReg.magnet_detected << endl;

    b = i2c_smbus_read_byte_data(m_i2cHandle, 0x1A);
    if (b < 0)
    {
        cerr << "ReadConfig: read agc failed. status: " << b << endl;
        return;
    }

    cout << "Automatic Gain Control: " << b << endl << endl;
}

float MagnetSensor::ReadAngle()
{
    if (ioctl(m_i2cHandle, I2C_SLAVE, c_sensorAddress) < 0)
    {
        cerr << "ReadAngle: ioctl failed" << endl;
        return NAN;
    }

    int raw_angle = i2c_smbus_read_word_data(m_i2cHandle, 0x0C);
    if (raw_angle < 0)
    {
        cerr << "ReadAngle: read raw_angle failed. status: " << raw_angle << endl;
        return NAN;
    }

    raw_angle = SwapBytesWord(raw_angle);
    cout << "Raw angle: " << raw_angle << endl;

    int angle = i2c_smbus_read_word_data(m_i2cHandle, 0x0E);
    if (angle < 0)
    {
        cerr << "ReadAngle: read angle failed. status: " << angle << endl;
        return NAN;
    }

    angle = SwapBytesWord(angle);
    cout << "Angle: " << angle << endl << endl;

    float res = 0.f;
    return res;
}
