#ragma once

#include <functional>
#include <thread>

class Worker
{
public:
    Worker();
    ~Worker();

    void AddTask(std::function<viod()> task);

private:
    void LoopFunc();

    std::thread m_thread;
};
