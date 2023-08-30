/**
 * @mqtt_trigger.h
 * @brief Creating and Running MQTT based trigger
 *
 * This contains the function definitions for creating and running MQTT based capture trigger.
 *
 */

#ifndef __MQTT_TRIGGER_H__
#define __MQTT_TRIGGER_H__

#include <iostream>
#include <fstream>
#include <iomanip>
#include <algorithm>
#include <atomic>
#include <queue>
#include <chrono>
#include <mutex>
#include <set>

#include <aws/iot/MqttClient.h>
#include <aws/crt/Api.h>
#include <aws/crt/io/HostResolver.h>
#include <aws/greengrass/GreengrassCoreIpcClient.h>

#include <edge-ml-accelerator/trigger/base_trigger.h>

using namespace Aws::Crt;
using namespace Aws::Greengrass;
using namespace edgeml::utils;

namespace edgeml
{
    namespace trigger
    {

        Aws::Crt::ApiHandle apiHandleMqtt(g_allocator);
        std::string outMessageStringMqtt_ = "";

        class IoTCoreResponseHandler : public SubscribeToIoTCoreStreamHandler
        {
            public:
                virtual ~IoTCoreResponseHandler() {}

                void OnStreamEvent(IoTCoreMessage *response) override
                {
                    auto jsonMessage = response->GetMessage();
                    auto binaryMessage = response->GetMessage();
                    if (jsonMessage.has_value() && jsonMessage.value().GetPayload().has_value())
                    {
                        // Handle JSON message.
                        auto messageBytes = binaryMessage.value().GetPayload().value();
                        outMessageStringMqtt_ = std::string(messageBytes.begin(), messageBytes.end());
                    }
                    else if (binaryMessage.has_value() && binaryMessage.value().GetPayload().has_value())
                    {
                        // Handle Binary message.
                        auto messageBytes = binaryMessage.value().GetPayload().value();
                        outMessageStringMqtt_ = std::string(messageBytes.begin(), messageBytes.end());
                    }
                }

                bool OnStreamError(OperationError *error) override
                {
                    LOG_ERROR("[TRIGGER::MQTT Trigger] OnStreamError");
                    return true;
                }

                void OnStreamClosed() override
                {
                    LOG_ALWAYS("[TRIGGER::MQTT Trigger] OnStreamClosed");
                }

            private:
                std::mutex mtx_;

        };

        class MqttTrigger : public Trigger
        {
            public:
                IoTCoreResponseHandler* streamHandler_;
                std::vector<std::string> topic_list_;

                MqttTrigger(utils::jsonParser::jValue j, int cameraIndex);
                void getTrigger(int& errc, int& iter);
                ~MqttTrigger();

            private:
                GreengrassCoreIpcClient* mqttClient;
                QOS qos = QOS_AT_MOST_ONCE;
                std::mutex mtx_;
                long int timeoutDelay;
                std::set<std::string> captureStrs {"capture", "CAPTURE", "{capture}", "{CAPTURE}"};
                std::set<std::string> inferStrs {"infer", "INFER", "inference", "INFERENCE", "{infer}", "{INFER}", "{inference}", "{INFERENCE}"};
                std::chrono::steady_clock::time_point start_timeout_ = std::chrono::steady_clock::now();
                std::chrono::steady_clock::time_point end_timeout_ = std::chrono::steady_clock::now();
                std::chrono::duration<double> duration_timeout_;
                std::chrono::steady_clock::time_point trigger_start_time_ = std::chrono::steady_clock::now();
                std::chrono::steady_clock::time_point trigger_end_time_ = std::chrono::steady_clock::now();
                std::chrono::duration<double> trigger_elapsed_seconds_;
        };

    }
}

#endif