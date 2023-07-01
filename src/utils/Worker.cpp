#include <Worker.h>

using namespace std;

Worker::Worker() : m_thread(LoopFunc)
{
}

Worker::~Worker()
{
    m_quit = true;
    m_cv.notify_one();
    m_thread.join();
}

void Worker::AddTask(std::function<void()>&& task)
{
    {
        unique_lock lk(m_mutex);
        m_tasks.emplace(move(task));
    }
    m_cv.notify_one();
}

void Worker::LoopFunc()
{
    while (true)
    {
        unique_lock lk(m_mutex);
        if (m_tasks.empty()) m_cv.wait(lk);
        if (m_quit) break;
        if (m_tasks.empty()) continue;
        auto curTask = move(m_tasks.front());
        m_tasks.pop();
        lk.unlock();

        curTask(); // execute
    }
}
