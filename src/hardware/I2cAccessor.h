#pragma once

#include <atomic>
#include <condition_variable>
#include <functional>
#include <list>
#include <mutex>
#include <queue>
#include <optional>
#include <thread>

class I2cTransaction;
using TimePoint = std::chrono::time_point<std::chrono::steady_clock>;

enum class I2cStatus : int
{
    None,
    Completed,
    Next,
    Repeat,
    Failed,
};

/// @brief 
/// @param int: handle to I2C file.
/// @param std::chrono::duration&: delay next command for duration.
using I2cCommand = std::function<I2cStatus(int, std::chrono::duration&)>;
using TransactionCompletion = std::function<void(I2cStatus)>;

class I2cTransaction
{
    friend class I2cAccessor;

    I2cTransaction(int i2cHandle, int deviceAddress) :
        m_i2cHandle(i2cHandle), m_deviceAddress(deviceAddress)
    {}
    I2cTransaction(const I2cTransaction&) = delete;

public:
    bool AddCommand(I2cCommand&& command)
    {
        if (m_commandsCount < std::size(m_commands))
        {
            m_commands[m_commandsCount].emplace(std::move(command));
            ++m_commandsCount;
            return true;
        }
        return false;
    }

    void SetCompletionCallback(TransactionCompletion&& callback)
    {
        m_completionCallback = std::move(callback);
    }

    void MakeRecursive(std::function<bool()>&& isRecursionCompleted, std::chrono::duration delayNextIteration)
    {
        m_optIsRecursionCompleted.emplace(std::move(isRecursionCompleted));
        m_delayNextIteration = delayNextIteration;
    }

    bool IsValid() const { return m_i2cHandle >= 0; }
    bool IsCompleted() const { return m_curCommand == m_commandsCount; }

private:
    TimePoint RunCommand();
    void Complete(I2cStatus status);

    const int m_i2cHandle = -1;
    const int m_deviceAddress = 0;
    std::optional<I2cCommand> m_commands[3];
    int m_commandsCount = 0;
    int m_curCommand = 0;
    TransactionCompletion m_completionCallback;
    std::optional<std::function<bool()>> m_optIsRecursionCompleted;
    std::chrono::duration m_delayNextIteration = {};
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

    void PushTransaction(I2cTransaction&& transaction);

private:
    void LoopFunc();

    int m_i2cHandle = -1;

    struct Task
    {
        TimePoint time;
        std::list<I2cTransaction>::iterator itTransaction;

        static bool Compare(const Task& t1, const Task& t2)
        {
            return t1.time > t2.time;
        }
    };

    std::priority_queue<Task, std::vector<Task>, decltype(Task::Compare)> m_tasks;
    std::list<I2cTransaction> m_transactions;

    std::atomic_bool m_quit = false;
    std::mutex m_mutex;
    std::condition_variable m_cv;
    std::optional<std::thread> m_thread;
};