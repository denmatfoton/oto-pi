#pragma once

#include "MQTTClient.h"

#include <string>
#include <unordered_set>

namespace MqttWrapper {

class Client
{
public:
    Client() = default;
    ~Client();

    int Init(const std::string& serverAddress, const std::string& clientId);
    int Connect(const std::string& userName, const std::string& password);
    int ConnectRetry(const std::string& userName, const std::string& password, int reconnectTimeoutS);
    int Subscribe(const std::string& topic);
    int Unsubscribe(const std::string& topic);
    int PublishAsync(const std::string& topic, const char* message,
        int qos = 0, MQTTClient_deliveryToken* pToken = nullptr);

private:
    void OnMessageArrived(const char *topicName, MQTTClient_message *message);

    MQTTClient m_client = nullptr;
    std::unordered_set<std::string> m_subscribedTopics;
};

} // namespace MqttWrapper
