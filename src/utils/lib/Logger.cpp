#include "Logger.h"

#include <chrono>
#include <iomanip>

using namespace std;

Logger::Logger(const char* filename)
{
	Open(filename);
}

void Logger::Open(const char* filename)
{
	m_output.open(filename, ios::out | ios::app);
}

const char* strLogLevel[static_cast<int>(Max)] = {"INFO", "WARNINIG", "ERROR"};

void Logger::Write(LogLevel level, const char* message, const char* functionName, int line)
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

			if (functionName != nullptr)
			{
				m_output << functionName << ", " << line << ": ";
			}
			
			m_output << message << endl;
		}
	}
}
