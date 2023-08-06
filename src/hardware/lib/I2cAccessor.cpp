#include "I2cAccessor.h"

#include <errno.h>
#include <fcntl.h>
#include <linux/i2c-dev.h>
#include <sys/ioctl.h>
#include <unistd.h>

#include <iostream>

using namespace std;
static constexpr TimePoint c_errorTime = chrono::steady_clock::time_point::min();

I2cAccessor::~I2cAccessor()
{
    if (m_thread.has_value())
    {
        cout << "Quit I2cAccessor thread" << endl;
        {
            unique_lock lk(m_mutex);
            m_quit = true;
        }
        m_cv.notify_one();
        m_thread->join();
    }

    if (m_i2cHandle >= 0)
    {
        close(m_i2cHandle);
    }
}

int I2cAccessor::Init(const char* i2cFileName)
{
    errno = 0;
    m_i2cHandle = open(i2cFileName, O_RDWR);
    if (m_i2cHandle < 0)
    {
        cout << "Open i2c file failed with: " << hex << errno << dec << endl;
        return -1;
    }

    m_thread.emplace([this]{ LoopFunc(); });

    return 0;
}

void I2cAccessor::PushTransaction(I2cTransaction&& transaction)
{
    {
        unique_lock lk(m_mutex);
        for (auto it : m_transactionsMap) // Allow single transaction per device
        {
            // Abort all old transactions
            if (it.second.m_deviceAddress == transaction.m_deviceAddress)
            {
                it.second.m_isAborted = true;
            }
        }
        m_transactionsMap.emplace(m_nextTransactionId, move(transaction));
        auto curTime = chrono::steady_clock::now();
        m_tasksQueue.emplace(curTime, m_nextTransactionId);
        ++m_nextTransactionId;
    }
    m_cv.notify_one();
}

void I2cAccessor::LoopFunc()
{
    while (true)
    {
        unique_lock lk(m_mutex);
        if (m_quit) break;

        if (m_tasksQueue.empty())
        {
            m_cv.wait(lk);
        }
        else
        {
            auto curTime = chrono::steady_clock::now();
            if (curTime < m_tasksQueue.top().startTime)
            {
                m_cv.wait_until(lk, m_tasksQueue.top().startTime);
            }
        }
        
        if (m_quit) break;
        if (m_tasksQueue.empty()) continue;
        auto curTime = chrono::steady_clock::now();
        if (curTime < m_tasksQueue.top().startTime) continue;

        auto curTask = move(m_tasksQueue.top());
        m_tasksQueue.pop();
        
        lk.unlock();

        I2cTransaction& curTransaction = m_transactionsMap[curTask.transactionId];
        TimePoint nextTime = curTransaction.RunCommand();

        lk.lock();

        if (curTransaction.IsCompleted())
        {
            m_transactionsMap.erase(curTask.transactionId);
        }
        else
        {
            m_tasksQueue.emplace(nextTime, curTask.transactionId);
        }
    }
}

TimePoint I2cTransaction::RunCommand()
{
    if (m_isAborted)
    {
        Complete(I2cStatus::Abort);
        return c_errorTime;
    }

    if (IsCompleted() || !IsValid())
    {
        return c_errorTime;
    }

    if (ioctl(m_i2cHandle, I2C_SLAVE, m_deviceAddress) < 0)
    {
        Complete(I2cStatus::CommFailure);
        return c_errorTime;
    }

    std::chrono::milliseconds delayNextCommand = 0ms;
    I2cStatus status = (*m_commands[m_curCommand])(m_i2cHandle, delayNextCommand);
    
    switch (status)
    {
    case I2cStatus::Next:
        if (++m_curCommand < m_commandsCount)
        {
            break;
        }
        [[fallthrough]];
    case I2cStatus::Completed:
        if (!m_optIsRecursionCompleted.has_value())
        {
            Complete(I2cStatus::Success);
            break;
        }
        status = (*m_optIsRecursionCompleted)();
        if (status == I2cStatus::Repeat)
        {
            m_curCommand = 0;
            delayNextCommand = m_delayNextIteration;
        }
        else
        {
            Complete(status);
        }
        break;
    case I2cStatus::Repeat:
        break;
    case I2cStatus::CommFailure:
    default:
        Complete(status);
        return c_errorTime;
    }

    auto curTime = chrono::steady_clock::now();
    return curTime + delayNextCommand;
}

void I2cTransaction::Complete(I2cStatus status)
{
    m_curCommand = m_commandsCount;
    
    if (m_optCompletionAction.has_value())
    {
        (*m_optCompletionAction)(status);
    }

    m_completionPromise.set_value(status);
}
