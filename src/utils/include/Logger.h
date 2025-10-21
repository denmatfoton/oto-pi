#pragma once

#include <fstream>
#include <mutex>

enum LogLevel : int
{
    Info,
    Warning,
    Error,

    Max,
};

class Logger
{
public:
    Logger()
    {
    }
    Logger(const char* filename);

    bool Open(const char* filename);
    void Write(LogLevel level, const char* message,
        const char* functionName = nullptr,
        const char* fileName = nullptr,
        int lineNumber = -1);
    void FormattedWrite(LogLevel level, const char* functionName,
        const char* fileName, int lineNumber, const char* format, ...);
    void SetLogLevel(LogLevel level)
    {
        m_level = level;
    }

private:
    LogLevel m_level = LogLevel::Info;
    std::ofstream m_output;
    std::mutex m_mutex;
};

#define Log(level, ...) \
    if (m_pLogger != nullptr) \
    { \
        m_pLogger->FormattedWrite(level, __func__, __FILE__, __LINE__, __VA_ARGS__); \
    }

#define LogInfo(...)    Log(LogLevel::Info, __VA_ARGS__)
#define LogWarning(...) Log(LogLevel::Warning, __VA_ARGS__)
#define LogError(...)   Log(LogLevel::Error, __VA_ARGS__)
