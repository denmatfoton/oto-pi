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
	Logger() {}
	Logger(const char* filename);
	
	void Open(const char* filename);
	void Write(LogLevel level, const char* message, const char* functionName = nullptr, int line = -1);
	void SetLogLevel(LogLevel level)
	{
		m_level = level;
	}

private:
	LogLevel m_level = LogLevel::Info;
	std::ofstream m_output;
	std::mutex m_mutex;
};

#define Log(level, message) \
    if (m_pLogger != nullptr) \
    { \
        m_pLogger->Write(level, message, __func__, __LINE__); \
    }

#define LogInfo(message) Log(LogLevel::Info, message)
#define LogWarning(message) Log(LogLevel::Warning, message)
#define LogError(message) Log(LogLevel::Error, message)
