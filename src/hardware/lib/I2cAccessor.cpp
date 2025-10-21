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

I2cTransaction* I2cAccessor::PushTransaction(I2cTransaction&& newTransaction)
{
    I2cTransaction* pushedTransaction = nullptr;
    {
        unique_lock lk(m_mutex);
        for (auto& transaction : m_transactions)
        {
            // Abort all old transactions with same device address
            if (transaction.m_deviceAddress == newTransaction.m_deviceAddress)
            {
                transaction.m_isAborted = true;
            }
        }
        pushedTransaction = &m_transactions.emplace_front(move(newTransaction));
        auto curTime = chrono::steady_clock::now();
        m_tasks.emplace(curTime, m_transactions.begin());
    }
    m_cv.notify_one();

    return pushedTransaction;
}

void I2cAccessor::LoopFunc()
{
    while (true)
    {
        unique_lock lk(m_mutex);
        if (m_quit) break;

        if (m_tasks.empty())
        {
            m_cv.wait(lk);
        }
        else
        {
            auto curTime = chrono::steady_clock::now();
            if (curTime < m_tasks.top().startTime)
            {
                m_cv.wait_until(lk, m_tasks.top().startTime);
            }
        }
        
        if (m_quit) break;
        if (m_tasks.empty()) continue;
        auto curTime = chrono::steady_clock::now();
        if (curTime < m_tasks.top().startTime) continue;

        auto curTask = m_tasks.top();
        m_tasks.pop();
        
        lk.unlock();

        TimePoint nextTime = curTask.itTransaction->RunCommand();

        lk.lock();

        if (curTask.itTransaction->IsCompleted())
        {
            m_transactions.erase(curTask.itTransaction);
        }
        else
        {
            m_tasks.emplace(nextTime, curTask.itTransaction);
        }
    }
}

TimePoint I2cTransaction::RunCommand()
{
    if (m_isAborted)
    {
        Complete(HwResult::Abort);
        return c_errorTime;
    }

    if (IsCompleted() || !IsValid())
    {
        return c_errorTime;
    }

    if (ioctl(m_i2cHandle, I2C_SLAVE, m_deviceAddress) < 0)
    {
        Complete(HwResult::CommFailure);
        return c_errorTime;
    }

    std::chrono::milliseconds delayNextCommand = 0ms;
    HwResult status = (*m_commands[m_curCommand])(m_i2cHandle, delayNextCommand);
    
    switch (status)
    {
    case HwResult::Next:
        if (++m_curCommand < m_commandsCount)
        {
            break;
        }
        [[fallthrough]];
    case HwResult::Completed:
        if (!m_optIsRecursionCompleted.has_value())
        {
            Complete(HwResult::Success);
            break;
        }
        status = (*m_optIsRecursionCompleted)();
        if (status == HwResult::Repeat)
        {
            m_curCommand = 0;
            delayNextCommand = m_delayNextIteration;
        }
        else
        {
            Complete(status);
        }
        break;
    case HwResult::Repeat:
        break;
    case HwResult::CommFailure:
    default:
        Complete(status);
        return c_errorTime;
    }

    auto curTime = chrono::steady_clock::now();
    return curTime + delayNextCommand;
}

void I2cTransaction::Complete(HwResult status)
{
    m_curCommand = m_commandsCount;
    
    if (m_optCompletionAction.has_value())
    {
        (*m_optCompletionAction)(status);
    }

    m_completionPromise.set_value(status);
}
