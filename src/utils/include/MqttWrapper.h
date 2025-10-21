#pragma once

#include "MQTTClient.h"

#include <functional>
#include <string>
#include <unordered_set>

class Logger;

namespace MqttWrapper {

class Client
{
public:
    Client() = default;
    ~Client();

    int Init(const std::string& serverAddress, const std::string& clientId);

    int Connect(const std::string& userName, const std::string& password);
    int ConnectRetry(const std::string& userName, const std::string& password, int reconnectTimeoutS);
    int Disconnect();
    
    int Subscribe(const std::string& topic);
    int Unsubscribe(const std::string& topic);
    
    int PublishAsync(const std::string& topic, const char* message,
        int qos = 0, MQTTClient_deliveryToken* pToken = nullptr);

    void SetLogger(Logger* pLogger) { m_pLogger = pLogger; }

    void SetMessageHandler(std::function<void(std::string_view, std::string_view)> handler)
    {
        m_optMessageHandler = std::move(handler);
    }

private:
    void OnMessageArrived(const char *topicName, MQTTClient_message *messageStruct);
    void OnConnectionLost(const char* cause);

    MQTTClient m_client = nullptr;
    std::unordered_set<std::string> m_subscribedTopics;
    
    Logger* m_pLogger = nullptr;
    std::optional<std::function<void(cstd::string_view, std::string_view)>> m_optMessageHandler;
};

} // namespace MqttWrapper
