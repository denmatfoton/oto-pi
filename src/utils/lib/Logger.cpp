#include "Logger.h"

#include <chrono>
#include <cstdarg>
#include <cstring>
#include <iomanip>
#include <iostream>

using namespace std;

Logger::Logger(const char* filename)
{
	Open(filename);
}

bool Logger::Open(const char* filename)
{
	m_output.open(filename, ios::out | ios::app);
	if (m_output.is_open())
	{
		cout << "Opened log file: " << filename << endl;
		Write(LogLevel::Info, "Logger started");
		return true;
	}
	
	cerr << "Failed to open log file: " << filename << endl;
	return false;
}

const char* strLogLevel[static_cast<int>(Max)] = {"INFO", "WARNINIG", "ERROR"};

void Logger::Write(LogLevel level, const char* message,
	const char* functionName, const char* fileName, int line)
{
	if (m_output.is_open() && level >= m_level)
	{
		auto curTime = chrono::system_clock::now();
		time_t t = chrono::system_clock::to_time_t(curTime);
		std::tm tm;
		localtime_r(&t, &tm);

		auto since_epoch = curTime.time_since_epoch();
		auto millis = chrono::duration_cast<chrono::milliseconds>(since_epoch);
		int ms = static_cast<int>(millis.count() % 1000);

		{
			unique_lock lk(m_mutex);
			
			m_output << std::put_time(&tm, "%D %T.") << ms;
			m_output << " [" << strLogLevel[static_cast<int>(level)] << "] ";

			if (functionName != nullptr && fileName != nullptr)
			{
				const char* strippedFileName = strrchr(fileName, '/') + 1;
				m_output << std::left << "| "
					<< setw(20) << strippedFileName << "| "
					<< setw(4)  << line << "| "
					<< setw(20) << functionName << "| ";
			}
			
			m_output << message << endl;
		}
	}
}

void Logger::FormattedWrite(LogLevel level, const char* functionName,
	const char* fileName, int line, const char* format, ...)
{
	char message[256];
	std::va_list args;
	va_start(args, format);
	vsnprintf(message, sizeof(message), format, args);
	va_end(args);
	Write(level, message, functionName, fileName, line);
}
