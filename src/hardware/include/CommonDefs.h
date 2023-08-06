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
    Abort,

    // Custom status
    UnexpectedValue,
    MaxValueReached,
    Timeout,
};

constexpr const char c_i2cFileName[] = "/dev/i2c-1";

#define Statement(x)  do { x; } while(0);
#define FAILED(hr_)   ((hr_) < 0)

#define IfFailRet(hr_) Statement( \
	const auto hrLocal_ = (hr_); \
	if (FAILED(hrLocal_)) \
	{ \
		return hrLocal_; \
	})
