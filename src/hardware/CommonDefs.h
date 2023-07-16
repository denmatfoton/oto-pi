#pragma once

#include <chrono>

using TimePoint = std::chrono::time_point<std::chrono::steady_clock>;

enum class I2cStatus : int
{
    // For I2cAccessor
    Success,
    Completed,
    Next,
    Repeat,
    CommFailure,

    // Custom status
    UnexpectedValue,
    MaxValueReached,
    Timeout,
};
