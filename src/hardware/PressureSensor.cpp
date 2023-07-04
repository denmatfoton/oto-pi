#include "PressureSensor.h"
#include "I2cAccessor.h"

extern "C" {
#include <i2c/smbus.h>
}
#include <linux/i2c-dev.h>
#include <sys/ioctl.h>
#include <unistd.h>

#include <iostream>
#include <memory>

using namespace std;

#if 0
int PressureSensor::ReadRawPressure()
{
    static constexpr char requestMeasurementCmd[] = {0xAA, 0, 0};
    int bytesWritten = write(m_i2cHandle, requestMeasurementCmd, sizeof(requestMeasurementCmd));
    if (bytesWritten != sizeof(requestMeasurementCmd))
    {
        cerr << "ReadRawPressure: request measurement failed. bytesWritten: " << bytesWritten << endl;
        return 0;
    }

    while (true)
    {
        this_thread::sleep_for(std::chrono::milliseconds(1));
        int status = i2c_smbus_read_byte(m_i2cHandle);
        if (status < 0)
        {
            cerr << "ReadRawPressure: read status failed. status: " << status << endl;
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
        cerr << "ReadRawPressure: read measurement failed. bytesRead: " << bytesRead << endl;
        return 0;
    }

    int status = readBuff[0];
    if ((status & c_integrityFlag) || (status & c_mathSatFlag))
    {
        cerr << "ReadRawPressure: read measurement failed. status: " << std::hex << status << std::dec << endl;
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
    int reading = ReadRawPressure();
    return static_cast<float>((reading - c_outputMin) * (c_maxPsi - c_minPsi)) 
        / static_cast<float>(c_outputMax - c_outputMin) + static_cast<float>(c_minPsi);
}
#endif

void PressureSensor::FillI2cTransaction(I2cTransaction& transaction)
{
    transaction.AddCommand([] (int i2cHandle, std::chrono::milliseconds& delayNextCommand) {
        static constexpr char requestMeasurementCmd[] = {0xAA, 0, 0};
        int bytesWritten = write(i2cHandle, requestMeasurementCmd, sizeof(requestMeasurementCmd));
        if (bytesWritten != sizeof(requestMeasurementCmd))
        {
            cerr << "ReadRawPressure: request measurement failed. bytesWritten: " << bytesWritten << endl;
            return I2cStatus::Failed;
        }
        delayNextCommand = 3ms;
        return I2cStatus::Next;
    });

    transaction.AddCommand([] (int i2cHandle, std::chrono::milliseconds& delayNextCommand) {
        int status = i2c_smbus_read_byte(i2cHandle);
        if (status < 0)
        {
            cerr << "ReadRawPressure: read status failed. status: " << status << endl;
            return I2cStatus::Failed;
        }
        
        if ((status & c_busyFlag) == 0 || (status == 0xFF))
        {
            return I2cStatus::Next;
        }
        else
        {
            delayNextCommand = 1ms;
            return I2cStatus::Repeat;
        }
    });

    transaction.AddCommand([this] (int i2cHandle, std::chrono::milliseconds& /*delayNextCommand*/) {
        char readBuff[4];
        int bytesRead = read(i2cHandle, readBuff, sizeof(readBuff));
        if (bytesRead != sizeof(readBuff))
        {
            cerr << "ReadRawPressure: read measurement failed. bytesRead: " << bytesRead << endl;
            return I2cStatus::Failed;
        }

        int status = readBuff[0];
        if ((status & c_integrityFlag) || (status & c_mathSatFlag))
        {
            cerr << "ReadRawPressure: read measurement failed. status: " << std::hex << status << std::dec << endl;
            return I2cStatus::Failed;
        }

        int reading = 0;
        for (int i = 0; ++i < 4;)
        {
            reading <<= 8;
            reading |= readBuff[i];
        }

        m_lastRawValue.store(reading);

        return I2cStatus::Completed;
    });
}

std::future<int> PressureSensor::ReadRawPressureAsync()
{
    shared_ptr<promise<int>> spPromise = make_shared<promise<int>>;
    I2cTransaction transaction = m_i2cAccessor.CreateTransaction(c_sensorAddress);
    FillI2cTransaction(transaction);

    future<int> measurementFuture = spPromise->get_future();
    
    transaction.SetCompletionCallback(move([this, spPromise = move(spPromise)] (I2cStatus status) mutable {
        int value = status == I2cStatus::Completed ? m_lastRawValue.load() : -1;
        spPromise->set_value(value);
    }));

    m_i2cAccessor.PushTransaction(move(transaction));

    return measurementFuture;
}
