#include <ConfigManager.h>
#include <Logger.h>
#include <MqttWrapper.h>
#include <NozzleControl.h>
#include <Sprinkler.h>

#include <cstdio>
#include <cstring>
#include <iostream>

#include <unistd.h>


using namespace std;


constexpr const char* c_defaultLogFileName = "oto-pi.log";


class OtoPiApp
{
public:
    int Init(const char* configFileName)
    {
        if (m_configManager.LoadFromFile(configFileName) != 0 ||
            !m_configManager.IsValidConfig())
        {
            return -1;
        }

        const char* logFileName = m_configManager.GetLogFileName().c_str();
        if (strlen(logFileName) == 0)
        {
            logFileName = c_defaultLogFileName;
        }
        if (!m_logger.Open(logFileName))
        {
            return -1;
        }
        m_logger.SetLogLevel(LogLevel::Info);
        m_pLogger = &m_logger;
        
        IfFailRet(m_sprinkler.Init());
        m_sprinkler.SetLogger(m_pLogger);

        m_mqttClient.SetLogger(m_pLogger);
        IfFailRet(SetupMqtt());

        return 0;
    }

    int SetupMqtt()
    {
        if (m_mqttClient.Init(configManager.GetMqttServerAddress(), configManager.GetMqttClientId()) != MQTTCLIENT_SUCCESS)
        {
            LogError("Failed to initialize MQTT client");
            return -1;
        }
    
        LogInfo("Created MQTT client with ID: %s", configManager.GetMqttClientId().c_str());
        
        int reconnectTimeoutS = configManager.GetReconnectTimeout().value_or(30);
        if (m_mqttClient.ConnectRetry(configManager.GetMqttUser(), configManager.GetMqttPassword(),
            reconnectTimeoutS) != MQTTCLIENT_SUCCESS)
        {
            LogError("Failed to connect to MQTT server");
            return -1;
        }
    
        if (m_mqttClient.Subscribe(configManager.GetCommandTopic()) != MQTTCLIENT_SUCCESS)
        {
            LogError("Failed to subscribe to command topic");
            return -1;
        }
    
        m_mqttClient.SetMessageHandler([this] (string_view topic, string_view message) { ProcessMessage(topic, message); });
    
        LogInfo("Connected to %s", configManager.GetMqttServerAddress().c_str());
        m_mqttClient.PublishAsync(configManager.GetStatusTopic(), "on");
    
        return 0;
    }

    void ProcessMessage(string_view topic, string_view message)
    {
        if (topic == configManager.GetCommandTopic())
        {
            istringstream iss(message.data());
            string command;
            iss >> command;

            if (command == "apply")
            {
                string zoneName;
                iss >> zoneName;
                if (zoneName.empty())
                {
                    LogWarning("Zone name is missing");
                    return;
                }

                string densityStr;
                iss >> densityStr;
                if (densityStr.empty())
                {
                    LogWarning("Density is missing, using default value of 1.0");
                }

                float density = densityStr.empty() ? 1. : stof(densityStr);
                Zone zone;
                if (zone.LoadFromFile(zoneName.c_str()) != 0)
                {
                    LogWarning("Failed to load zone from file: %s", zoneName.c_str());
                    return;
                }

                m_sprinkler.StartWateringAsync(move(zone), density, [this](HwResult result) {
                    if (result == HwResult::Success)
                    {
                        LogInfo("Watering completed successfully");
                        m_mqttClient.PublishAsync(configManager.GetStatusTopic(), "wateringDone");
                    }
                    else
                    {
                        LogWarning("Watering failed with result: %d", static_cast<int>(result));
                        m_mqttClient.PublishAsync(configManager.GetStatusTopic(), "wateringFailed");
                    }
                });
            }
            else if (command == "stop")
            {
                m_sprinkler.StopWatering();
            }
            else
            {
                LogWarning("Unknown command: %s", message.data());
            }
        }
        else
        {
            LogWarning("Unknown topic: %s", topic.data());
        }
    }

private:
    Irrigation::Sprinkler m_sprinkler;
    ConfigManager m_configManager;
    MqttWrapper::Client m_mqttClient;
    Logger m_logger;
    Logger* m_pLogger = nullptr;
}

int main(int argc, char *argv[])
{
    char *configFileName = nullptr;

    // Process command line arguments
    for (int c = 0; c != -1; c = getopt(argc, argv, "c:")) {
        switch (c) {
        case 'c':
            configFileName = optarg;
            break;
        }
    }

    if (configFileName == nullptr)
    {
        cerr << "No config file was provided" << endl;
        return -1;
    }

    OtoPiApp app;
    IfFailRet(app.Init(configFileName));

    {
        // Idle
        int ch = 0;
        do
        {
            ch = getchar();
        }
        while (ch != 'Q' && ch != 'q');
    }

    return 0;
}
