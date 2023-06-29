#include "PressureSensor.h"

extern "C" {
#include <i2c/smbus.h>
}
#include <linux/i2c-dev.h>
#include <math.h>
#include <sys/ioctl.h>
#include <unistd.h>

#include <chrono>
#include <iostream>
#include <thread>

using namespace std;

int PressureSensor::ReadRawPressue()
{
    if (ioctl(m_i2cHandle, I2C_SLAVE, c_sensorAddress) < 0)
    {
        cerr << "ReadRawPressue: ioctl failed" << endl;
        return 0;
    }

    static constexpr char requestMeasurementCmd[] = {0xAA, 0, 0};
    int bytesWritten = write(m_i2cHandle, requestMeasurementCmd, sizeof(requestMeasurementCmd));
    if (bytesWritten != sizeof(requestMeasurementCmd))
    {
        cerr << "ReadRawPressue: request measurement failed. bytesWritten: " << bytesWritten << endl;
        return 0;
    }

    while (true)
    {
        this_thread::sleep_for(std::chrono::milliseconds(1));
        int status = i2c_smbus_read_byte(m_i2cHandle);
        if (status < 0)
        {
            cerr << "ReadRawPressue: read status failed. status: " << status << endl;
            return 0;
        }
        if ((status & c_busyFlag) == 0 || (status == 0xFF))
        {
            break;
        }
    }

    char readBuff[4];
    int bytesRead = read(m_i2cHandle, readBuff, sizeof(readBuff));
    if (bytesRead != sizeof(readBuff))
    {
        cerr << "ReadRawPressue: read measurement failed. bytesRead: " << bytesRead << endl;
        return 0;
    }

    int status = readBuff[0];
    if ((status & c_itegrityFlag) || (status & c_mathSatFlag))
    {
        cerr << "ReadRawPressue: read measurement failed. status: " << std::hex << status << std::dec << endl;
        return 0;
    }

    int reading = 0;
    for (int i = 0; ++i < 4;)
    {
        reading <<= 8;
        reading |= readBuff[i];
    }

    return reading;
}

float PressureSensor::ReadPressurePsi()
{
    int reading = ReadRawPressue();
    return static_cast<float>((reading - c_outputMin) * (c_maxPsi - c_minPsi)) 
        / static_cast<float>(c_outputMax - c_outputMin) + static_cast<float>(c_minPsi);
}
