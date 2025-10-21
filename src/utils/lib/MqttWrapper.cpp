#include "MqttWrapper.h"

#include "Logger.h"

#include <chrono>
#include <cstring>
#include <thread>

using namespace std;

namespace MqttWrapper {

Client::~Client()
{
    if (m_client == nullptr)
    {
        return;
    }

    // move, because m_subscribedTopics will be modified in the loop
    unordered_set<string> subscribedTopics(move(m_subscribedTopics));
    for (const string& topic : subscribedTopics)
    {
        Unsubscribe(topic);
    }

    Disconnect();

    MQTTClient_destroy(&m_client);

    LogInfo("MQTT client destroyed");
}

int Client::Init(const std::string& serverAddress, const std::string& clientId)
{
    if (int rc = MQTTClient_create(&m_client, serverAddress.c_str(), clientId.c_str(),
        MQTTCLIENT_PERSISTENCE_NONE, nullptr); rc != MQTTCLIENT_SUCCESS)
    {
        LogError("Failed to create client, return code %d\n", rc);
        return rc;
    }

    if (int rc = MQTTClient_setCallbacks(m_client, this, 
        [] (void* context, char* cause) {
            reinterpret_cast<Client*>(context)->OnConnectionLost(cause);
        },
        [] (void* context, char* topicName, int /*topicLen*/, MQTTClient_message *message) {
            reinterpret_cast<Client*>(context)->OnMessageArrived(topicName, message);
            MQTTClient_freeMessage(&message);
            MQTTClient_free(topicName);
            return 1;
        }, nullptr); rc != MQTTCLIENT_SUCCESS)
    {
        LogError("Failed to set callbacks, return code %d\n", rc);
        return rc;
    }
    
    LogInfo("MQTT client '%s' initialized. Server address: '%s'", clientId.c_str(), serverAddress.c_str());

    return MQTTCLIENT_SUCCESS;
}

int Client::Connect(const string& userName, const string& password)
{
    MQTTClient_connectOptions connectOptions = MQTTClient_connectOptions_initializer;
    connectOptions.keepAliveInterval = 20;
    connectOptions.cleansession = 1;
    if (!userName.empty())
    {
        connectOptions.username = userName.c_str();
    }
    if (!password.empty())
    {
        connectOptions.password = password.c_str();
    }

    if (int rc = MQTTClient_connect(m_client, &connectOptions); rc != MQTTCLIENT_SUCCESS)
    {
        LogError("Failed to connect, return code %d\n", rc);
        return rc;
    }

    LogInfo("Connected to MQTT server");

    return MQTTCLIENT_SUCCESS;
}

int Client::ConnectRetry(const string& userName, const string& password, int reconnectTimeoutS)
{
    while (true)
    {
        if (int rc = Connect(userName, password); rc != MQTTCLIENT_FAILURE)
        {
            return rc;
        }
        
        this_thread::sleep_for(chrono::seconds(reconnectTimeoutS));
    }
}

int Client::Disconnect()
{
    int rc = MQTTClient_disconnect(m_client, 10000);

    if (rc != MQTTCLIENT_SUCCESS)
    {
        LogError("Failed to disconnect, return code %d\n", rc);
    }
    else
    {
        LogInfo("Client disconnected");
    }

    return rc;
}

int Client::Subscribe(const string& topic)
{
    static constexpr int qos = 1;
    if (int rc = MQTTClient_subscribe(m_client, topic.c_str(), qos); rc != MQTTCLIENT_SUCCESS)
    {
        LogError("Failed to subscribe to topic '%s', return code %d\n", topic.c_str(), rc);
        return rc;
    }

    LogInfo("Subscribed to topic '%s'", topic.c_str());

    m_subscribedTopics.insert(topic);

    return MQTTCLIENT_SUCCESS;
}

int Client::Unsubscribe(const string& topic)
{
    if (int rc = MQTTClient_unsubscribe(m_client, topic.c_str()); rc != MQTTCLIENT_SUCCESS)
    {
        LogError("Failed to unsubscribe from topic '%s', return code %d\n", topic.c_str(), rc);
        return rc;
    }
    
    LogInfo("Unsubscribed from topic '%s'", topic.c_str());
    
    m_subscribedTopics.erase(topic);

    return MQTTCLIENT_SUCCESS;
}

int Client::PublishAsync(const string& topic, const char* message, int qos, MQTTClient_deliveryToken* pToken)
{
    MQTTClient_message msgStruct = MQTTClient_message_initializer;
    msgStruct.payload = const_cast<char*>(message);
    msgStruct.payloadlen = static_cast<int>(strlen(message));
    msgStruct.qos = qos;
    msgStruct.retained = 0;

    if (int rc = MQTTClient_publishMessage(m_client, topic.c_str(), &msgStruct, pToken); rc != MQTTCLIENT_SUCCESS)
    {
        LogError("Failed to publish topic: '%s', message: '%s', return code %d", topic.c_str(), message, rc);
        return rc;
    }
    
    LogInfo("Published topic: '%s', message: '%s'", topic.c_str(), message);

    return MQTTCLIENT_SUCCESS;
}

void Client::OnMessageArrived(const char *topicName, MQTTClient_message *messageStruct)
{
    const char* message = reinterpret_cast<const char*>(messageStruct->payload);
    LogInfo("Received topic: '%s', message: '%.*s'", topicName, messageStruct->payloadlen, message);
    if (m_optMessageHandler)
    {
        m_optMessageHandler->operator()(string_view(topicName), string_view(message, messageStruct->payloadlen));
    }
    else
    {
        LogWarning("No message handler set");
    }
}

void Client::OnConnectionLost(const char* cause)
{
    LogWarning("Cause: %s", cause);
}

} // namespace MqttWrapper