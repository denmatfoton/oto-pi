#include "MagnetSensor.h"
#include "I2cAccessor.h"
#include "Utils.h"

extern "C" {
#include <i2c/smbus.h>
}
#include <linux/i2c-dev.h>
#include <math.h>
#include <sys/ioctl.h>
#include <unistd.h>

#include <iostream>

using namespace std;

// TODO: convert to async
void MagnetSensor::ReadConfig()
{
    int i2cHandle = m_i2cAccessor.GetI2cHandle();

    if (ioctl(i2cHandle, I2C_SLAVE, c_sensorAddress) < 0)
    {
        cerr << "ReadConfig: ioctl failed" << endl;
        return;
    }
    
    int zpos = i2c_smbus_read_word_data(i2cHandle, 1);
    cout << "ZPOS: " << zpos << endl;
    
    int mpos = i2c_smbus_read_word_data(i2cHandle, 3);
    cout << "MPOS: " << mpos << endl;
    
    int mang = i2c_smbus_read_word_data(i2cHandle, 5);
    cout << "MANG: " << mang << endl;

    int wConf = i2c_smbus_read_word_data(i2cHandle, 0x7);
    if (wConf < 0)
    {
        cerr << "ReadConfig: read word failed. status: " << wConf << endl;
        return;
    }

    ConfigReg confReg { wConf };
    
    cout << "WD: " << confReg.fields.wd << endl;
    cout << "FTH: " << confReg.fields.fth << endl;
    cout << "SF: " << confReg.fields.sf << endl;

    cout << "PWMF: " << confReg.fields.pwmf << endl;
    cout << "OUTS: " << confReg.fields.outs << endl;
    cout << "HYST: " << confReg.fields.hyst << endl;
    cout << "PM: " << confReg.fields.pm << endl << endl;
}

// TODO: convert to async
void MagnetSensor::ReadStatus()
{
    int i2cHandle = m_i2cAccessor.GetI2cHandle();

    if (ioctl(i2cHandle, I2C_SLAVE, c_sensorAddress) < 0)
    {
        cerr << "ReadStatus: ioctl failed" << endl;
        return;
    }

    int b = i2c_smbus_read_byte_data(i2cHandle, 0x0B);
    if (b < 0)
    {
        cerr << "ReadConfig: read status failed. status: " << b << endl;
        return;
    }

    StatusReg statusReg { static_cast<__u8>(b) };

    cout << "Magnet high: " << statusReg.fields.magnet_high << endl;
    cout << "Magnet low: " << statusReg.fields.magnet_low << endl;
    cout << "Magnet detected: " << statusReg.fields.magnet_detected << endl;

    b = i2c_smbus_read_byte_data(i2cHandle, 0x1A);
    if (b < 0)
    {
        cerr << "ReadConfig: read agc failed. status: " << b << endl;
        return;
    }

    cout << "Automatic Gain Control: " << b << endl << endl;
}

void MagnetSensor::FillI2cTransactionReadAngle(I2cTransaction& transaction)
{
    transaction.AddCommand([this] (int i2cHandle, std::chrono::milliseconds& /*delayNextCommand*/) {
        int angle = i2c_smbus_read_word_data(i2cHandle, 0x0E);
        if (angle < 0)
        {
            cerr << "ReadAngle: read angle failed. status: " << angle << endl;
            return I2cStatus::Failure;
        }

        angle = SwapBytesWord(angle);
        m_lastRawAngle.store(angle);
        
        return I2cStatus::Completed;
    });
}

std::future<I2cStatus> MagnetSensor::ReadAngleAsync()
{
    I2cTransaction transaction = m_i2cAccessor.CreateTransaction(c_sensorAddress);
    
    FillI2cTransactionReadAngle(transaction);

    future<I2cStatus> measurementFuture = transaction.GetFuture();
    m_i2cAccessor.PushTransaction(move(transaction));

    return measurementFuture;
}

std::future<I2cStatus> MagnetSensor::NotifyWhenAngle(std::function<bool(int)>&& isExpectedValue,
        std::function<void(I2cStatus)>&& completionAction)
{
    I2cTransaction transaction = m_i2cAccessor.CreateTransaction(c_sensorAddress);
    
    FillI2cTransactionReadAngle(transaction);

    transaction.MakeRecursive([this, checkAngle = move(isExpectedValue)] {
        return checkAngle(GetLastRawAngle());
    }, 2ms);

    transaction.SetCompletionAction(move(completionAction));

    future<I2cStatus> measurementFuture = transaction.GetFuture();
    m_i2cAccessor.PushTransaction(move(transaction));

    return measurementFuture;
}
