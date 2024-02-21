#pragma once

#include <optional>
#include <string>
#include <unordered_map>

class ConfigManager
{
public:
	ConfigManager() = default;

	int LoadFromFile(const char* fileName);
	bool IsValidConfig() const;

	std::optional<int> GetIntValue(const std::string& setting) const;
	const std::string& GetStringValue(const std::string& setting) const;

	const std::string& GetMqttServerAddress() const { return GetStringValue("mqtt_server"); }
	const std::string& GetMqttClientId()      const { return GetStringValue("mqtt_client_id"); }
	const std::string& GetMqttUser() 		  const { return GetStringValue("mqtt_user"); }
	const std::string& GetMqttPassword() 	  const { return GetStringValue("mqtt_password"); }
	const std::string& GetCommandTopic() 	  const { return GetStringValue("command_topic"); }
	const std::string& GetStatusTopic()  	  const { return GetStringValue("status_topic"); }
	const std::string& GetLogFileName()  	  const { return GetStringValue("log_file"); }

	std::optional<int> GetReconnectTimeout()  const { return GetIntValue("mqtt_reconnect_timeout"); }

private:
	std::unordered_map<std::string, std::string> m_settingsMap;
};
