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

template<typename T>
static std::future<T> GetCompletedFuture(T value)
{
    return std::async(std::launch::deferred, [value = std::move(value)]() {
        return value;
    });
}

void PressureSensor::FillI2cTransaction(I2cTransaction& transaction)
{
    transaction.AddCommand([] (int i2cHandle, std::chrono::milliseconds& delayNextCommand) {
        static constexpr char requestMeasurementCmd[] = { '\xAA', '\0', '\0' };
        auto bytesWritten = write(i2cHandle, requestMeasurementCmd, sizeof(requestMeasurementCmd));
        if (bytesWritten != sizeof(requestMeasurementCmd))
        {
            cerr << "ReadRawPressure: request measurement failed. bytesWritten: " << bytesWritten << endl;
            return HwResult::CommFailure;
        }
        delayNextCommand = 3ms;
        return HwResult::Next;
    });

    transaction.AddCommand([] (int i2cHandle, std::chrono::milliseconds& delayNextCommand) {
        int status = i2c_smbus_read_byte(i2cHandle);
        if (status < 0)
        {
            cerr << "ReadRawPressure: read status failed. status: " << status << endl;
            return HwResult::CommFailure;
        }
        
        if ((status & c_busyFlag) == 0 || (status == 0xFF))
        {
            return HwResult::Next;
        }
        else
        {
            delayNextCommand = 1ms;
            return HwResult::Repeat;
        }
    });

    transaction.AddCommand([this] (int i2cHandle, std::chrono::milliseconds& /*delayNextCommand*/) {
        char readBuff[4];
        auto bytesRead = read(i2cHandle, readBuff, sizeof(readBuff));
        if (bytesRead != sizeof(readBuff))
        {
            cerr << "ReadRawPressure: read measurement failed. bytesRead: " << bytesRead << endl;
            return HwResult::CommFailure;
        }

        int status = readBuff[0];
        if ((status & c_integrityFlag) || (status & c_mathSatFlag))
        {
            cerr << "ReadRawPressure: read measurement failed. status: " << std::hex << status << std::dec << endl;
            return HwResult::CommFailure;
        }

        int reading = 0;
        for (int i = 0; ++i < 4;)
        {
            reading <<= 8;
            reading |= readBuff[i];
        }

        reading -= c_outputMin;
        ProcessMeasurement(reading);

        return HwResult::Completed;
    });
}

void PressureSensor::ProcessMeasurement(int curValue)
{
    int curTimeMs = TimeSinceEpochMs();
    int prevValue = m_lastRawValue.load();
    uint32_t prevTimeMs = m_lastMeasurementTimeMs.load();

    m_lastRawValue.store(curValue);
    m_minRawValue.store(min(m_minRawValue.load(), curValue));
    m_lastMeasurementTimeMs.store(curTimeMs);

    if (prevValue != 0)
    {
        int rate = TruncateNoise(curValue - prevValue) * c_rateMultiplyer / (curTimeMs - prevTimeMs);
        m_lastChangeRate.store(rate);
    }
}

std::future<HwResult> PressureSensor::ReadPressureAsync()
{
    if (m_pCurTransaction != nullptr)
    {
        cerr << "PressureSensor::ReadPressureAsync: transaction already in progress" << endl;
        return GetCompletedFuture(HwResult::Failure);
    }
    I2cTransaction transaction = m_i2cAccessor.CreateTransaction(c_sensorAddress);
    FillI2cTransaction(transaction);

    transaction.SetCompletionAction([this] (HwResult) {
        m_pCurTransaction = nullptr;
    });

    future<HwResult> measurementFuture = transaction.GetFuture();
    m_pCurTransaction = m_i2cAccessor.PushTransaction(move(transaction));

    return measurementFuture;
}

std::future<HwResult> PressureSensor::NotifyWhenPressure(std::function<HwResult(int)>&& isExpectedValue,
        std::function<void(HwResult)>&& completionAction)
{
    if (m_pCurTransaction != nullptr)
    {
        cerr << "PressureSensor::ReadPressureAsync: transaction already in progress" << endl;
        return GetCompletedFuture(HwResult::Failure);
    }
    I2cTransaction transaction = m_i2cAccessor.CreateTransaction(c_sensorAddress);
    
    FillI2cTransaction(transaction);

    transaction.MakeRecursive([this, checkPressure = move(isExpectedValue)] {
        return checkPressure(GetLastPressure());
    }, 2ms);

    transaction.SetCompletionAction(
        [this, completionAction = move(completionAction)] (HwResult status) {
            m_pCurTransaction = nullptr;
            completionAction(status);
        }
    );

    future<HwResult> measurementFuture = transaction.GetFuture();
    m_pCurTransaction = m_i2cAccessor.PushTransaction(move(transaction));

    return measurementFuture;
}

std::future<HwResult> PressureSensor::StartContinuousMeasurement(std::function<HwResult(int)>&& onValue)
{
    if (m_pCurTransaction != nullptr)
    {
        cerr << "PressureSensor::ReadPressureAsync: transaction already in progress" << endl;
        return GetCompletedFuture(HwResult::Failure);
    }
    I2cTransaction transaction = m_i2cAccessor.CreateTransaction(c_sensorAddress);
    
    FillI2cTransaction(transaction);

    transaction.MakeRecursive([this, onValue = move(onValue)] {
        return onValue(GetLastPressure());
    }, 2ms);

    transaction.SetCompletionAction(
        [this] (HwResult /*status*/) {
            m_pCurTransaction = nullptr;
        }
    );

    future<HwResult> measurementFuture = transaction.GetFuture();
    m_pCurTransaction = m_i2cAccessor.PushTransaction(move(transaction));

    return measurementFuture;
}

void PressureSensor::AbortMeasurement()
{
    if (m_pCurTransaction != nullptr)
    {
        m_pCurTransaction->Abort();
        m_pCurTransaction = nullptr;
    }
}

bool PressureSensor::IsMeasurementStale() const
{
    static constexpr uint32_t staleMeasurementThresholdMs = 50;
    return TimeSinceEpochMs() - GetLastMeasurementTimeMs() > staleMeasurementThresholdMs;
}

int PressureSensor::GetPressureFetchIfStale()
{
    if (IsMeasurementStale())
    {
        auto measurementFuture = ReadPressureAsync();
        measurementFuture.wait();
    }
    return IsMeasurementStale() ? -1 : GetLastPressure();
}
