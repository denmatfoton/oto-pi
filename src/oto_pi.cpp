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


int SetupMqtt(MqttWrapper::Client& mqttClient, const ConfigManager& configManager)
{
    if (mqttClient.Init(configManager.GetMqttServerAddress(), configManager.GetMqttClientId()) != MQTTCLIENT_SUCCESS)
    {
        return -1;
    }

    cout << "Created MQTT client with ID: " << configManager.GetMqttClientId() << endl;
    
    int reconnectTimeoutS = configManager.GetReconnectTimeout().value_or(30);
    if (mqttClient.ConnectRetry(configManager.GetMqttUser(), configManager.GetMqttPassword(),
        reconnectTimeoutS) != MQTTCLIENT_SUCCESS)
    {
        return -1;
    }

    if (mqttClient.Subscribe(configManager.GetCommandTopic()) != MQTTCLIENT_SUCCESS)
    {
        return -1;
    }

    cout << endl << "Connected to " << configManager.GetMqttServerAddress() << endl;
    mqttClient.PublishAsync(configManager.GetStatusTopic(), "on");

    return 0;
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

    // Read config file
    ConfigManager configManager;
    if (configManager.LoadFromFile(configFileName) != 0 ||
        !configManager.IsValidConfig())
    {
        return -1;
    }

    const char* logFileName = configManager.GetLogFileName().c_str();
    if (strlen(logFileName) == 0)
    {
        logFileName = c_defaultLogFileName;
    }
    Logger logger(logFileName);

    {
        MqttWrapper::Client mqttClient;
        mqttClient.SetLogger(&logger);
        if (int rc = SetupMqtt(mqttClient, configManager); rc != 0)
        {
            return rc;
        }

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
