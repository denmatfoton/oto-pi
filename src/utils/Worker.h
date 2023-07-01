#pragma once

#include <atomic>
#include <condition_variable>
#include <functional>
#include <mutex>
#include <queue>
#include <thread>

class Worker
{
public:
    Worker();
    ~Worker();

    void AddTask(std::function<void()>&& task);

private:
    void LoopFunc();

    std::queue<std::function<void()>> m_tasks;
    std::atomic_bool m_quit = false;

    std::mutex m_mutex;
    std::condition_variable m_cv;
    std::thread m_thread;
};
