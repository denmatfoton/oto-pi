#pragma once

#include "CommonDefs.h"

#include <atomic>
#include <condition_variable>
#include <functional>
#include <future>
#include <list>
#include <mutex>
#include <optional>
#include <queue>
#include <thread>

/// @brief 
/// @param int: handle to I2C file.
/// @param std::chrono::milliseconds&: delay next command for duration.
using I2cCommand = std::function<HwResult(int, std::chrono::milliseconds&)>;

class I2cTransaction
{
    friend class I2cAccessor;

    I2cTransaction(int i2cHandle, int deviceAddress) :
        m_i2cHandle(i2cHandle), m_deviceAddress(deviceAddress)
    {}
    I2cTransaction(const I2cTransaction&) = delete;

public:
    I2cTransaction(I2cTransaction&& other) : m_i2cHandle(other.m_i2cHandle),
        m_deviceAddress(other.m_deviceAddress),
        m_commandsCount(other.m_commandsCount),
        m_curCommand(other.m_curCommand),
        m_completionPromise(std::move(other.m_completionPromise)),
        m_isAborted(other.m_isAborted),
        m_optCompletionAction(std::move(other.m_optCompletionAction)),
        m_optIsRecursionCompleted(std::move(other.m_optIsRecursionCompleted)),
        m_delayNextIteration(other.m_delayNextIteration)
    {
        for (int i = 0; i < m_commandsCount; ++i)
        {
            m_commands[i] = std::move(other.m_commands[i]);
        }
    }

    bool AddCommand(I2cCommand&& command)
    {
        if (m_commandsCount < static_cast<int>(std::size(m_commands)))
        {
            m_commands[m_commandsCount].emplace(std::move(command));
            ++m_commandsCount;
            return true;
        }
        return false;
    }

    std::future<HwResult> GetFuture() { return m_completionPromise.get_future(); }

    void MakeRecursive(std::function<HwResult()>&& isRecursionCompleted, std::chrono::milliseconds delayNextIteration)
    {
        m_optIsRecursionCompleted.emplace(std::move(isRecursionCompleted));
        m_delayNextIteration = delayNextIteration;
    }

    void SetCompletionAction(std::function<void(HwResult)>&& completionAction)
    {
        m_optCompletionAction.emplace(std::move(completionAction));
    }

    bool IsValid() const { return m_i2cHandle >= 0; }
    bool IsCompleted() const { return m_curCommand == m_commandsCount; }

    void Abort()
    {
        m_isAborted = true;
    }

private:
    TimePoint RunCommand();
    void Complete(HwResult status);

    const int m_i2cHandle = -1;
    const int m_deviceAddress = 0;
    std::optional<I2cCommand> m_commands[3];
    int m_commandsCount = 0;
    int m_curCommand = 0;
    std::promise<HwResult> m_completionPromise;
    bool m_isAborted = false;

    std::optional<std::function<void(HwResult)>> m_optCompletionAction;

    // For recursive transaction
    std::optional<std::function<HwResult()>> m_optIsRecursionCompleted;
    std::chrono::milliseconds m_delayNextIteration = {};
};

class I2cAccessor
{
public:
    I2cAccessor() {}
    ~I2cAccessor();

    int Init(const char* i2cFileName);

    I2cTransaction CreateTransaction(int deviceAddress) const
    {
        return I2cTransaction(m_i2cHandle, deviceAddress);
    }

    I2cTransaction* PushTransaction(I2cTransaction&& transaction);

    int GetI2cHandle() { return m_i2cHandle; }

private:
    void LoopFunc();

    int m_i2cHandle = -1;
    using TransactionIterator = std::list<I2cTransaction>::iterator;

    struct Task
    {
        Task(TimePoint start, TransactionIterator it) :
            startTime(start), itTransaction(it) {}

        TimePoint startTime;
        TransactionIterator itTransaction;
    };

    struct CompareTasks {
        bool operator()(const Task& t1, const Task& t2) {
            return t1.startTime > t2.startTime;
        }
    };

    std::priority_queue<Task, std::vector<Task>, CompareTasks> m_tasks{CompareTasks()};
    std::list<I2cTransaction> m_transactions;

    bool m_quit = false;
    std::mutex m_mutex;
    std::condition_variable m_cv;
    std::optional<std::thread> m_thread;
};
