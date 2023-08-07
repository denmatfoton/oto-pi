#include "PressureSensor.h"
#include "I2cAccessor.h"
#include "Utils.h"

extern "C" {
#include <i2c/smbus.h>
}
#include <linux/i2c-dev.h>
#include <sys/ioctl.h>
#include <unistd.h>

#include <iostream>

using namespace std;

void PressureSensor::FillI2cTransaction(I2cTransaction& transaction)
{
    transaction.AddCommand([] (int i2cHandle, std::chrono::milliseconds& delayNextCommand) {
        static constexpr char requestMeasurementCmd[] = {0xAA, 0, 0};
        int bytesWritten = write(i2cHandle, requestMeasurementCmd, sizeof(requestMeasurementCmd));
        if (bytesWritten != sizeof(requestMeasurementCmd))
        {
            cerr << "ReadRawPressure: request measurement failed. bytesWritten: " << bytesWritten << endl;
            return I2cStatus::CommFailure;
        }
        delayNextCommand = 3ms;
        return I2cStatus::Next;
    });

    transaction.AddCommand([] (int i2cHandle, std::chrono::milliseconds& delayNextCommand) {
        int status = i2c_smbus_read_byte(i2cHandle);
        if (status < 0)
        {
            cerr << "ReadRawPressure: read status failed. status: " << status << endl;
            return I2cStatus::CommFailure;
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
            return I2cStatus::CommFailure;
        }

        int status = readBuff[0];
        if ((status & c_integrityFlag) || (status & c_mathSatFlag))
        {
            cerr << "ReadRawPressure: read measurement failed. status: " << std::hex << status << std::dec << endl;
            return I2cStatus::CommFailure;
        }

        int reading = 0;
        for (int i = 0; ++i < 4;)
        {
            reading <<= 8;
            reading |= readBuff[i];
        }

        m_lastRawValue.store(reading - c_outputMin);
        m_minRawValue.store(min(m_minRawValue.load(), reading));
        m_lastMeasurementTimeMs.store(TimeSinceEpochMs());

        return I2cStatus::Completed;
    });
}

std::future<I2cStatus> PressureSensor::ReadPressureAsync()
{
    I2cTransaction transaction = m_i2cAccessor.CreateTransaction(c_sensorAddress);
    FillI2cTransaction(transaction);

    future<I2cStatus> measurementFuture = transaction.GetFuture();
    m_i2cAccessor.PushTransaction(move(transaction));

    return measurementFuture;
}

std::future<I2cStatus> PressureSensor::NotifyWhenPressure(std::function<I2cStatus(int)>&& isExpectedValue,
        std::function<void(I2cStatus)>&& completionAction)
{
    I2cTransaction transaction = m_i2cAccessor.CreateTransaction(c_sensorAddress);
    
    FillI2cTransaction(transaction);

    transaction.MakeRecursive([this, checkPressure = move(isExpectedValue)] {
        return checkPressure(GetLastRawPressure());
    }, 2ms);

    transaction.SetCompletionAction(move(completionAction));

    future<I2cStatus> measurementFuture = transaction.GetFuture();
    m_i2cAccessor.PushTransaction(move(transaction));

    return measurementFuture;
}

bool PressureSensor::IsMeasurementStale() const
{
    static constexpr uint32_t staleMeasurementThresholdMs = 50;
    return TimeSinceEpochMs() - GetLastMeasurementTimeMs() > staleMeasurementThresholdMs;
}

int PressureSensor::GetRawPressureFetchIfStale()
{
    if (IsMeasurementStale())
    {
        auto measurementFuture = ReadPressureAsync();
        measurementFuture.wait();
    }
    return IsMeasurementStale() ? -1 : GetLastRawPressure();
}
