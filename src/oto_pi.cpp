#include <ConfigManager.h>
#include <Logger.h>
#include <MqttWrapper.h>
#include <NozzleControl.h>
#include <Sprinkler.h>

#include <cstdio>
#include <iostream>

#include <unistd.h>


using namespace std;

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

    // Establish MQTT connection
    MqttWrapper::Client mqttClient;
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

    // Idle
    int ch = 0;
    do
    {
        ch = getchar();
    }
    while (ch != 'Q' && ch != 'q');

    return 0;
}
