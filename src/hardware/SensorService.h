#pragma once

#include <functional>

struct ReadingRequest
{
    std::function<int()> m_readValue;
    std::function<>
};

class SensorService
{
public:
    void NotifyWhen(std::function<bool()> checker, int pullPeriodMs, std::function<void()> callback);
};
