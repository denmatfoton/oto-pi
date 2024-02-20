#include "MqttWrapper.h"

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

    if (int rc = MQTTClient_disconnect(m_client, 10000); rc != MQTTCLIENT_SUCCESS)
    {
        printf("Failed to disconnect, return code %d\n", rc);
    }

    MQTTClient_destroy(&m_client);
}
 
void ConnectionLost(void* /*context*/, char *cause)
{
    printf("\nConnection lost\n");
    printf("     cause: %s\n", cause);
}

int Client::Init(const std::string& serverAddress, const std::string& clientId)
{
    if (int rc = MQTTClient_create(&m_client, serverAddress.c_str(), clientId.c_str(),
        MQTTCLIENT_PERSISTENCE_NONE, nullptr); rc != MQTTCLIENT_SUCCESS)
    {
        printf("Failed to create client, return code %d\n", rc);
        return rc;
    }

    if (int rc = MQTTClient_setCallbacks(m_client, this, ConnectionLost,
        [] (void *context, char *topicName, int /*topicLen*/, MQTTClient_message *message) {
            reinterpret_cast<Client*>(context)->OnMessageArrived(topicName, message);
            MQTTClient_freeMessage(&message);
            MQTTClient_free(topicName);
            return 1;
        }, nullptr); rc != MQTTCLIENT_SUCCESS)
    {
        printf("Failed to set callbacks, return code %d\n", rc);
        return rc;
    }

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
        printf("Failed to connect, return code %d\n", rc);
        return rc;
    }

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

int Client::Subscribe(const string& topic)
{
    static constexpr int qos = 1;
    if (int rc = MQTTClient_subscribe(m_client, topic.c_str(), qos); rc != MQTTCLIENT_SUCCESS)
    {
        printf("Failed to subscribe, return code %d\n", rc);
        return rc;
    }

    m_subscribedTopics.insert(topic);

    return MQTTCLIENT_SUCCESS;
}

int Client::Unsubscribe(const string& topic)
{
    if (int rc = MQTTClient_unsubscribe(m_client, topic.c_str()); rc != MQTTCLIENT_SUCCESS)
    {
        printf("Failed to unsubscribe, return code %d\n", rc);
        return rc;
    }
    
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
        printf("Failed to publish message, return code %d\n", rc);
        return rc;
    }

    return MQTTCLIENT_SUCCESS;
}

void Client::OnMessageArrived(const char *topicName, MQTTClient_message *message)
{
    printf("Message arrived:\n");
    printf("    Topic: %s\n", topicName);
    printf("    message: %.*s\n", message->payloadlen, reinterpret_cast<const char*>(message->payload));
}

} // namespace MqttWrapper