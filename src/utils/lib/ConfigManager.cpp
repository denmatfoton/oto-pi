#include "ConfigManager.h"

#include <charconv>
#include <cstring>
#include <fstream>
#include <iostream>
#include <sstream>

using namespace std;


int ConfigManager::LoadFromFile(const char* fileName)
{
	ifstream file(fileName, ios::in);

	if (!file.good())
	{
		cerr << "Can't open config file '" << fileName << "'" << endl;
		return -1;
	}

	char buffer[256];
	static constexpr const char* delim = " =\r\n";
	while (file.getline(buffer, sizeof(buffer)))
	{
		char* line = buffer;
		line += strspn(line, delim);
		auto settingLen = strcspn(line, delim);

		if (settingLen == 0) continue;

		line[settingLen] = '\0';
		char* setting = line;

		line += settingLen + 1;
		line += strspn(line, delim);
		char* value = line;
		auto valueLen = strcspn(line, "\r\n");
		value[valueLen] = '\0';

		if (valueLen == 0) continue;

		m_settingsMap[setting] = value;
	}

	return 0;
}

bool ConfigManager::IsValidConfig() const
{
    if (GetMqttServerAddress().empty())
    {
        cerr << "Required mqtt_server setting is missing in config file" << endl;
        return false;
    }

    if (GetMqttClientId().empty())
    {
        cerr << "Required mqtt_client_id setting is missing in config file" << endl;
        return false;
    }

    if (GetCommandTopic().empty())
    {
        cerr << "Required command_topic setting is missing in config file" << endl;
        return false;
    }

    if (GetStatusTopic().empty())
    {
        cerr << "Required status_topic setting is missing in config file" << endl;
        return false;
    }

	return true;
}

std::optional<int> ConfigManager::GetIntValue(const string& setting) const
{
	auto it = m_settingsMap.find(setting);
	if (it != m_settingsMap.end())
	{
		int value = 0;
		if (from_chars(it->second.c_str(), it->second.c_str() + it->second.size(), value).ec == errc{})
		{
			return value;
		}
	}
	return nullopt;
}

const string& ConfigManager::GetStringValue(const string& setting) const
{
	static const string empty = ""s;
	auto it = m_settingsMap.find(setting);
	return it != m_settingsMap.end() ? it->second : empty;
}
