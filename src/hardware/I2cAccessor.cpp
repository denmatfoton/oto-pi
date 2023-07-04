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
        m_quit = true;
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
        m_transactions.emplace_front(move(transaction));
        auto curTime = chrono::steady_clock::now();
        m_tasks.emplace(curTime, m_transactions.begin());
    }
    m_cv.notify_one();
}

void I2cAccessor::LoopFunc()
{
    while (true)
    {
        unique_lock lk(m_mutex);

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

        auto curTask = move(m_tasks.top());
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
    if (IsCompleted() || !IsValid())
    {
        return c_errorTime;
    }

    if (ioctl(m_i2cHandle, I2C_SLAVE, m_deviceAddress) < 0)
    {
        Complete(I2cStatus::Failed);
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
        if (m_optIsRecursionCompleted.has_value() &&
            !(*m_optIsRecursionCompleted)())
        {
            m_curCommand = 0;
            delayNextCommand = m_delayNextIteration;
        }
        else
        {
            Complete(I2cStatus::Completed);
        }
        break;
    case I2cStatus::Repeat:
        break;
    case I2cStatus::Failed:
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
    m_completionCallback(status);
}
